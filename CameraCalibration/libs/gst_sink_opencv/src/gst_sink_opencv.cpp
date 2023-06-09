#include "gst_sink_opencv.hpp"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/highgui/highgui.hpp>

#include <mutex>
#include <chrono>
#include <utility>

using namespace std;

GstSinkOpenCV::GstSinkOpenCV( std::string input_pipeline, int bufSize, bool debug )
{
    mFrameBufferSize = bufSize;

    mPipelineStr = std::move(input_pipeline);
    mPipeline = nullptr;
    mSink = nullptr;

    mDebug = debug;
}

GstSinkOpenCV::~GstSinkOpenCV()
{
    if( mPipeline )
    {
        /* cleanup and exit */
        gst_element_set_state( mPipeline, GST_STATE_NULL );
        gst_object_unref( mPipeline );
        mPipeline = nullptr;
    }

    /*if(mSink)
    {
        gst_object_unref( mSink );
        mSink=NULL;
    }*/

}

GstSinkOpenCV* GstSinkOpenCV::Create(string input_pipeline, size_t bufSize, int timeout_sec, bool debug )
{
    auto* gstSinkOpencv = new GstSinkOpenCV( std::move(input_pipeline), bufSize, debug );

    if( !gstSinkOpencv->init( timeout_sec ) )
    {
        delete gstSinkOpencv;
        return nullptr;
    }

    return gstSinkOpencv;
}

bool GstSinkOpenCV::init( int timeout_sec )
{
    GError *error = nullptr;
    GstStateChangeReturn ret;

    mPipelineStr += " ! videoconvert ! " \
                    "appsink name=sink caps=\"video/x-raw,format=BGR\"";

    cout << endl << "Input pipeline:" << endl << mPipelineStr << endl << endl;

    mPipeline = gst_parse_launch( mPipelineStr.c_str(), &error );

    if (error != nullptr)
    {
        g_print ("*** Error *** could not construct pipeline: %s\n", error->message);
        g_clear_error (&error);
        return NULL;
    }

    /* set to PAUSED to make the first frame arrive in the sink */
    ret = gst_element_set_state (mPipeline, GST_STATE_PLAYING);
    switch (ret)
    {
    case GST_STATE_CHANGE_FAILURE:
        g_print ("Failed to play the pipeline\n");
        return false;

    case GST_STATE_CHANGE_NO_PREROLL:
        /* for live sources, we need to set the pipeline to PLAYING before we can
           * receive a buffer. We don't do that yet */
        g_print ("Waiting for first frame\n");
        break;
    default:
        break;
    }

    /* This can block for up to "timeout_sec" seconds. If your machine is really overloaded,
       * it might time out before the pipeline prerolled and we generate an error. A
       * better way is to run a mainloop and catch errors there. */
    ret = gst_element_get_state( mPipeline, nullptr, nullptr, timeout_sec * GST_SECOND );
    if (/*ret == GST_STATE_CHANGE_FAILURE*/ret!=GST_STATE_CHANGE_SUCCESS)
    {
        g_print ("Source connection timeout\n");
        return false;
    }

    /* get sink */
    mSink = gst_bin_get_by_name (GST_BIN (mPipeline), "sink");

    GstSample *sample;
    g_signal_emit_by_name (mSink, "pull-preroll", &sample, NULL);

    /* if we have a buffer now, convert it to a pixbuf. It's possible that we
       * don't have a buffer because we went EOS right away or had an error. */
    if (sample)
    {
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *s;
        GstMapInfo map;

        gint width;
        gint height;

        gboolean res;

        /* get the snapshot buffer format now. We set the caps on the appsink so
         * that it can only be an rgb buffer. The only thing we have not specified
         * on the caps is the height, which is dependant on the pixel-aspect-ratio
         * of the source material */
        caps = gst_sample_get_caps (sample);
        if (!caps)
        {
            g_print ("could not get the format of the first frame\n");
            return false;
        }
        s = gst_caps_get_structure (caps, 0);

        /* we need to get the final caps on the buffer to get the size */
        res = gst_structure_get_int (s, "width", &width);
        res |= gst_structure_get_int (s, "height", &height);
        if (!res)
        {
            g_print ("could not get the dimensions of the first frame\n");
            return false;
        }

        //cerr << width << height << endl;

        /* create pixmap from buffer and save, gstreamer video buffers have a stride
         * that is rounded up to the nearest multiple of 4 */
        buffer = gst_sample_get_buffer (sample);

        /* Mapping a buffer can fail (non-readable) */
        if (gst_buffer_map (buffer, &map, GST_MAP_READ))
        {
            mWidth = width;
            mHeight = height;
            mChannels = map.size / (mWidth*mHeight);

            if( mFrameBuffer.size()<mFrameBufferSize )
            {
                cv::Mat frame( mHeight, mWidth, CV_8UC3 );

                memcpy( frame.data, map.data, map.size );

                if (mDebug)
                {
                    cv::imshow("First Frame", frame);
                    cv::waitKey(5);
                }

                mFrameMutex.lock();
                mFrameBuffer.push( std::move(frame) );
                mFrameMutex.unlock();
            }

            gst_buffer_unmap (buffer, &map);
        }
        gst_sample_unref (sample);
    }
    else
    {
        g_print ("could not get first frame \n");
        return false;
    }

    /* we use appsink in push mode, it sends us a signal when data is available
       * and we pull out the data in the signal callback. We want the appsink to
       * push as fast as it can, hence the sync=false */
    g_object_set (G_OBJECT (mSink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect( mSink, "new-sample", G_CALLBACK(on_new_sample_from_sink), this );

    return true;
}

GstFlowReturn GstSinkOpenCV::on_new_sample_from_sink( GstElement* elt, GstSinkOpenCV* sinkData )
{
    GstSample *sample;
    //GstFlowReturn ret;

    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample( GST_APP_SINK (elt) );

    if (sample)
    {
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *s;
        GstMapInfo map;

        gint width;
        gint height;

        gboolean res;

        /* get the snapshot buffer format now. We set the caps on the appsink so
         * that it can only be an rgb buffer. The only thing we have not specified
         * on the caps is the height, which is dependant on the pixel-aspect-ratio
         * of the source material */
        caps = gst_sample_get_caps (sample);
        if (!caps)
        {
            g_print ("could not get the format of the first frame\n");
            return GST_FLOW_CUSTOM_ERROR;
        }
        s = gst_caps_get_structure (caps, 0);

        /* we need to get the final caps on the buffer to get the size */
        res = gst_structure_get_int (s, "width", &width);
        res |= gst_structure_get_int (s, "height", &height);
        if (!res)
        {
            g_print ("could not get the dimensions of the first frame\n");
            return GST_FLOW_CUSTOM_ERROR;
        }

        /* create pixmap from buffer and save, gstreamer video buffers have a stride
         * that is rounded up to the nearest multiple of 4 */
        buffer = gst_sample_get_buffer (sample);

        /* Mapping a buffer can fail (non-readable) */
        if (gst_buffer_map (buffer, &map, GST_MAP_READ))
        {
            if( width!=sinkData->mWidth || height!=sinkData->mHeight ) // New size?
            {
                g_print( "New Size \n" );

                sinkData->mWidth = width;
                sinkData->mHeight = height;
                sinkData->mChannels = map.size / (sinkData->mWidth*sinkData->mHeight);

                if( sinkData->mChannels!=3 )
                {
                    g_print ("Only BGR image are supported! \n");
                    return GST_FLOW_CUSTOM_ERROR;
                }
            }

            //std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

            sinkData->mFrameMutex.lock();
            if( sinkData->mFrameBuffer.size()<sinkData->mFrameBufferSize )
            {
                cv::Mat frame( sinkData->mHeight, sinkData->mWidth, CV_8UC3 );

                memcpy( frame.data, map.data, map.size );

                if (sinkData->mDebug)
                {
                    cv::imshow("New frame", frame);
                }

                sinkData->mFrameBuffer.push( std::move(frame) );

                //std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
                //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() <<std::endl;
            }
            else
            {
                std::cout << "Buffer full" << endl;
            }
            sinkData->mFrameMutex.unlock();


            gst_buffer_unmap (buffer, &map);
        }
        gst_sample_unref (sample);



    }
    else
    {
        g_print ("could not get first frame \n");
        return GST_FLOW_CUSTOM_ERROR;
    }


    return GST_FLOW_OK;
}

double GstSinkOpenCV::getBufPerc()
{
    return static_cast<double>(mFrameBuffer.size())/mFrameBufferSize;
}

cv::Mat GstSinkOpenCV::getLastFrame()
{
    cv::Mat frame;

    mFrameMutex.lock();
    if (!mFrameBuffer.empty())
    {
        frame = std::move(mFrameBuffer.front());
        mFrameBuffer.pop();
    }
    mFrameMutex.unlock();

    return frame;
}
