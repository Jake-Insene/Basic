#define INSTANCE_SPRITE
#include "bread/Renderer2D/Renderer2D.Header.slang"

layout(set = 0, binding = 0) ConstantBuffer<SceneUniform> Scene;
layout(set = 1, binding = 0) Sampler2D BindTextures[16];

[shader("vertex")]
SpriteFragmentInput VertexMain(SpriteVertexInput Input, uint VertexIndex : SV_VertexID)
{
    ObjectData Object = SpriteVertex(Input, VertexIndex);
    
    SpriteFragmentInput Output;
    Output.Color = Object.Color;
    Output.UV = Object.UV;
    float4 WorldPosition = InstanceToWorld(Object, Object.InstancePosition);
    float4 WorldToClip = WorldToClip(Scene, WorldPosition);
    Output.ClipPosition = WorldToClip;
    
    return Output;
}

[shader("fragment")]
half4 FragmentMain(SpriteFragmentInput Input) : SV_Target
{
    return Input.Color * LoadPixel(BindTextures[Input.TextureIndex], Input.UV);
}
