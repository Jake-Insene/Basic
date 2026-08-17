#include "Basic/Core/Shader.hpp"

#include <IO/File.hpp>


namespace Basic
{

Shader::Shader(Mem::Allocator& allocator, const ShaderInfo& info)
: allocator(allocator), shader_info(info), shader_code(), shader_stages()
{
    shader_info = info;
    shader_code = IO::File::read_all(allocator, info.path);

    shader_stages[0] = GPU::ShaderStageInfo
    {
        .stage = GPU::ShaderStage::Vertex,
        .code = Mem::to_const_bytes(shader_code),
        .name = info.vertex_name,
    };

    shader_stages[1] = GPU::ShaderStageInfo
    {
        .stage = GPU::ShaderStage::Fragment,
        .code = Mem::to_const_bytes(shader_code),
        .name = info.fragment_name,
    };
}

Shader::~Shader()
{
    allocator.free(shader_code);
}

}
