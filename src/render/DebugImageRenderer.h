#pragma once

#include "IVideoRenderer.h"

class DebugImageRenderer : public IVideoRenderer {
public:
    void initialize() override;
    void resize(int width, int height) override;
    void setVideoFrame(const playerlab::core::VideoFrame& frame) override;
    void render() override;

private:
    int width_ = 0;
    int height_ = 0;
};
