#pragma once

class IVideoRenderer {
public:
    virtual ~IVideoRenderer() = default;

    virtual void initialize() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void render() = 0;
};
