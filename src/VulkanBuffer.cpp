#include "VulkanBuffer.h"

#include <iostream>
#include <ostream>

#include "RenderCommand.h"

// VertexBuffer

VulkanVertexBuffer::VulkanVertexBuffer(std::span<const shaderio::Vertex> vertices)
{
    std::cout << "Creating Vertex Buffer" << std::endl;

    VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
    VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(), instance.GetTransientCmdPool());
    // Buffer of all vertices
    utils::Buffer m_vertexBuffer = instance.GetAllocator().createBufferAndUploadData(cmd, std::span<const shaderio::Vertex>(vertices),
                                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    DBG_VK_NAME(m_vertexBuffer.buffer);

    utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(), instance.GetTransientCmdPool(), instance.GetContext().getGraphicsQueue().queue);

    instance.setVertexBuffer(m_vertexBuffer);
    
    instance.GetAllocator().freeStagingBuffers(); // Data is uploaded, staging buffers can be released
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
    
}

void VulkanVertexBuffer::Bind() const
{
}

void VulkanVertexBuffer::Unbind() const
{
}

// IndexBuffer

VulkanIndexBuffer::VulkanIndexBuffer(std::span<const uint32_t> indices, uint32_t count)//count * sizeof(uint32) ??
    : m_Count(count)
{
    std::cout << "Creating Index Buffer" << std::endl;

    VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    
    VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(), instance.GetTransientCmdPool());
    // Buffer of all vertices
    utils::Buffer m_indexBuffer = instance.GetAllocator().createBufferAndUploadData(cmd, std::span<const uint32_t>(indices),
                                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    DBG_VK_NAME(m_indexBuffer.buffer);

    utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(), instance.GetTransientCmdPool(), instance.GetContext().getGraphicsQueue().queue);

    instance.setIndexBuffer(m_indexBuffer);
    
    instance.GetAllocator().freeStagingBuffers(); // Data is uploaded, staging buffers can be released
}

VulkanIndexBuffer::~VulkanIndexBuffer()
{
    
}

void VulkanIndexBuffer::Bind() const
{
}

void VulkanIndexBuffer::Unbind() const
{
}
