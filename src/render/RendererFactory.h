#pragma once

#include <memory>

#include "IVideoRenderer.h"

enum class RendererType {
    OpenGL,
    DebugImage
};

class RendererFactory {
public:
    static std::unique_ptr<IVideoRenderer> create(RendererType type);
};
