#pragma once
#include <cstdint>

#include <span>


#include <array>
#include <glm/glm.hpp>
namespace shaderio
{
    using namespace glm;
#include "shaders/shader_io.h"
}
class VertexBuffer
{
public:
    virtual ~VertexBuffer() {}

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    static VertexBuffer* Create(std::span<const shaderio::Vertex> vertices);
};

class IndexBuffer
{
public:
    virtual ~IndexBuffer() {}

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual uint32_t GetCount() const = 0;

    static IndexBuffer* Create(std::span<const uint32_t> indices, uint32_t count);
};