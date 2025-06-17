#include "Texture.h"

#include "RendererAPI.h"
#include "VulkanTexture.h"

std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path)
{
    switch (RendererAPI::GetAPI())
    {
    case RenderAPIType::None: return nullptr;
    case RenderAPIType::Vulkan: return std::make_shared<VulkanTexture2D>(path);
    }

    return nullptr;
}
