#include "RendererFactory.h"

#include "DebugImageRenderer.h"
#include "OpenGLVideoRenderer.h"

std::unique_ptr<IVideoRenderer> RendererFactory::create(RendererType type) {
    switch (type) {
        case RendererType::OpenGL:
            return std::make_unique<OpenGLVideoRenderer>();
        case RendererType::DebugImage:
            return std::make_unique<DebugImageRenderer>();
        default:
            return nullptr;
    }
}
