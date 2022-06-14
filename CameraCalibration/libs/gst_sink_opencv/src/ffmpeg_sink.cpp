#include "ffmpeg_sink.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
}

struct AVFrameDeleter
{
    void operator()(AVFrame *frame) const { av_frame_free(&frame); };
};

typedef std::unique_ptr<AVFrame, AVFrameDeleter> AVFramePtr;

FFmpegThread::~FFmpegThread()
{
    avcodec_free_context(&m_codecContext);
    avformat_close_input(&m_formatContext);
}

bool FFmpegThread::init()
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    avdevice_register_all();

    avformat_network_init();

    m_formatContext = avformat_alloc_context();

    AVDictionary *streamOpts = nullptr;
    AVInputFormat* iformat = nullptr;
    if (!m_inputFormat.empty())
    {
        iformat = av_find_input_format(m_inputFormat.c_str());
        av_dict_set(&streamOpts, "rtbufsize", "15000000", 0); // https://superuser.com/questions/1158820/ffmpeg-real-time-buffer-issue-rtbufsize-parameter
    }
    int error = avformat_open_input(&m_formatContext,
        m_url.c_str(), 
        iformat,
        nullptr);

    if (error != 0)
    {
        return false;
    }

    m_streamNumber = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_streamNumber < 0) {
        return false;
    }

    m_codecContext = avcodec_alloc_context3(nullptr);
    if (m_codecContext == nullptr) {
        return false;
    }

    if (avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[m_streamNumber]->codecpar) < 0) {
        return false;
    }

    auto codec = avcodec_find_decoder(m_codecContext->codec_id);
    if (codec == nullptr)
    {
        return false;  // Codec not found
    }

    // Open codec
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0)
    {
        assert(false && "Error on codec opening");
        return false;  // Could not open codec
    }

    return true;
}

void FFmpegThread::run()
{
    AVFramePtr videoFrame(av_frame_alloc());
    AVPacket packet;
    while (!isInterruptionRequested() && av_read_frame(m_formatContext, &packet) >= 0)
    {
        if (packet.stream_index == m_streamNumber)
        {
            // Here it goes
            const int ret = avcodec_send_packet(m_codecContext, &packet);
            if (ret < 0)
            {
                av_packet_unref(&packet);
                return;
            }
            while (!isInterruptionRequested() && avcodec_receive_frame(m_codecContext, videoFrame.get()) == 0)
            {
                // transformation

                AVPacket avEncodedPacket;

                av_init_packet(&avEncodedPacket);
                avEncodedPacket.data = NULL;
                avEncodedPacket.size = 0;


                cv::Mat img(videoFrame->height, videoFrame->width, CV_8UC3);

                int stride = img.step[0];

                auto img_convert_ctx = sws_getCachedContext(
                    NULL,
                    m_codecContext->width,
                    m_codecContext->height,
                    m_codecContext->pix_fmt,
                    m_codecContext->width,
                    m_codecContext->height,
                    AV_PIX_FMT_BGR24,
                    SWS_POINT,//SWS_FAST_BILINEAR,
                    NULL, NULL, NULL);
                sws_scale(img_convert_ctx, videoFrame->data, videoFrame->linesize, 0, m_codecContext->height,
                    &img.data,
                    &stride);

                emit newImage(img);
            }
        }
        av_packet_unref(&packet);
    }

}
