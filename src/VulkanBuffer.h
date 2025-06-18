#pragma once

#include "Buffer.h"
#include "VulkanRendererAPI.h"

class VulkanVertexBuffer : public VertexBuffer
{
public:
    VulkanVertexBuffer(std::span<const shaderio::Vertex> vertices);
    virtual ~VulkanVertexBuffer();

    virtual void Bind() const;
    virtual void Unbind() const;
private:
    uint32_t m_RendererID;
};

class VulkanIndexBuffer : public IndexBuffer
{
public:
    VulkanIndexBuffer(std::span<const uint32_t> indices, uint32_t count);
    virtual ~VulkanIndexBuffer();

    virtual void Bind() const;
    virtual void Unbind() const;

    virtual uint32_t GetCount() const { return m_Count; }
private:
    uint32_t m_RendererID;
    uint32_t m_Count;
};