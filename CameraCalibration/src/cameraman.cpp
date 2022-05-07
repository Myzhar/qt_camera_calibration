#include "cameraman.h"

#include <gst/gst.h>

#include <QObject>

namespace {


GST_DEBUG_CATEGORY(devmon_debug);
#define GST_CAT_DEFAULT devmon_debug


gchar* get_launch_line(GstDevice * device)
{
    static const char *const ignored_propnames[] =
    { "name", "parent", "direction", "template", "caps", NULL };
    GString *launch_line;
    GstElement *element;
    GstElement *pureelement;
    GParamSpec **properties, *property;
    GValue value = G_VALUE_INIT;
    GValue pvalue = G_VALUE_INIT;
    guint i, number_of_properties;
    GstElementFactory *factory;

    element = gst_device_create_element(device, NULL);

    if (!element)
        return NULL;

    factory = gst_element_get_factory(element);
    if (!factory) {
        gst_object_unref(element);
        return NULL;
    }

    if (!gst_plugin_feature_get_name(factory)) {
        gst_object_unref(element);
        return NULL;
    }

    launch_line = g_string_new(gst_plugin_feature_get_name(factory));

    pureelement = gst_element_factory_create(factory, NULL);

    /* get paramspecs and show non-default properties */
    properties =
        g_object_class_list_properties(G_OBJECT_GET_CLASS(element),
            &number_of_properties);
    if (properties) {
        for (i = 0; i < number_of_properties; i++) {
            gint j;
            gboolean ignore = FALSE;
            property = properties[i];

            /* skip some properties */
            if ((property->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE)
                continue;

            for (j = 0; ignored_propnames[j]; j++)
                if (!g_strcmp0(ignored_propnames[j], property->name))
                    ignore = TRUE;

            if (ignore)
                continue;

            /* Can't use _param_value_defaults () because sub-classes modify the
             * values already.
             */

            g_value_init(&value, property->value_type);
            g_value_init(&pvalue, property->value_type);
            g_object_get_property(G_OBJECT(element), property->name, &value);
            g_object_get_property(G_OBJECT(pureelement), property->name, &pvalue);
            if (gst_value_compare(&value, &pvalue) != GST_VALUE_EQUAL) {
                gchar *valuestr = gst_value_serialize(&value);

                if (!valuestr) {
                    GST_WARNING("Could not serialize property %s:%s",
                        GST_OBJECT_NAME(element), property->name);
                    g_free(valuestr);
                    goto next;
                }

                g_string_append_printf(launch_line, " %s=%s",
                    property->name, valuestr);
                g_free(valuestr);

            }

        next:
            g_value_unset(&value);
            g_value_unset(&pvalue);
        }
        g_free(properties);
    }

    gst_object_unref(element);
    gst_object_unref(pureelement);

    return g_string_free(launch_line, FALSE);
}


void unescape_value_string(gchar * s)
{
    gchar *w;

    if (*s == 0)
        return;

    if (*s != '"') {
        return ;
    }

    /* Find the closing quotes */
    w = s;
    s++;

    while (*s != '"') {
        if (*s == 0)
            break;
        if (*s == '\\') {
            s++;
            if (*s == 0)
                break;
        }
        *w = *s;
        w++;
        s++;
    }

    *w = 0;
}


void
print_device(GstDevice * device, gpointer user_data)
{
    //gchar *device_class, *str, *name;
    //GstCaps *caps;
    //GstStructure *props;
    //guint i, size = 0;

    std::vector<CameraDesc>& result = *static_cast<std::vector<CameraDesc>*>(user_data);

    CameraDesc desc;


    auto name = gst_device_get_display_name(device);
    desc.description = name;
    g_free(name);

    auto str = get_launch_line(device);
    desc.launchLine = str;
    g_free(str);

    //auto device_class = gst_device_get_device_class(device);
    if (auto props = gst_device_get_properties(device))
    {
        const char * path = gst_structure_get_string(props, "device.path");
        desc.id = path;
        gst_structure_free(props);
    } 
    else if (auto element = gst_device_create_element(device, NULL))
    {
        GValue value = G_VALUE_INIT;
        g_object_get_property(G_OBJECT(element), "device-path", &value);
        if (gchar *valuestr = gst_value_serialize(&value))
        {
            unescape_value_string(valuestr);
            desc.id = valuestr;
            g_free(valuestr);
        }
        g_value_unset(&value);
        gst_object_unref(element);
    }

    //g_print("\nDevice %s:\n\n", modified ? "modified" : "found");
    //g_print("\tname  : %s\n", name);

    //g_print("\tclass : %s\n", device_class);

    guint size = 0;
    auto caps = gst_device_get_caps(device);
    if (caps != NULL)
        size = gst_caps_get_size(caps);

    for (int i = 0; i < size; ++i) {
        GstStructure *s = gst_caps_get_structure(caps, i);
        //GstCapsFeatures *features = gst_caps_get_features(caps, i);
        //int height, width;
        CameraMode mode{};
        gst_structure_get_int(s, "width", &mode.w);
        gst_structure_get_int(s, "height", &mode.h);
        if (auto framerates = gst_structure_get_value(s, "framerate")) {
            if (GST_VALUE_HOLDS_FRACTION(framerates))
            {
                mode.den = gst_value_get_fraction_numerator(framerates);
                mode.num = gst_value_get_fraction_denominator(framerates);
                mode.fps = double(mode.den) / mode.num;

                desc.modes.push_back(mode);
            }
            else if (GST_VALUE_HOLDS_LIST(framerates))
            {
                const auto num_framerates = gst_value_list_get_size(framerates);
                for (i = 0; i < num_framerates; i++)
                {
                    const GValue *value;
                    value = gst_value_list_get_value(framerates, i);
                    mode.den = gst_value_get_fraction_numerator(value);
                    mode.num = gst_value_get_fraction_denominator(value);
                    mode.fps = double(mode.den) / mode.num;

                    desc.modes.push_back(mode);
                }
            }
            else if (GST_VALUE_HOLDS_FRACTION_RANGE(framerates))
            {
                const auto fraction_range_min = gst_value_get_fraction_range_min(framerates);
                const auto numerator_min = gst_value_get_fraction_numerator(fraction_range_min);
                const auto denominator_min = gst_value_get_fraction_denominator(fraction_range_min);

                const auto fraction_range_max = gst_value_get_fraction_range_max(framerates);
                const auto numerator_max = gst_value_get_fraction_numerator(fraction_range_max);
                const auto denominator_max = gst_value_get_fraction_denominator(fraction_range_max);
                //g_print("FractionRange: %d/%d - %d/%d\n", numerator_min, denominator_min, numerator_max, denominator_max);

                const auto num_framerates = (numerator_max - numerator_min + 1) * (denominator_max - denominator_min + 1);
                for (int i = numerator_min; i <= numerator_max; i++)
                {
                    for (int j = denominator_min; j <= denominator_max; j++)
                    {
                        mode.den = i;
                        mode.num = j;
                        mode.fps = double(mode.den) / mode.num;

                        desc.modes.push_back(mode);
                    }
                }
            }
        }


        //g_print("\t%s %s", (i == 0) ? "caps  :" : "       ",
        //    gst_structure_get_name(s));
        //if (features && (gst_caps_features_is_any(features) ||
        //    !gst_caps_features_is_equal(features,
        //        GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY))) {
        //    gchar *features_string = gst_caps_features_to_string(features);

        //    g_print("(%s)", features_string);
        //    g_free(features_string);
        //}
        //gst_structure_foreach(s, print_field, NULL);
        //g_print("\n");
    }
    //if (props) {
    //    g_print("\tproperties:");
    //    gst_structure_foreach(props, print_structure_field, NULL);
    //    gst_structure_free(props);
    //    g_print("\n");
    //}

    /*
    str = get_launch_line(device);
    if (gst_device_has_classes(device, "Source"))
        g_print("\tgst-launch-1.0 %s ! ...\n", str);
    else if (gst_device_has_classes(device, "Sink"))
        g_print("\tgst-launch-1.0 ... ! %s\n", str);
    else if (gst_device_has_classes(device, "CameraSource")) {
        g_print("\tgst-launch-1.0 %s.vfsrc name=camerasrc ! ... "
            "camerasrc.vidsrc ! [video/x-h264] ... \n", str);
    }

    g_free(str);
    g_print("\n");
    */

    //g_free(name);
    //g_free(device_class);

    if (caps != NULL)
        gst_caps_unref(caps);

    result.push_back(std::move(desc));
}


gboolean
bus_msg_handler(GstBus * bus, GstMessage * msg, gpointer user_data)
{
    GstDevice *device;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_DEVICE_ADDED:
        gst_message_parse_device_added(msg, &device);
        print_device(device, user_data);
        gst_object_unref(device);
        break;
    //case GST_MESSAGE_DEVICE_REMOVED:
    //    gst_message_parse_device_removed(msg, &device);
    //    device_removed(device);
    //    gst_object_unref(device);
    //    break;
    case GST_MESSAGE_DEVICE_CHANGED:
        gst_message_parse_device_changed(msg, &device, NULL);
        print_device(device, user_data);
        gst_object_unref(device);
        break;
    default:
        g_print("%s message\n", GST_MESSAGE_TYPE_NAME(msg));
        break;
    }

    return TRUE;
}

gboolean
quit_loop(GMainLoop * loop)
{
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

GstDeviceMonitor *device_monitor()
{
    GstDeviceMonitor *monitor;
    GstBus *bus;
    GstCaps *caps;

    // starts the monitor for the devices 
    monitor = gst_device_monitor_new();

    // starts the bus for the monitor
    //bus = gst_device_monitor_get_bus(monitor);
    //gst_object_unref(bus);

    // adds a filter to scan for only video devices
    caps = gst_caps_new_empty_simple("video/x-raw");
    gst_device_monitor_add_filter(monitor, "Video/Source", caps);
    gst_caps_unref(caps);

    //const auto ok = gst_device_monitor_start(monitor);

    return monitor;
}


} // namespace

std::vector<CameraDesc> getCameraDescriptions()
{
    std::vector<CameraDesc> result;

    /*
    g_set_prgname("gst-device-monitor-");

    GST_DEBUG_CATEGORY_INIT(devmon_debug, "device-monitor", 0,
        "gst-device-monitor");

    gboolean include_hidden = FALSE;
    auto loop = g_main_loop_new(NULL, FALSE);
    auto monitor = gst_device_monitor_new();
    gst_device_monitor_set_show_all_devices(monitor, include_hidden);

    auto bus = gst_device_monitor_get_bus(monitor);
    auto bus_watch_id = gst_bus_add_watch(bus, bus_msg_handler, &result);
    gst_object_unref(bus);

    if (gst_device_monitor_start(monitor)) 
    {
        // https://github.com/huskyroboticsteam/Orpheus/blob/084ccacf19f520836f15db4abb37f678d3f20993/Rover/GStreamer/device-scanner.cpp
        // adds a filter to scan for only video devices
        auto caps = gst_caps_new_empty_simple("video/x-raw");
        gst_device_monitor_add_filter(monitor, "Video/Source", caps);
        gst_caps_unref(caps);

        // Consume all the messages pending on the bus and exit
        g_idle_add((GSourceFunc)quit_loop, loop);

        g_main_loop_run(loop);

        gst_device_monitor_stop(monitor);
    }
    gst_object_unref(monitor);

    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    */

    // create the monitor
    auto monitor = device_monitor();
    auto dev = gst_device_monitor_get_devices(monitor);

    // loop for the lists
    GList *cur = g_list_first(dev);
    while (cur != NULL)
    {
        GstDevice * device = (GstDevice *)cur->data;

        print_device(device, &result);

        cur = g_list_next(cur);
    }
    //gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);

    return result;
}

QString CameraMode::getDescr() const
{
    return QObject::tr("%1 x %2 @ %3 FPS [ %4 / %5 sec ]").arg(w).arg(h).arg(fps).arg(num).arg(den);
}
