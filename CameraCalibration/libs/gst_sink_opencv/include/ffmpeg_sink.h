#pragma once

#include "camerathreadbase.h"

#include <string>
#include <utility>
#include <atomic>

#include <opencv2/core/core.hpp>

struct AVFormatContext;
struct AVCodecContext;

class FFmpegThread : public CameraThreadBase
{
    Q_OBJECT

public:
    FFmpegThread(std::string url, std::string inputFormat)
        : m_url(std::move(url)), m_inputFormat(std::move(inputFormat))
    {}

    ~FFmpegThread();

    bool init();

    double getBufPerc() override { return m_queueSize / 10.; }
    void dataConsumed() override { --m_queueSize; }

signals:
    void newImage(cv::Mat frame);

protected:
    void run() override;

private:
    std::string m_url;
    std::string m_inputFormat;

    AVFormatContext* m_formatContext{};
    AVCodecContext* m_codecContext{};
    int m_streamNumber;

    std::atomic_int m_queueSize;
};
