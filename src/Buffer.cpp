#include "Buffer.h"
#include "RendererAPI.h"
#include "VulkanBuffer.h"

VertexBuffer* VertexBuffer::Create(std::span<const shaderio::Vertex> vertices)
{
    switch (RendererAPI::GetAPI())
    {
        case RenderAPIType::None: return nullptr; // error hending hzcore asset look hazel
        case RenderAPIType::Vulkan: return new VulkanVertexBuffer(vertices);
    }

    return nullptr;
}

IndexBuffer* IndexBuffer::Create(std::span<const uint32_t> indices, uint32_t count)
{
    switch (RendererAPI::GetAPI())
    {
    case RenderAPIType::None: return nullptr; // error hending hzcore asset look hazel
    case RenderAPIType::Vulkan: return new VulkanIndexBuffer(indices, count);
    }

    return nullptr; 
}