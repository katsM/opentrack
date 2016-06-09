/* Copyright (c) 2012 Patrick Ruoff
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "camera.h"
#include <string>
#include <QDebug>
#include "opentrack-compat/sleep.hpp"
#include "opentrack-compat/camera-names.hpp"

void Camera::set_device(const QString& name)
{
    const int index = camera_name_to_index(name);

    desired_name = name;

    if (desired_index != index)
    {
        desired_index = index;
        _set_device_index();

        // reset fps
        dt_valid = 0;
        dt_mean = 0;
        active_index = index;
    }
}

QString Camera::get_desired_name() const
{
    return desired_name;
}

void Camera::set_fps(int fps)
{
    if (cam_desired.fps != fps)
    {
        cam_desired.fps = fps;
        _set_fps();
    }
}

void Camera::set_res(int x_res, int y_res)
{
    if (cam_desired.res_x != x_res || cam_desired.res_y != y_res)
    {
        cam_desired.res_x = x_res;
        cam_desired.res_y = y_res;
        _set_res();
    }
}

bool Camera::get_info(CamInfo& ret)
{
    if (cam_info.res_x == 0 || cam_info.res_y == 0)
    {
        return false;
    }
    ret = cam_info;
    return true;
}

bool Camera::get_frame(float dt, cv::Mat* frame)
{
    bool new_frame = _get_frame(frame);
    // measure fps of valid frames
    constexpr float dt_smoothing_const = 0.95f;
    dt_valid += dt;
    if (new_frame)
    {
        dt_mean = dt_smoothing_const * dt_mean + (1 - dt_smoothing_const) * dt_valid;
        cam_info.fps = int(std::round(dt_mean > 1e-3f ? 1 / dt_mean : 0));
        dt_valid = 0;
    }
    else
        qDebug() << "pt camera: can't get frame";
    return new_frame;
}

void CVCamera::start()
{
    stop();
    cap = new cv::VideoCapture(desired_index);
    _set_res();
    _set_fps();
    // extract camera info
    if (cap->isOpened())
    {
        active_index = desired_index;
        cam_info.res_x = 0;
        cam_info.res_y = 0;
    } else {
        stop();
    }
}

void CVCamera::stop()
{
    if (cap)
    {
        const bool opened = cap->isOpened();
        if (opened)
        {
            qDebug() << "pt: freeing camera";
            cap->release();
        }
        delete cap;
        cap = nullptr;
        // give opencv time to exit camera threads, etc.
        if (opened)
            portable::sleep(500);
        qDebug() << "pt camera: assuming stopped";
    }
}

bool CVCamera::_get_frame(cv::Mat* frame)
{
    if (cap && cap->isOpened())
    {
        for (int i = 0; i < 100 && !cap->read(*frame); i++)
            ;;

        if (frame->empty())
            return false;

        cam_info.res_x = frame->cols;
        cam_info.res_y = frame->rows;
        return true;
    }
    return false;
}

void CVCamera::_set_fps()
{
    if (cap) cap->set(CV_CAP_PROP_FPS, cam_desired.fps);
}

void CVCamera::_set_res()
{
    if (cap)
    {
        cap->set(CV_CAP_PROP_FRAME_WIDTH,  cam_desired.res_x);
        cap->set(CV_CAP_PROP_FRAME_HEIGHT, cam_desired.res_y);
    }
}
void CVCamera::_set_device_index()
{
    if (desired_index != active_index)
        stop();
}
