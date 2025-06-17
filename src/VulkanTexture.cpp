#include "VulkanTexture.h"

#include "RenderCommand.h"

VulkanTexture2D::VulkanTexture2D(const std::string& path)
    : m_Path(path)
{
    VulkanRendererAPI& instance = VulkanRendererAPI::Get();
    if (instance.GetTextureCount() >= instance.GetMaxTexture())
    {
        instance.ResizeDescriptor();
    }
    // Load and create the images
    const std::vector<std::string> searchPaths = {".", "resources", "../resources", "../../resources"};
        
    std::string filename = utils::findFile(path, searchPaths);
    ASSERT(!filename.empty(), "Could not load texture image!");
        
    VkCommandBuffer cmd = utils::beginSingleTimeCommands(instance.GetContext().getDevice(),
                                                         instance.GetTransientCmdPool());

    stbi_set_flip_vertically_on_load(true); // Flip the image vertically

    // Load the image from disk
    int w, h, comp, req_comp{4};
    stbi_uc* data = stbi_load(filename.c_str(), &w, &h, &comp, req_comp);
    ASSERT(data != nullptr, "Could not load texture image!");
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // Define how to create the image
    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {uint32_t(w), uint32_t(h), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
    };

    // Use the VMA allocator to create the image
    const std::span dataSpan(data, w * h * 4);
    
    utils::ImageResource image =
        instance.GetAllocator().createImageAndUploadData(cmd, dataSpan, imageInfo,
                                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    DBG_VK_NAME(image.image);
    image.extent = {uint32_t(w), uint32_t(h)};

    // Create the image view
    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
    };
    VK_CHECK(vkCreateImageView(instance.GetContext().getDevice(), &viewInfo, nullptr, &image.view));
    DBG_VK_NAME(image.view);

    stbi_image_free(data);

    // Store dimensions
    m_Width = w;
    m_Height = h;

    //return image;
    //m_image = image;

    utils::endSingleTimeCommands(cmd, instance.GetContext().getDevice(),
                                 instance.GetTransientCmdPool(),
                                 instance.GetContext().getGraphicsQueue().queue);

    m_TextureIndex = instance.AddTextureToPool(std::move(image));

   instance.GetAllocator().freeStagingBuffers(); // Data is uploaded, staging buffers can be released
}

VulkanTexture2D::~VulkanTexture2D()
{
    // DO NOTHING - the renderer owns the texture
    // The renderer will clean up all textures when it's destroyed

    /*try {
        // Check 1: Is the static instance valid?
        if (!VulkanRendererAPI::s_instance) {
            return; // Renderer already destroyed, nothing to do
        }
        
        VulkanRendererAPI& instance = VulkanRendererAPI::Get();
        
        // Check 2: Is the renderer properly initialized?
        if (!instance.IsInitialized()) {
            return; // Renderer not initialized
        }
        
        // Check 3: Is our texture index valid?
        if (!instance.IsTextureValid(m_TextureIndex)) {
            return; // Texture already removed or invalid index
        }
        
        // Check 4: Is the texture vector not empty?
        if (instance.GetTextureCount() == 0) {
            return; // No textures to remove
        }
        
        // All checks passed - safe to remove
        instance.RemoveTextureFromPool(m_TextureIndex);
        
    } catch (const std::exception& e) {
        // Log error but don't crash
        LOGW("Failed to remove texture from pool in destructor: %s", e.what());
    } catch (...) {
        // Catch any other exceptions
        LOGW("Unknown error in texture destructor");
    }*/
}