#include "DebugImageRenderer.h"

void DebugImageRenderer::initialize() {}

void DebugImageRenderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void DebugImageRenderer::setVideoFrame(const playerlab::core::VideoFrame& frame) {
    width_ = frame.width;
    height_ = frame.height;
}

void DebugImageRenderer::render() {}
