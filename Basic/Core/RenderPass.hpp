#pragma once
#include "Core/RenderCore.hpp"


namespace Basic
{

struct RenderPass
{
    virtual void setup() = 0;
    
    virtual void render() = 0;
};

}