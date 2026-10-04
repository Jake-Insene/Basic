#pragma once
#include <Mem/Allocator.hpp>
#include <input/input.h>

#include <Basic/2D/SpriteBatch.hpp>


namespace Basic
{

struct Scene
{
    DisableCopy(Scene);
    DisableMove(Scene);

    Scene(Mem::Allocator&) {}
    virtual ~Scene() {}

    virtual void update(f32) {}
    virtual void render(Basic::SpriteBatch&) {}

    virtual void event(const Event&) {}
};

}
