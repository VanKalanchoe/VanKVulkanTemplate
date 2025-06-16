#version 460
//#extension GL_GOOGLE_include_directive : require              // For #include
#extension GL_EXT_scalar_block_layout : require               // For scalar layout
#extension GL_EXT_shader_explicit_arithmetic_types : require  // For uint64_t, ...

//#include "shader_io.h"


#ifndef HOST_DEVICE_H
#define HOST_DEVICE_H

#ifdef __SLANG__
typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;
#define STATIC_CONST static const
#else
#define STATIC_CONST const
#endif

// Layout constants
// Set 0
STATIC_CONST int LSetTextures  = 0;
STATIC_CONST int LBindTextures = 0;
// Set 1
STATIC_CONST int LSetScene      = 1;
STATIC_CONST int LBindSceneInfo = 0;
// Vertex layout
STATIC_CONST int LVPosition = 0;
STATIC_CONST int LVColor    = 1;
STATIC_CONST int LVTexCoord = 2;


struct SceneInfo
{
  uint64_t dataBufferAddress;
  vec2     resolution;
  float    animValue;
  int      numData;
  int      texId;
};

struct PushConstant
{
  vec3 color;
};

struct PushConstantCompute
{
  uint64_t bufferAddress;
  float    rotationAngle;
  int      numVertex;
};

struct Vertex
{
  vec3 position;
  vec3 color;
  vec2 texCoord;
};


#endif  // HOST_DEVICE_H



layout(location = LVPosition) in vec3 inPosition;
layout(location = LVColor) in vec3 inColor;
layout(location = LVTexCoord) in vec2 inUv;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUv;

layout(set = 1, binding = 0, scalar) uniform _SceneInfo
{
  SceneInfo sceneInfo;
};


void main()
{
  vec3 pos = inPosition;

  // Adjust aspect ratio
  float aspectRatio = sceneInfo.resolution.y / sceneInfo.resolution.x;
  pos.x *= aspectRatio;
  // Set the position in clip space
  gl_Position = vec4(pos, 1.0);
  // Pass the color and uv
  outColor = inColor;
  outUv    = inUv;
}