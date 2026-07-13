#pragma once
#include "bread/Core/Header.slang"
#include "bread/Core/Structures.Header.slang"


struct SpriteVertexInput
{
    float4 Attrib0;
    float4 Attrib1;
    float4 Attrib2;
    float4 Attrib3;
};

static float2 _QuadPositions[6] = float2[6](
    float2(0, 1),
    float2(1, 1),
    float2(1, 0),

    float2(1, 0),
    float2(0, 0),
    float2(0, 1)
);

static float2 _UVPositions[6] = float2[6](
    float2(0, 0),
    float2(1, 0),
    float2(1, 1),

    float2(1, 1),
    float2(0, 1),
    float2(0, 0)
);

static float2 GetUnitQuad(uint VertexIndex)
{
    return _QuadPositions[VertexIndex];
}

static float2 GetUnitUVQuad(uint VertexIndex)
{
    return _QuadPositions[VertexIndex];
}

struct SpriteFragmentInput
{
    nointerpolation half4 Color;
    float2 UV;
    float4 ClipPosition : SV_Position;
    nointerpolation uint TextureIndex;
};

ObjectData SpriteVertex(SpriteVertexInput Input, uint VertexIndex)
{
    ObjectData Object;

    uint InputUintColor = reinterpret<uint>(Input.Attrib1.z);
    Object.Color = unpackUnorm4x8ToHalf(InputUintColor);

    Object.UV = GetUnitUVQuad(VertexIndex % 6);

    float2 VertexPosition = GetUnitQuad(VertexIndex % 6);
    Object.VertexPosition = float4(VertexPosition, 0, 1);

    Object.InstancePosition = float4((VertexPosition * Input.Attrib2.zw) + Input.Attrib2.xy, 0, 1);

    Object.WorldTransform = float4x4(
        float4(Input.Attrib0.xy, 0, Input.Attrib1.x),
        float4(Input.Attrib0.zw, 0, Input.Attrib1.y),
        float4(0, 0, 1, 0),
        float4(0, 0, 0, 1)
    );
    
    Object.TextureIndex = reinterpret<uint>(Input.Attrib1.w);
    return Object;
}

