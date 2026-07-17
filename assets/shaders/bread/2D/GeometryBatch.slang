struct BatchBlock
{
    float4x4 ViewProjection;
};

[push_constant] BatchBlock Block;

struct GeometryVertexInput
{
    // .xy | position
    // .z | rgba u8 color
    // .w unused
    float4 A0;
};

struct GeometryFragmentInput
{
    float4 ClipPosition : SV_Position;
    nointerpolation half4 Color;
};

[shader("vertex")]
GeometryFragmentInput VertexMain(GeometryVertexInput Input, uint VertexIndex: SV_VertexID)
{
    GeometryFragmentInput Output;
    Output.ClipPosition = mul(float4(Input.A0.xy, 0, 1), Block.ViewProjection);
    Output.Color = unpackUnorm4x8ToHalf(reinterpret<uint>(Input.A0.z));
    return Output;
}

[shader("fragment")]
half4 FragmentMain(GeometryFragmentInput Input) : SV_Target
{
    return Input.Color;
}
