struct BatchBlock
{
    float4x4 ViewProjection;
};

[push_constant] BatchBlock Block;
layout(set = 0, binding = 0) Sampler2D Texture;

struct SpriteVertexInput
{
    // .xy | position
    // .zw | uv position
    float4 A0;
    // .x | rgba u8 color
    float4 A1;
};

struct SpriteFragmentInput
{
    float4 ClipPosition : SV_Position;
    float2 UV;
    nointerpolation half4 Color;
};

[shader("vertex")]
SpriteFragmentInput VertexMain(SpriteVertexInput Input, uint VertexIndex: SV_VertexID)
{
    SpriteFragmentInput Output;
    Output.ClipPosition = mul(float4(Input.A0.xy, 0, 1), Block.ViewProjection);
    Output.UV = Input.A0.zw;
    Output.Color = unpackUnorm4x8ToHalf(reinterpret<uint>(Input.A1.x));
    return Output;
}

[shader("fragment")]
half4 FragmentMain(SpriteFragmentInput Input) : SV_Target0
{
    return half4(Texture.Sample(Input.UV)) * Input.Color;
}
