#pragma once

#include "Texture.h"
#include "VulkanRendererAPI.h"

class VulkanTexture2D : public Texture2D
{
public:
    VulkanTexture2D(const std::string& path);
    virtual ~VulkanTexture2D();

    virtual uint32_t GetWidth() const override { return m_Width; };
    virtual uint32_t GetHeight() const override { return m_Height; };
    virtual uint32_t GetTextureIndex() const override { return m_TextureIndex; }

private:
    std::string m_Path;//maybe deleted when asset manaager exists only here for hot reloading in the future 
    uint32_t m_Width, m_Height;
    uint32_t m_TextureIndex = 0;
};
