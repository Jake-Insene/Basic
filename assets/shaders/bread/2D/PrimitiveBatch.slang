#include "bread/Core/Header.slang"

//layout(set = 0, binding = 0) ConstantBuffer<SceneUniform> Scene;
//layout(set = 1, binding = 0) Sampler2D BindTextures[16];

struct BatchBlock
{
    float4x4 ViewProjection;
};

[push_constant] BatchBlock Block;

struct PrimitiveVertexInput
{
    float4 A0;
};

struct PrimitiveFragmentInput
{
    float4 ClipPosition : SV_Position;
    nointerpolation half4 Color;
};

[shader("vertex")]
PrimitiveFragmentInput VertexMain(PrimitiveVertexInput Input, uint VertexIndex: SV_VertexID)
{
    PrimitiveFragmentInput Output;
    Output.ClipPosition = mul(float4(Input.A0.xy, 0, 1), Block.ViewProjection);
    Output.Color = unpackUnorm4x8ToHalf(reinterpret<uint>(Input.A0.z));
    return Output;
}

[shader("fragment")]
half4 FragmentMain(PrimitiveFragmentInput Input) : SV_Target
{
    return Input.Color;
}
