#include <iostream>
#include <gst/gst.h>

#include <opencv2/core/core.hpp>

#include <mutex>
#include <queue>

class GstSinkOpenCV
{
    enum { FRAME_BUF_SIZE = 5 };
public:
    static GstSinkOpenCV* Create(std::string input_pipeline, size_t bufSize = FRAME_BUF_SIZE, int timeout_sec=15, bool debug=false );
    ~GstSinkOpenCV();

    cv::Mat getLastFrame();
    double getBufPerc();

private:
    GstSinkOpenCV(std::string input_pipeline, int bufSize, bool debug );
    bool init(int timeout_sec);

    static GstFlowReturn on_new_sample_from_sink(GstElement* elt, GstSinkOpenCV* sinkData );

protected:

private:
    size_t mFrameBufferSize;
    std::string mPipelineStr;

    GstElement* mPipeline;
    GstElement* mSink;

    std::queue<cv::Mat> mFrameBuffer;

    int mWidth{};
    int mHeight{};
    int mChannels{};

    bool mDebug;

    std::mutex mFrameMutex;
};
