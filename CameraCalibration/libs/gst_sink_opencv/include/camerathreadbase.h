#pragma once

#include <QThread>

class CameraThreadBase : public QThread
{
public:
    virtual double getBufPerc() = 0;
};