#pragma once

#include <QString>
#include <vector>

struct CameraMode
{
    int w;
    int h;
    double fps;
    int num;
    int den;

    QString getDescr() const;
};

struct CameraDesc
{
    QString id;
    QString description;
    QString launchLine;
    std::vector<CameraMode> modes;
};

std::vector<CameraDesc> getCameraDescriptions();

