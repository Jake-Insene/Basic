#pragma once
#include <Collections/StringView.hpp>
#include <gpu/gpu.h>


namespace Basic
{

struct ShaderInfo
{
    Collections::StringView path;

    Collections::StringView vertex_name;
    Collections::StringView fragment_name;
};

struct Shader
{
    DisableCopy(Shader);
    DisableMove(Shader);

    Mem::Allocator& allocator;

    ShaderInfo shader_info;
    Slice<u8> shader_code;
    GPU::ShaderStageInfo shader_stages[2];

    Shader(Mem::Allocator& allocator, const ShaderInfo& info);
    ~Shader();

    Slice<const GPU::ShaderStageInfo> get_stages() const { return shader_stages; }
};

}
