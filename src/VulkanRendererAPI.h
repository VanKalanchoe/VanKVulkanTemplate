#pragma once
#include "RendererAPI.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_filesystem.h>
#include <iostream>

/*--
 * We are using Volk that provides a simple way to load Vulkan functions.
 * This is also loading all extensions and functions up to Vulkan 1.3.
 * That way we don't link statically to the Vulkan library.
-*/
#include "volk.h"

/*--
 * We are using the Vulkan Memory Allocator (VMA) to manage memory.
 * This is a library that helps to allocate memory for Vulkan resources.
-*/


// Disable warnings in VMA
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#pragma warning(push)
#pragma warning(disable : 4100)  // Unreferenced formal parameter
#pragma warning(disable : 4189)  // Local variable is initialized but not referenced
#pragma warning(disable : 4127)  // Conditional expression is constant
#pragma warning(disable : 4324)  // Structure was padded due to alignment specifier
#pragma warning(disable : 4505)  // Unreferenced function with internal linkage has been removed
#include "vk_mem_alloc.h"
#pragma warning(pop)
#ifdef __clang__
#pragma clang diagnostic pop
#endif


// Return string for the Vulkan enum, like error messages
#include "vulkan/vk_enum_string_helper.h"

/*
/*--
 * GLFW is a library that provides a simple way to create windows and handle input.
-#1#
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#include <signal.h>  // For SIGINT
#endif
#include <GLFW/glfw3.h>
*/

/*--
 * GLM is a header-only library for mathematics.
 * It provides a simple way to handle vectors, matrices, and other mathematical objects.
 * We are using it to handle the vertex data and the transformation matrices.
-*/
#include <glm/glm.hpp>

// Logger
#include "logger.h"

// Debug utilities for naming Vulkan objects (DBG_VK_NAME)
#include "debug_util.h"

// Some graphic user interface (GUI) using Dear ImGui
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "imgui_internal.h"  // For Docking

// To load JPG/PNG images
#include "stb_image.h"

// Standard C++ includes
#include <array>
#include <cmath>          // For std::sin, ... functions
#include <filesystem>     // For std::filesystem::path ...
#include <iostream>       // For std::cerr
#include <span>           // For std::span
#include <unordered_map>  // For std::unordered_map
#include <vector>         // For std::vector

/*--
 * The shaders are compiled to Spir-V and embedded in the C++ code. (CMAKE)
-*/

#ifdef USE_SLANG
#include "_autogen/shader.comp.slang.h"
#include "_autogen/shader.rast.slang.h"
#else
#include "_autogen/shader.frag.glsl.h"
#include "_autogen/shader.vert.glsl.h"
#include "_autogen/shader.comp.glsl.h"
#endif

namespace shaderio
{
    // Shader IO namespace - use to share code between device and host
    using namespace glm; // Allow to use GLSL type, without glm:: prefix and without leaking in global namespace
#include "shaders/shader_io.h"
} // namespace shaderio

// Macro to either assert or throw based on the build type
#ifdef NDEBUG
#define ASSERT(condition, message)                                                                                     \
  do                                                                                                                   \
  {                                                                                                                    \
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
      throw std::runtime_error(message);                                                                               \
    }                                                                                                                  \
  } while(false)
#else
#define ASSERT(condition, message) assert((condition) && (message))
#endif

#include "slang/slang.h"
#include "slang/slang-com-helper.h"
#include "slang/slang-com-ptr.h"

#include <shaderc/shaderc.hpp>

struct CompileResult
{
    std::vector<uint32_t> spirvCode; // Filled on success
    std::string diagnostics; // Populated with diagnostics or errors, empty if none
    bool succeeded = false;
};

CompileResult compileSlangToSpirv(
    const std::string& moduleName,
    const std::string& filePath,
    const std::string& sourceCode,
    const std::string& entryPointName);

//--- Geometry -------------------------------------------------------------------------------------------------------------

/*--
 * Structure to hold the vertex data (see in shader_io.h), consisting only of a position, color and texture coordinates
 * Later we create a buffer with this data and use it to render a triangle.
-*/
struct Vertex : public shaderio::Vertex
{
    /*--
     * The binding description is used to describe at which rate to load data from memory throughout the vertices.
    -*/
    static auto getBindingDescription()
    {
        return std::to_array<VkVertexInputBindingDescription>(
            {{.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}});
    }

    /*--
     * The attribute descriptions describe how to extract a vertex attribute from
     * a chunk of vertex data originating from a binding description.
     * See in the vertex shader how the location is used to access the data.
    -*/
    static auto getAttributeDescriptions()
    {
        return std::to_array<VkVertexInputAttributeDescription>(
            {
                {
                    .location = shaderio::LVPosition, .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = uint32_t(offsetof(Vertex, position))
                },
                {
                    .location = shaderio::LVColor, .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = uint32_t(offsetof(Vertex, color))
                },
                {
                    .location = shaderio::LVTexCoord, .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = uint32_t(offsetof(Vertex, texCoord))
                }
            });
    }
};

// 2x3 vertices with a position, color and texCoords, make two CCW triangles
static const auto s_vertices = std::to_array<shaderio::Vertex>({
    {{0.0F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.5F, 0.5F}}, // Colored triangle
    {{-0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.5F, 0.5F}},
    {{0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {0.5F, 0.5F}},
    //
    {{0.1F, -0.4F, 0.75F}, {.3F, .3F, .3F}, {0.5F, 1.0F}}, // White triangle (textured)
    {{-0.4F, 0.6F, 0.25F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F}},
    {{0.6F, 0.6F, 0.75F}, {.7F, .7F, .7F}, {0.0F, 0.0F}},
});

// Indices for the two triangles
static const auto s_indices = std::to_array<uint32_t>({
    0, 1, 2, // First triangle (colored)
    3, 4, 5 // Second triangle (textured)
});

/*
{{-0.4F, -0.4F, 0.75F}, {0.3F, 0.3F, 0.3F}, {0.0F, 1.0F}}, // 3
{{-0.4F,  0.6F, 0.75F}, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F}}, // 4
{{ 0.6F,  0.6F, 0.75F}, {0.7F, 0.7F, 0.7F}, {1.0F, 0.0F}}, // 5
{{ 0.6F, -0.4F, 0.75F}, {0.7F, 0.7F, 0.7F}, {1.0F, 1.0F}}, // 6
*/

/*0, 1, 2, // First triangle (colored)
    3, 4, 5, // Second triangle (textured)
    3, 5, 6*/

// Points stored in a buffer and retrieved using buffer reference (flashing points)
static const auto s_points = std::to_array<glm::vec2>({{0.05F, 0.0F}, {-0.05F, 0.0F}, {0.0F, -0.05F}, {0.0F, 0.05F}});


//--- Vulkan Helpers ------------------------------------------------------------------------------------------------------------
#ifdef NDEBUG
#define VK_CHECK(vkFnc) vkFnc
#else
#define VK_CHECK(vkFnc)                                                                                                \
  {                                                                                                                    \
    if(const VkResult checkResult = (vkFnc); checkResult != VK_SUCCESS)                                                \
    {                                                                                                                  \
      const char* errMsg = string_VkResult(checkResult);                                                               \
      LOGE("Vulkan error: %s", errMsg);                                                                                \
      ASSERT(checkResult == VK_SUCCESS, errMsg);                                                                       \
    }                                                                                                                  \
  }
#endif

namespace utils
{
    /*--
     * A buffer is a region of memory used to store data.
     * It is used to store vertex data, index data, uniform data, and other types of data.
     * There is a VkBuffer object that represents the buffer, and a VmaAllocation object that represents the memory allocation.
     * The address is used to access the buffer in the shader.
    -*/
    struct Buffer
    {
        VkBuffer buffer{}; // Vulkan Buffer
        VmaAllocation allocation{}; // Memory associated with the buffer
        VkDeviceAddress address{}; // Address of the buffer in the shader
    };

    /*--
     * An image is a region of memory used to store image data.
     * It is used to store texture data, framebuffer data, and other types of data.
    -*/
    struct Image
    {
        VkImage image{}; // Vulkan Image
        VmaAllocation allocation{}; // Memory associated with the image
    };

    /*-- 
     * The image resource is an image with an image view and a layout.
     * and other information like format and extent.
    -*/
    struct ImageResource : Image
    {
        VkImageView view{}; // Image view
        VkExtent2D extent{}; // Size of the image
        VkImageLayout layout{}; // Layout of the image (color attachment, shader read, ...)
    };

    /*- Not implemented here -*/
    struct AccelerationStructure
    {
        VkAccelerationStructureKHR accel{};
        VmaAllocation allocation{};
        VkDeviceAddress deviceAddress{};
        VkDeviceSize size{};
        Buffer buffer; // Underlying buffer
    };

    /*--
     * A queue is a sequence of commands that are executed in order.
     * The queue is used to submit command buffers to the GPU.
     * The family index is used to identify the queue family (graphic, compute, transfer, ...) .
     * The queue index is used to identify the queue in the family, multiple queues can be in the same family.
    -*/
    struct QueueInfo
    {
        uint32_t familyIndex = ~0U; // Family index of the queue (graphic, compute, transfer, ...)
        uint32_t queueIndex = ~0U; // Index of the queue in the family
        VkQueue queue{}; // The queue object
    };

    /*-- 
     * Combines hash values using the FNV-1a based algorithm 
    -*/
    static std::size_t hashCombine(std::size_t seed, auto const& value)
    {
        return seed ^ (std::hash<std::decay_t<decltype(value)>>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    /*-- 
     * This returns the pipeline and access flags for a given layout, use for changing the image layout  
    -*/
    static std::tuple<VkPipelineStageFlags2, VkAccessFlags2> makePipelineStageAccessTuple(VkImageLayout state)
    {
        switch (state)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return std::make_tuple(VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE);
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return std::make_tuple(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return std::make_tuple(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
                                   | VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
                                   VK_ACCESS_2_SHADER_READ_BIT);
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return std::make_tuple(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return std::make_tuple(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        case VK_IMAGE_LAYOUT_GENERAL:
            return std::make_tuple(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                   VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT |
                                   VK_ACCESS_2_TRANSFER_WRITE_BIT);
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return std::make_tuple(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE);
        default:
            {
                ASSERT(false, "Unsupported layout transition!");
                return std::make_tuple(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                       VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT);
            }
        }
    };

    /*-- 
     * Return the barrier with the most common pair of stage and access flags for a given layout 
    -*/
    static VkImageMemoryBarrier2 createImageMemoryBarrier(VkImage image,
                                                          VkImageLayout oldLayout,
                                                          VkImageLayout newLayout,
                                                          VkImageSubresourceRange subresourceRange = {
                                                              VK_IMAGE_ASPECT_COLOR_BIT,
                                                              0, 1, 0, 1
                                                          })
    {
        const auto [srcStage, srcAccess] = makePipelineStageAccessTuple(oldLayout);
        const auto [dstStage, dstAccess] = makePipelineStageAccessTuple(newLayout);

        VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = srcStage,
            .srcAccessMask = srcAccess,
            .dstStageMask = dstStage,
            .dstAccessMask = dstAccess,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = subresourceRange
        };
        return barrier;
    }

    /*--
     * A helper function to transition an image from one layout to another.
     * In the pipeline, the image must be in the correct layout to be used, and this function is used to transition the image to the correct layout.
    -*/
    static void cmdTransitionImageLayout(VkCommandBuffer cmd,
                                         VkImage image,
                                         VkImageLayout oldLayout,
                                         VkImageLayout newLayout,
                                         VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
    {
        const VkImageMemoryBarrier2 barrier = createImageMemoryBarrier(image, oldLayout, newLayout,
                                                                       {aspectMask, 0, 1, 0, 1});
        const VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier
        };

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    /*-- 
     *  This helper returns the access mask for a given stage mask.
    -*/
    static VkAccessFlags2 inferAccessMaskFromStage(VkPipelineStageFlags2 stage, bool src)
    {
        VkAccessFlags2 access = 0;

        // Handle each possible stage bit
        if ((stage & VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT) != 0)
            access |= src ? VK_ACCESS_2_SHADER_READ_BIT : VK_ACCESS_2_SHADER_WRITE_BIT;
        if ((stage & VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT) != 0)
            access |= src ? VK_ACCESS_2_SHADER_READ_BIT : VK_ACCESS_2_SHADER_WRITE_BIT;
        if ((stage & VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT) != 0)
            access |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT; // Always read-only
        if ((stage & VK_PIPELINE_STAGE_2_TRANSFER_BIT) != 0)
            access |= src ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_TRANSFER_WRITE_BIT;
        ASSERT(access != 0, "Missing stage implementation");
        return access;
    }

    /*--
     * This useful function simplifies the addition of buffer barriers, by inferring 
     * the access masks from the stage masks, and adding the buffer barrier to the command buffer.
    -*/
    static void cmdBufferMemoryBarrier(VkCommandBuffer commandBuffer,
                                       VkBuffer buffer,
                                       VkPipelineStageFlags2 srcStageMask,
                                       VkPipelineStageFlags2 dstStageMask,
                                       VkAccessFlags2 srcAccessMask = 0, // Default to infer if not provided
                                       VkAccessFlags2 dstAccessMask = 0, // Default to infer if not provided
                                       VkDeviceSize offset = 0,
                                       VkDeviceSize size = VK_WHOLE_SIZE,
                                       uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                       uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED)
    {
        // Infer access masks if not explicitly provided
        if (srcAccessMask == 0)
        {
            srcAccessMask = inferAccessMaskFromStage(srcStageMask, true);
        }
        if (dstAccessMask == 0)
        {
            dstAccessMask = inferAccessMaskFromStage(dstStageMask, false);
        }

        const std::array<VkBufferMemoryBarrier2, 1> bufferBarrier{
            {
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = srcStageMask,
                    .srcAccessMask = srcAccessMask,
                    .dstStageMask = dstStageMask,
                    .dstAccessMask = dstAccessMask,
                    .srcQueueFamilyIndex = srcQueueFamilyIndex,
                    .dstQueueFamilyIndex = dstQueueFamilyIndex,
                    .buffer = buffer,
                    .offset = offset,
                    .size = size
                }
            }
        };

        const VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = uint32_t(bufferBarrier.size()),
            .pBufferMemoryBarriers = bufferBarrier.data()
        };
        vkCmdPipelineBarrier2(commandBuffer, &depInfo);
    }


    /*--
     * A helper function to find a supported format from a list of candidates.
     * For example, we can use this function to find a supported depth format.
    -*/
    static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
                                        const std::vector<VkFormat>& candidates,
                                        VkImageTiling tiling,
                                        VkFormatFeatureFlags2 features)
    {
        for (const VkFormat format : candidates)
        {
            VkFormatProperties2 props{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
            vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.formatProperties.linearTilingFeatures & features) ==
                features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.formatProperties.optimalTilingFeatures & features) ==
                features)
            {
                return format;
            }
        }
        ASSERT(false, "failed to find supported format!");
        return VK_FORMAT_UNDEFINED;
    }

    /*--
     * A helper function to find the depth format that is supported by the physical device.
    -*/
    static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
    {
        return findSupportedFormat(physicalDevice,
                                   {
                                       VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                       VK_FORMAT_D24_UNORM_S8_UINT
                                   },
                                   VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    /*--
     * A helper function to create a shader module from a Spir-V code.
    -*/
    static VkShaderModule createShaderModule(VkDevice device, const std::span<const uint32_t>& code)
    {
        const VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size() * sizeof(uint32_t),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };
        VkShaderModule shaderModule{};
        VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }

    //--- Command Buffer ------------------------------------------------------------------------------------------------------------

    /*-- Simple helper for the creation of a temporary command buffer, use to record the commands to upload data, or transition images. -*/
    static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool cmdPool)
    {
        const VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = cmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        VkCommandBuffer cmd{};
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &cmd));
        const VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
        return cmd;
    }

    /*-- 
     * Submit the temporary command buffer, wait until the command is finished, and clean up. 
     * This is a blocking function and should be used only for small operations 
    --*/
    static void endSingleTimeCommands(VkCommandBuffer cmd, VkDevice device, VkCommandPool cmdPool, VkQueue queue)
    {
        // Submit and clean up
        VK_CHECK(vkEndCommandBuffer(cmd));

        // Create fence for synchronization
        const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        std::array<VkFence, 1> fence{};
        VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, fence.data()));

        const VkCommandBufferSubmitInfo cmdBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cmd
        };
        const std::array<VkSubmitInfo2, 1> submitInfo{
            {
                {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1,
                    .pCommandBufferInfos = &cmdBufferInfo
                }
            }
        };
        VK_CHECK(vkQueueSubmit2(queue, uint32_t(submitInfo.size()), submitInfo.data(), fence[0]));
        VK_CHECK(vkWaitForFences(device, uint32_t(fence.size()), fence.data(), VK_TRUE, UINT64_MAX));

        // Cleanup
        vkDestroyFence(device, fence[0], nullptr);
        vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
    }

    // Helper to chain elements to the pNext
    template <typename MainT, typename NewT>
    static void pNextChainPushFront(MainT* mainStruct, NewT* newStruct)
    {
        newStruct->pNext = mainStruct->pNext;
        mainStruct->pNext = newStruct;
    }

    // Validation settings: to fine tune what is checked
    struct ValidationSettings
    {
        VkBool32 fine_grained_locking{VK_TRUE};
        VkBool32 validate_core{VK_TRUE};
        VkBool32 check_image_layout{VK_TRUE};
        VkBool32 check_command_buffer{VK_TRUE};
        VkBool32 check_object_in_use{VK_TRUE};
        VkBool32 check_query{VK_TRUE};
        VkBool32 check_shaders{VK_TRUE};
        VkBool32 check_shaders_caching{VK_TRUE};
        VkBool32 unique_handles{VK_TRUE};
        VkBool32 object_lifetime{VK_TRUE};
        VkBool32 stateless_param{VK_TRUE};
        std::vector<const char*> debug_action{"VK_DBG_LAYER_ACTION_LOG_MSG"};
        // "VK_DBG_LAYER_ACTION_DEBUG_OUTPUT", "VK_DBG_LAYER_ACTION_BREAK"
        std::vector<const char*> report_flags{"error"};

        VkBaseInStructure* buildPNextChain()
        {
            layerSettings = std::vector<VkLayerSettingEXT>{
                {layerName, "fine_grained_locking", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &fine_grained_locking},
                {layerName, "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &validate_core},
                {layerName, "check_image_layout", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &check_image_layout},
                {layerName, "check_command_buffer", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &check_command_buffer},
                {layerName, "check_object_in_use", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &check_object_in_use},
                {layerName, "check_query", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &check_query},
                {layerName, "check_shaders", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &check_shaders},
                {layerName, "check_shaders_caching", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &check_shaders_caching},
                {layerName, "unique_handles", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &unique_handles},
                {layerName, "object_lifetime", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &object_lifetime},
                {layerName, "stateless_param", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &stateless_param},
                {
                    layerName, "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT, uint32_t(debug_action.size()),
                    debug_action.data()
                },
                {
                    layerName, "report_flags", VK_LAYER_SETTING_TYPE_STRING_EXT, uint32_t(report_flags.size()),
                    report_flags.data()
                },

            };
            layerSettingsCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
                .settingCount = uint32_t(layerSettings.size()),
                .pSettings = layerSettings.data(),
            };

            return reinterpret_cast<VkBaseInStructure*>(&layerSettingsCreateInfo);
        }

        static constexpr const char* layerName{"VK_LAYER_KHRONOS_validation"};
        std::vector<VkLayerSettingEXT> layerSettings{};
        VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{};
    };

    //--- Vulkan Context ------------------------------------------------------------------------------------------------------------

    /*--
     * The context is the main class that holds the Vulkan instance, the physical device, the logical device, and the queue.
     * The instance is the main object that is used to interact with the Vulkan library.
     * The physical device is the GPU that is used to render the scene.
     * The logical device is the interface to the physical device.
     * The queue is used to submit command buffers to the GPU.
     *
     * This class will add default extensions and layers to the Vulkan instance, same as for the logical device.
     * It will also create a debug callback if the validation layers are enabled.
     * For more flexibility, a custom list of extensions and layers should be added.
    -*/
    class Context
    {
    public:
        Context() = default;
        ~Context() { assert(m_device == VK_NULL_HANDLE && "Missing destroy()"); }

        void init()
        {
            initInstance();
            selectPhysicalDevice();
            initLogicalDevice();
        }

        // Destroy internal resources and reset its initial state
        void deinit()
        {
            vkDeviceWaitIdle(m_device);
            if (m_enableValidationLayers && vkDestroyDebugUtilsMessengerEXT)
            {
                vkDestroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
            }
            vkDestroyDevice(m_device, nullptr);
            vkDestroyInstance(m_instance, nullptr);
            *this = {};
        }

        VkDevice getDevice() const { return m_device; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        VkInstance getInstance() const { return m_instance; }
        const QueueInfo& getGraphicsQueue() const { return m_queues[0]; }
        uint32_t getApiVersion() const { return m_apiVersion; }

        VkPhysicalDeviceFeatures2 getPhysicalDeviceFeatures() const { return m_deviceFeatures; }
        VkPhysicalDeviceVulkan11Features getVulkan11Features() const { return m_features11; }
        VkPhysicalDeviceVulkan12Features getVulkan12Features() const { return m_features12; }
        VkPhysicalDeviceVulkan13Features getVulkan13Features() const { return m_features13; }
        VkPhysicalDeviceVulkan14Features getVulkan14Features() const { return m_features14; }

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT getDynamicStateFeatures() const
        {
            return m_dynamicStateFeatures;
        }

        VkPhysicalDeviceExtendedDynamicState2FeaturesEXT getDynamicState2Features() const
        {
            return m_dynamicState2Features;
        }

        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT getDynamicState3Features() const
        {
            return m_dynamicState3Features;
        }

        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT getSwapchainFeatures() const { return m_swapchainFeatures; }

    private:
        //--- Vulkan Debug ------------------------------------------------------------------------------------------------------------

        /*-- Callback function to catch validation errors  -*/
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                            VkDebugUtilsMessageTypeFlagsEXT,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                            void*)
        {
            const Logger::LogLevel level =
                (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0
                    ? Logger::LogLevel::eERROR
                    : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0
                    ? Logger::LogLevel::eWARNING
                    : Logger::LogLevel::eINFO;
            Logger::getInstance().log(level, "%s", callbackData->pMessage);
            if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
            {
#if defined(_MSVC_LANG)
                __debugbreak();
#elif defined(__linux__)
      raise(SIGTRAP);
#endif
            }
            return VK_FALSE;
        }

        void initInstance()
        {
            vkEnumerateInstanceVersion(&m_apiVersion);
            LOGI("VULKAN API: %d.%d", VK_VERSION_MAJOR(m_apiVersion), VK_VERSION_MINOR(m_apiVersion));
            ASSERT(m_apiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0), "Require Vulkan 1.4 loader");

            /*// This finds the KHR surface extensions needed to display on the right platform
            uint32_t     glfwExtensionCount = 0;
            const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);*/
            uint32_t sdl3ExtensionCount = 0;
            char const* const* sdl3Extensions = SDL_Vulkan_GetInstanceExtensions(&sdl3ExtensionCount);
            getAvailableInstanceExtensions();

            const VkApplicationInfo applicationInfo{
                .pApplicationName = "minimal_latest",
                .applicationVersion = 1,
                .pEngineName = "minimal_latest",
                .engineVersion = 1,
                .apiVersion = m_apiVersion,
            };

            // Add extensions requested by GLFW
            //m_instanceExtensions.insert(m_instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
            m_instanceExtensions.insert(m_instanceExtensions.end(), sdl3Extensions,
                                        sdl3Extensions + sdl3ExtensionCount);
            if (extensionIsAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_instanceExtensionsAvailable))
                m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Allow debug utils (naming objects)
            if (extensionIsAvailable(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, m_instanceExtensionsAvailable))
                m_instanceExtensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);

            // Adding the validation layer
            if (m_enableValidationLayers)
            {
                m_instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
            }

            // Setting for the validation layer
            ValidationSettings validationSettings{.validate_core = VK_TRUE}; // modify default value

            const VkInstanceCreateInfo instanceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = validationSettings.buildPNextChain(),
                .pApplicationInfo = &applicationInfo,
                .enabledLayerCount = uint32_t(m_instanceLayers.size()),
                .ppEnabledLayerNames = m_instanceLayers.data(),
                .enabledExtensionCount = uint32_t(m_instanceExtensions.size()),
                .ppEnabledExtensionNames = m_instanceExtensions.data(),
            };

            // Actual Vulkan instance creation
            VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

            // Load all Vulkan functions
            volkLoadInstance(m_instance);

            // Add the debug callback
            if (m_enableValidationLayers && vkCreateDebugUtilsMessengerEXT)
            {
                const VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                    .pfnUserCallback = Context::debugCallback, // <-- The callback function
                };
                VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_instance, &dbg_messenger_create_info, nullptr, &m_callback));
                LOGI("Validation Layers: ON");
            }
        }

        /*--
         * The physical device is the GPU that is used to render the scene.
         * We are selecting the first discrete GPU found, if there is one.
        -*/
        void selectPhysicalDevice()
        {
            size_t chosenDevice = 0;

            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
            ASSERT(deviceCount != 0, "failed to find GPUs with Vulkan support!");

            std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
            vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

            VkPhysicalDeviceProperties2 properties2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            for (size_t i = 0; i < physicalDevices.size(); i++)
            {
                vkGetPhysicalDeviceProperties2(physicalDevices[i], &properties2);
                if (properties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    chosenDevice = i;
                    break;
                }
            }

            m_physicalDevice = physicalDevices[chosenDevice];
            vkGetPhysicalDeviceProperties2(m_physicalDevice, &properties2);
            LOGI("Selected GPU: %s", properties2.properties.deviceName); // Show the name of the GPU
            LOGI("Driver: %d.%d.%d", VK_VERSION_MAJOR(properties2.properties.driverVersion),
                 VK_VERSION_MINOR(properties2.properties.driverVersion),
                 VK_VERSION_PATCH(properties2.properties.driverVersion));
            LOGI("Vulkan API: %d.%d.%d", VK_VERSION_MAJOR(properties2.properties.apiVersion),
                 VK_VERSION_MINOR(properties2.properties.apiVersion),
                 VK_VERSION_PATCH(properties2.properties.apiVersion));
        }

        /*--
         * The queue is used to submit command buffers to the GPU.
         * We are selecting the first queue found (graphic), which is the most common and needed for rendering graphic elements.
         * 
         * Other types of queues are used for compute, transfer, and other types of operations.
         * In a more advanced application, the user should select the queue that fits the application needs.
         * 
         * Eventually the user should create multiple queues for different types of operations.
         * 
         * Note: The queue is created with the creation of the logical device, this is the selection which are requested when creating the logical device.
         * Note: the search of the queue could be more advanced, and search for the right queue family.
        -*/
        QueueInfo getQueue(VkQueueFlagBits flags) const
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamilyCount, {
                                                                    .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2
                                                                });
            vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

            QueueInfo queueInfo;
            for (uint32_t i = 0; i < queueFamilies.size(); i++)
            {
                if (queueFamilies[i].queueFamilyProperties.queueFlags & flags)
                {
                    queueInfo.familyIndex = i;
                    queueInfo.queueIndex = 0;
                    // A second graphic queue could be index 1 (need logic to find the right one)
                    // m_queueInfo.queue = After creating the logical device
                    break;
                }
            }
            return queueInfo;
        }

        /*--
         * The logical device is the interface to the physical device.
         * It is used to create resources, allocate memory, and submit command buffers to the GPU.
         * The logical device is created with the physical device and the queue family that is used.
         * The logical device is created with the extensions and features that are needed.
         * Note: the feature structure is used to add all features up to Vulkan 1.4, but it can be used to add specific features.
         *       This class does not add any specific feature, or extension, but it can be added by the user.
        -*/
        void initLogicalDevice()
        {
            const float queuePriority = 1.0F;
            m_queues.clear();
            m_queues.emplace_back(getQueue(VK_QUEUE_GRAPHICS_BIT));

            // Request only one queue : graphic
            // User could request more specific queues: compute, transfer
            const VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = m_queues[0].familyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            };

            // Chaining all features up to Vulkan 1.4
            pNextChainPushFront(&m_features11, &m_features12);
            pNextChainPushFront(&m_features11, &m_features13);
            pNextChainPushFront(&m_features11, &m_features14);

            /*-- 
             * Check if the device supports the required extensions 
             * Because we cannot request a device with extension it is not supporting
            -*/
            getAvailableDeviceExtensions();
            if (extensionIsAvailable(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, m_deviceExtensionsAvailable))
            {
                m_deviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            }
            if (extensionIsAvailable(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, m_deviceExtensionsAvailable))
            {
                pNextChainPushFront(&m_features11, &m_dynamicStateFeatures);
                m_deviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
            }
            if (extensionIsAvailable(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME, m_deviceExtensionsAvailable))
            {
                pNextChainPushFront(&m_features11, &m_dynamicState2Features);
                m_deviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
            }
            if (extensionIsAvailable(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME, m_deviceExtensionsAvailable))
            {
                pNextChainPushFront(&m_features11, &m_dynamicState3Features);
                m_deviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
            }
            if (extensionIsAvailable(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME, m_deviceExtensionsAvailable))
            {
                pNextChainPushFront(&m_features11, &m_swapchainFeatures);
                m_deviceExtensions.push_back(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
            }

            // ImGui - Fix (Not using Vulkan 1.4 API)
            m_deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

            // Requesting all supported features, which will then be activated in the device
            // By requesting, it turns on all feature that it is supported, but the user could request specific features instead
            m_deviceFeatures.pNext = &m_features11;
            vkGetPhysicalDeviceFeatures2(m_physicalDevice, &m_deviceFeatures);

            ASSERT(m_features13.dynamicRendering, "Dynamic rendering required, update driver!");
            ASSERT(m_features13.maintenance4, "Extension VK_KHR_maintenance4 required, update driver!");
            // vkGetDeviceBufferMemoryRequirementsKHR, ...
            ASSERT(m_features14.maintenance5, "Extension VK_KHR_maintenance5 required, update driver!");
            // VkBufferUsageFlags2KHR, ...
            ASSERT(m_features14.maintenance6, "Extension VK_KHR_maintenance6 required, update driver!");
            // vkCmdPushConstants2KHR, vkCmdBindDescriptorSets2KHR

            // Get information about what the device can do
            VkPhysicalDeviceProperties2 deviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            deviceProperties.pNext = &m_pushDescriptorProperties;
            vkGetPhysicalDeviceProperties2(m_physicalDevice, &deviceProperties);

            // Create the logical device
            const VkDeviceCreateInfo deviceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &m_deviceFeatures,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledExtensionCount = uint32_t(m_deviceExtensions.size()),
                .ppEnabledExtensionNames = m_deviceExtensions.data(),
            };
            VK_CHECK(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));
            DBG_VK_NAME(m_device);

            volkLoadDevice(m_device); // Load all Vulkan device functions

            // Debug utility to name Vulkan objects, great in debugger like NSight
            debugUtilInitialize(m_device);

            // Get the requested queues
            vkGetDeviceQueue(m_device, m_queues[0].familyIndex, m_queues[0].queueIndex, &m_queues[0].queue);
            DBG_VK_NAME(m_queues[0].queue);

            // Log the enabled extensions
            LOGI("Enabled device extensions:");
            for (const auto& ext : m_deviceExtensions)
            {
                LOGI("  %s", ext);
            }
        }

        /*-- 
         * Get all available extensions for the device, because we cannot request an extension that isn't 
         * supported/available. If we do, the logical device creation would fail. 
        -*/
        void getAvailableDeviceExtensions()
        {
            uint32_t count{0};
            VK_CHECK(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, nullptr));
            m_deviceExtensionsAvailable.resize(count);
            VK_CHECK(
                vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, m_deviceExtensionsAvailable.data
                    ()));
        }

        void getAvailableInstanceExtensions()
        {
            uint32_t count{0};
            vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
            m_instanceExtensionsAvailable.resize(count);
            vkEnumerateInstanceExtensionProperties(nullptr, &count, m_instanceExtensionsAvailable.data());
        }

        // Work in conjunction with the above
        bool extensionIsAvailable(const std::string& name, const std::vector<VkExtensionProperties>& extensions)
        {
            for (auto& ext : extensions)
            {
                if (name == ext.extensionName)
                    return true;
            }
            return false;
        }


        // --- Members ------------------------------------------------------------------------------------------------------------
        uint32_t m_apiVersion{0}; // The Vulkan API version

        // Instance extension, extra extensions can be added here
        std::vector<const char*> m_instanceExtensions = {VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME};
        std::vector<const char*> m_instanceLayers = {}; // Add extra layers here

        // Device features, extra features can be added here
        VkPhysicalDeviceFeatures2 m_deviceFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features m_features11{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features m_features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features m_features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceVulkan14Features m_features14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};
        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT m_swapchainFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT
        };
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT m_dynamicStateFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT
        };
        VkPhysicalDeviceExtendedDynamicState2FeaturesEXT m_dynamicState2Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT
        };
        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT m_dynamicState3Features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT
        };

        // Properties: how much a feature can do
        VkPhysicalDevicePushDescriptorPropertiesKHR m_pushDescriptorProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR
        };

        std::vector<VkBaseOutStructure*> m_linkedDeviceProperties{
            reinterpret_cast<VkBaseOutStructure*>(&m_pushDescriptorProperties)
        };


        // Device extension, extra extensions can be added here
        std::vector<const char*> m_deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, // Needed for display on the screen
        };

        VkInstance m_instance{}; // The Vulkan instance
        VkPhysicalDevice m_physicalDevice{}; // The physical device (GPU)
        VkDevice m_device{}; // The logical device (interface to the physical device)
        std::vector<QueueInfo> m_queues{}; // The queue used to submit command buffers to the GPU
        VkDebugUtilsMessengerEXT m_callback{VK_NULL_HANDLE}; // The debug callback
        std::vector<VkExtensionProperties> m_instanceExtensionsAvailable{};
        std::vector<VkExtensionProperties> m_deviceExtensionsAvailable{};
#ifdef NDEBUG
  bool m_enableValidationLayers = false;
#else
        bool m_enableValidationLayers = true;
#endif
    };

    //--- Swapchain ------------------------------------------------------------------------------------------------------------
    /*--
     * Swapchain: The swapchain is responsible for presenting rendered images to the screen.
     * It consists of multiple images (frames) that are cycled through for rendering and display.
     * The swapchain is created with a surface and optional vSync setting, with the
     * window size determined during its setup.
     * "Frames in flight" refers to the number of images being processed concurrently (e.g., double buffering = 2, triple buffering = 3).
     * vSync enabled (FIFO mode) uses double buffering, while disabling vSync  (MAILBOX mode) uses triple buffering.
     *
     * The "current frame" is the frame currently being processed.
     * The "next image index" points to the swapchain image that will be rendered next, which might differ from the current frame's index.
     * If the window is resized or certain conditions are met, the swapchain needs to be recreated (`needRebuild` flag).
    -*/
    class Swapchain
    {
    public:
        Swapchain() = default;
        ~Swapchain() { assert(m_swapChain == VK_NULL_HANDLE && "Missing deinit()"); }

        void needToRebuild() { m_needRebuild = true; }
        bool needRebuilding() const { return m_needRebuild; }
        VkImage getNextImage() const { return m_nextImages[m_nextImageIndex].image; }
        VkImageView getNextImageView() const { return m_nextImages[m_nextImageIndex].imageView; }
        VkFormat getImageFormat() const { return m_imageFormat; }
        uint32_t getMaxFramesInFlight() const { return m_maxFramesInFlight; }
        VkSemaphore getWaitSemaphores() const { return m_frameResources[m_currentFrame].imageAvailableSemaphore; }
        VkSemaphore getSignalSemaphores() const { return m_frameResources[m_nextImageIndex].renderFinishedSemaphore; }

        // Initialize the swapchain with the provided context and surface, then we can create and re-create it
        void init(VkPhysicalDevice physicalDevice, VkDevice device, const QueueInfo& queue, VkSurfaceKHR surface,
                  VkCommandPool cmdPool)
        {
            m_physicalDevice = physicalDevice;
            m_device = device;
            m_queue = queue;
            m_surface = surface;
            m_cmdPool = cmdPool;
        }

        // Destroy internal resources and reset its initial state
        void deinit()
        {
            deinitResources();
            *this = {};
        }

        /*--
         * Create the swapchain using the provided context, surface, and vSync option. The actual window size is returned.
         * Queries the GPU capabilities, selects the best surface format and present mode, and creates the swapchain accordingly.
        -*/
        VkExtent2D initResources(bool vSync = true)
        {
            VkExtent2D outWindowSize;

            // Query the physical device's capabilities for the given surface.
            const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo2{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                .surface = m_surface
            };
            VkSurfaceCapabilities2KHR capabilities2{.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
            vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_physicalDevice, &surfaceInfo2, &capabilities2);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormats2KHR(m_physicalDevice, &surfaceInfo2, &formatCount, nullptr);
            std::vector<VkSurfaceFormat2KHR> formats(formatCount, {.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR});
            vkGetPhysicalDeviceSurfaceFormats2KHR(m_physicalDevice, &surfaceInfo2, &formatCount, formats.data());

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount,
                                                      presentModes.data());

            // Choose the best available surface format and present mode
            const VkSurfaceFormat2KHR surfaceFormat2 = selectSwapSurfaceFormat(formats);
            const VkPresentModeKHR presentMode = selectSwapPresentMode(presentModes, vSync);
            // Set the window size according to the surface's current extent
            outWindowSize = capabilities2.surfaceCapabilities.currentExtent;

            // Adjust the number of images in flight within GPU limitations
            uint32_t minImageCount = capabilities2.surfaceCapabilities.minImageCount; // Vulkan-defined minimum
            uint32_t preferredImageCount = std::max(3u, minImageCount); // Prefer 3, but respect minImageCount

            // Handle the maxImageCount case where 0 means "no upper limit"
            uint32_t maxImageCount = (capabilities2.surfaceCapabilities.maxImageCount == 0)
                                         ? preferredImageCount
                                         : // No upper limit, use preferred
                                         capabilities2.surfaceCapabilities.maxImageCount;

            // Clamp preferredImageCount to valid range [minImageCount, maxImageCount]
            m_maxFramesInFlight = std::clamp(preferredImageCount, minImageCount, maxImageCount);

            // Store the chosen image format
            m_imageFormat = surfaceFormat2.surfaceFormat.format;

            // Create the swapchain itself
            const VkSwapchainCreateInfoKHR swapchainCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = m_surface,
                .minImageCount = m_maxFramesInFlight,
                .imageFormat = surfaceFormat2.surfaceFormat.format,
                .imageColorSpace = surfaceFormat2.surfaceFormat.colorSpace,
                .imageExtent = capabilities2.surfaceCapabilities.currentExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .preTransform = capabilities2.surfaceCapabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = presentMode,
                .clipped = VK_TRUE,
            };
            VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapChain));
            DBG_VK_NAME(m_swapChain);

            // Retrieve the swapchain images
            {
                uint32_t imageCount;
                vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
                ASSERT(m_maxFramesInFlight <= imageCount, "Wrong swapchain setup");
                m_maxFramesInFlight = imageCount; // Use the number of images in the swapchain
            }
            std::vector<VkImage> swapImages(m_maxFramesInFlight);
            vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_maxFramesInFlight, swapImages.data());

            // Store the swapchain images and create views for them
            m_nextImages.resize(m_maxFramesInFlight);
            VkImageViewCreateInfo imageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_imageFormat,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0,
                    .layerCount = 1
                },
            };
            for (uint32_t i = 0; i < m_maxFramesInFlight; i++)
            {
                m_nextImages[i].image = swapImages[i];
                DBG_VK_NAME(m_nextImages[i].image);
                imageViewCreateInfo.image = m_nextImages[i].image;
                VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_nextImages[i].imageView));
                DBG_VK_NAME(m_nextImages[i].imageView);
            }

            // Initialize frame resources for each frame
            m_frameResources.resize(m_maxFramesInFlight);
            for (size_t i = 0; i < m_maxFramesInFlight; ++i)
            {
                /*--
                 * The sync objects are used to synchronize the rendering with the presentation.
                 * The image available semaphore is signaled when the image is available to render.
                 * The render finished semaphore is signaled when the rendering is finished.
                 * The in flight fence is signaled when the frame is in flight.
                -*/
                const VkSemaphoreCreateInfo semaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
                VK_CHECK(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frameResources[i].
                        imageAvailableSemaphore));
                DBG_VK_NAME(m_frameResources[i].imageAvailableSemaphore);
                VK_CHECK(
                    vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frameResources[i].
                        renderFinishedSemaphore));
                DBG_VK_NAME(m_frameResources[i].renderFinishedSemaphore);
            }

            // Transition images to present layout
            {
                VkCommandBuffer cmd = utils::beginSingleTimeCommands(m_device, m_cmdPool);
                for (uint32_t i = 0; i < m_maxFramesInFlight; i++)
                {
                    cmdTransitionImageLayout(cmd, m_nextImages[i].image, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
                }
                utils::endSingleTimeCommands(cmd, m_device, m_cmdPool, m_queue.queue);
            }

            return outWindowSize;
        }

        /*--
         * Recreate the swapchain, typically after a window resize or when it becomes invalid.
         * This waits for all rendering to be finished before destroying the old swapchain and creating a new one.
        -*/
        VkExtent2D reinitResources(bool vSync = true)
        {
            // Wait for all frames to finish rendering before recreating the swapchain
            vkQueueWaitIdle(m_queue.queue);

            m_currentFrame = 0;
            m_needRebuild = false;
            deinitResources();
            return initResources(vSync);
        }

        /*--
         * Destroy the swapchain and its associated resources.
         * This function is also called when the swapchain needs to be recreated.
        -*/
        void deinitResources()
        {
            vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
            for (auto& frameRes : m_frameResources)
            {
                vkDestroySemaphore(m_device, frameRes.imageAvailableSemaphore, nullptr);
                vkDestroySemaphore(m_device, frameRes.renderFinishedSemaphore, nullptr);
            }
            for (auto& image : m_nextImages)
            {
                vkDestroyImageView(m_device, image.imageView, nullptr);
            }
        }

        /*--
         * Prepares the command buffer for recording rendering commands.
         * This function handles synchronization with the previous frame and acquires the next image from the swapchain.
         * The command buffer is reset, ready for new rendering commands.
        -*/
        VkResult acquireNextImage(VkDevice device)
        {
            ASSERT(m_needRebuild == false, "Swapbuffer need to call reinitResources()");

            auto& frame = m_frameResources[m_currentFrame];

            // Acquire the next image from the swapchain
            const VkResult result = vkAcquireNextImageKHR(device, m_swapChain, std::numeric_limits<uint64_t>::max(),
                                                          frame.imageAvailableSemaphore, VK_NULL_HANDLE,
                                                          &m_nextImageIndex);
            // Handle special case if the swapchain is out of date (e.g., window resize)
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                m_needRebuild = true; // Swapchain must be rebuilt on the next frame
            }
            else
            {
                ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Couldn't aquire swapchain image");
            }
            return result;
        }

        /*--
         * Presents the rendered image to the screen.
         * The semaphore ensures that the image is presented only after rendering is complete.
         * Advances to the next frame in the cycle.
        -*/
        void presentFrame(VkQueue queue)
        {
            auto& frame = m_frameResources[m_nextImageIndex];

            // Setup the presentation info, linking the swapchain and the image index
            const VkPresentInfoKHR presentInfo{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1, // Wait for rendering to finish
                .pWaitSemaphores = &frame.renderFinishedSemaphore, // Synchronize presentation
                .swapchainCount = 1, // Swapchain to present the image
                .pSwapchains = &m_swapChain, // Pointer to the swapchain
                .pImageIndices = &m_nextImageIndex, // Index of the image to present
            };

            // Present the image and handle potential resizing issues
            const VkResult result = vkQueuePresentKHR(queue, &presentInfo);
            // If the swapchain is out of date (e.g., window resized), it needs to be rebuilt
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                m_needRebuild = true;
            }
            else
            {
                ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Couldn't present swapchain image");
            }

            // Advance to the next frame in the swapchain
            m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
        }

    private:
        // Represents an image within the swapchain that can be rendered to.
        struct Image
        {
            VkImage image{}; // Image to render to
            VkImageView imageView{}; // Image view to access the image
        };

        /*--
         * Resources associated with each frame being processed.
         * Each frame has its own set of resources, mainly synchronization primitives
        -*/
        struct FrameResources
        {
            VkSemaphore imageAvailableSemaphore{}; // Signals when the image is ready for rendering
            VkSemaphore renderFinishedSemaphore{}; // Signals when rendering is finished
        };

        // We choose the format that is the most common, and that is supported by* the physical device.
        VkSurfaceFormat2KHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& availableFormats) const
        {
            // If there's only one available format and it's undefined, return a default format.
            if (availableFormats.size() == 1 && availableFormats[0].surfaceFormat.format == VK_FORMAT_UNDEFINED)
            {
                VkSurfaceFormat2KHR result{
                    .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                    .surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                };
                return result;
            }

            const auto preferredFormats = std::to_array<VkSurfaceFormat2KHR>({
                {
                    .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                    .surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                },
                {
                    .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                    .surfaceFormat = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                },
            });

            // Check available formats against the preferred formats.
            for (const auto& preferredFormat : preferredFormats)
            {
                for (const auto& availableFormat : availableFormats)
                {
                    if (availableFormat.surfaceFormat.format == preferredFormat.surfaceFormat.format
                        && availableFormat.surfaceFormat.colorSpace == preferredFormat.surfaceFormat.colorSpace)
                    {
                        return availableFormat; // Return the first matching preferred format.
                    }
                }
            }

            // If none of the preferred formats are available, return the first available format.
            return availableFormats[0];
        }

        /*--
         * The present mode is chosen based on the vSync option
         * The FIFO mode is the most common, and is used when vSync is enabled.
         * The MAILBOX mode is used when vSync is disabled, and is the best mode for triple buffering.
         * The IMMEDIATE mode is used when vSync is disabled, and is the best mode for low latency.
        -*/
        VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                               bool vSync = true)
        {
            if (vSync)
            {
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            bool mailboxSupported = false, immediateSupported = false;

            for (VkPresentModeKHR mode : availablePresentModes)
            {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                    mailboxSupported = true;
                if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    immediateSupported = true;
            }

            if (mailboxSupported)
            {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }

            if (immediateSupported)
            {
                return VK_PRESENT_MODE_IMMEDIATE_KHR; // Best mode for low latency
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

    private:
        VkPhysicalDevice m_physicalDevice{}; // The physical device (GPU)
        VkDevice m_device{}; // The logical device (interface to the physical device)
        QueueInfo m_queue{}; // The queue used to submit command buffers to the GPU
        VkSwapchainKHR m_swapChain{}; // The swapchain
        VkFormat m_imageFormat{}; // The format of the swapchain images
        VkSurfaceKHR m_surface{}; // The surface to present images to
        VkCommandPool m_cmdPool{}; // The command pool for the swapchain

        std::vector<Image> m_nextImages{};
        std::vector<FrameResources> m_frameResources{};
        uint32_t m_currentFrame = 0;
        uint32_t m_nextImageIndex = 0;
        bool m_needRebuild = false;

        uint32_t m_maxFramesInFlight = 3; // Best for pretty much all cases
    };

    //--- Resource Allocator ------------------------------------------------------------------------------------------------------------
    /*--
     * Vulkan Memory Allocator (VMA) is a library that helps to manage memory in Vulkan.
     * This should be used to manage the memory of the resources instead of using the Vulkan API directly.
    -*/
    class ResourceAllocator
    {
    public:
        ResourceAllocator() = default;
        ~ResourceAllocator() { assert(m_allocator == nullptr && "Missing deinit()"); }
        operator VmaAllocator() const { return m_allocator; }

        // Initialization of VMA allocator.
        void init(VmaAllocatorCreateInfo allocatorInfo)
        {
            // #TODO : VK_EXT_memory_priority ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT

            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            // allow querying for the GPU address of a buffer
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;
            // allow using VkBufferUsageFlags2CreateInfoKHR

            m_device = allocatorInfo.device;
            // Because we use VMA_DYNAMIC_VULKAN_FUNCTIONS
            const VmaVulkanFunctions functions = {
                .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
            };
            allocatorInfo.pVulkanFunctions = &functions;
            vmaCreateAllocator(&allocatorInfo, &m_allocator);
        }

        // De-initialization of VMA allocator.
        void deinit()
        {
            if (!m_stagingBuffers.empty())
                LOGW("Warning: Staging buffers were not freed before destroying the allocator");
            freeStagingBuffers();
            vmaDestroyAllocator(m_allocator);
            *this = {};
        }

        /*-- Create a buffer -*/
        /* 
         * UBO: VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
         *        + VMA_MEMORY_USAGE_CPU_TO_GPU
         * SSBO: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
         *        + VMA_MEMORY_USAGE_CPU_TO_GPU // Use this if the CPU will frequently update the buffer
         *        + VMA_MEMORY_USAGE_GPU_ONLY // Use this if the CPU will rarely update the buffer
         *        + VMA_MEMORY_USAGE_GPU_TO_CPU  // Use this when you need to read back data from the SSBO to the CPU
         *      ----
         *        + VMA_ALLOCATION_CREATE_MAPPED_BIT // Automatically maps the buffer upon creation
         *        + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT // If the CPU will sequentially write to the buffer's memory,
         */
        Buffer createBuffer(VkDeviceSize size,
                            VkBufferUsageFlags2KHR usage,
                            VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO,
                            VmaAllocationCreateFlags flags = {}) const
        {
            // This can be used only with maintenance5
            const VkBufferUsageFlags2CreateInfoKHR bufferUsageFlags2CreateInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO_KHR,
                .usage = usage | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR,
            };

            const VkBufferCreateInfo bufferInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = &bufferUsageFlags2CreateInfo,
                .size = size,
                .usage = 0,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // Only one queue family will access i
            };

            VmaAllocationCreateInfo allocInfo = {.flags = flags, .usage = memoryUsage};
            const VkDeviceSize dedicatedMemoryMinSize = 64ULL * 1024; // 64 KB
            if (size > dedicatedMemoryMinSize)
            {
                allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // Use dedicated memory for large buffers
            }

            // Create the buffer
            Buffer resultBuffer;
            VmaAllocationInfo allocInfoOut{};
            VK_CHECK(
                vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &resultBuffer.buffer, &resultBuffer.allocation, &
                    allocInfoOut));

            // Get the GPU address of the buffer
            const VkBufferDeviceAddressInfo info = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = resultBuffer.buffer
            };
            resultBuffer.address = vkGetBufferDeviceAddress(m_device, &info);

            {
                // Find leaks
                static uint32_t counter = 0U;
                if (m_leakID == counter)
                {
#if defined(_MSVC_LANG)
                    __debugbreak();
#endif
                }
                std::string allocID = std::string("allocID: ") + std::to_string(counter++);
                vmaSetAllocationName(m_allocator, resultBuffer.allocation, allocID.c_str());
            }

            return resultBuffer;
        }

        //*-- Destroy a buffer -*/
        void destroyBuffer(Buffer buffer) const { vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation); }

        /*--
         * Create a staging buffer, copy data into it, and track it.
         * This method accepts data, handles the mapping, copying, and unmapping
         * automatically.
        -*/
        template <typename T>
        Buffer createStagingBuffer(const std::span<T>& vectorData)
        {
            const VkDeviceSize bufferSize = sizeof(T) * vectorData.size();

            // Create a staging buffer
            Buffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR,
                                                VMA_MEMORY_USAGE_CPU_TO_GPU,
                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

            // Track the staging buffer for later cleanup
            m_stagingBuffers.push_back(stagingBuffer);

            // Map and copy data to the staging buffer
            void* data;
            vmaMapMemory(m_allocator, stagingBuffer.allocation, &data);
            memcpy(data, vectorData.data(), (size_t)bufferSize);
            vmaUnmapMemory(m_allocator, stagingBuffer.allocation);
            return stagingBuffer;
        }

        /*--
         * Create a buffer (GPU only) with data, this is done using a staging buffer
         * The staging buffer is a buffer that is used to transfer data from the CPU
         * to the GPU.
         * and cannot be freed until the data is transferred. So the command buffer
         * must be submitted, then
         * the staging buffer can be cleared using the freeStagingBuffers function.
        -*/
        template <typename T>
        Buffer createBufferAndUploadData(VkCommandBuffer cmd, const std::span<T>& vectorData,
                                         VkBufferUsageFlags2KHR usageFlags)
        {
            // Create staging buffer and upload data
            Buffer stagingBuffer = createStagingBuffer(vectorData);

            // Create the final buffer in GPU memory
            const VkDeviceSize bufferSize = sizeof(T) * vectorData.size();
            Buffer buffer = createBuffer(bufferSize, usageFlags | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR,
                                         VMA_MEMORY_USAGE_GPU_ONLY);

            const std::array<VkBufferCopy, 1> copyRegion{{{.size = bufferSize}}};
            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer.buffer, uint32_t(copyRegion.size()), copyRegion.data());

            return buffer;
        }

        /*--
         * Create an image in GPU memory. This does not adding data to the image.
         * This is only creating the image in GPU memory.
         * See createImageAndUploadData for creating an image and uploading data.
        -*/
        Image createImage(const VkImageCreateInfo& imageInfo) const
        {
            const VmaAllocationCreateInfo createInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY};

            Image image;
            VmaAllocationInfo allocInfo{};
            VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &createInfo, &image.image, &image.allocation, &allocInfo));
            return image;
        }

        /*-- Destroy image --*/
        void destroyImage(Image& image) const { vmaDestroyImage(m_allocator, image.image, image.allocation); }

        void destroyImageResource(ImageResource& imageRessource) const
        {
            destroyImage(imageRessource);
            vkDestroyImageView(m_device, imageRessource.view, nullptr);
        }

        /*-- Create an image and upload data using a staging buffer --*/
        template <typename T>
        ImageResource createImageAndUploadData(VkCommandBuffer cmd, const std::span<T>& vectorData,
                                               const VkImageCreateInfo& _imageInfo, VkImageLayout finalLayout)
        {
            // Create staging buffer and upload data
            Buffer stagingBuffer = createStagingBuffer(vectorData);

            // Create image in GPU memory
            VkImageCreateInfo imageInfo = _imageInfo;
            imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // We will copy data to this image
            Image image = createImage(imageInfo);

            // Transition image layout for copying data
            cmdTransitionImageLayout(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy buffer data to the image
            const std::array<VkBufferImageCopy, 1> copyRegion{
                {
                    {
                        .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                        .imageExtent = imageInfo.extent
                    }
                }
            };

            vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   uint32_t(copyRegion.size()), copyRegion.data());

            // Transition image layout to final layout
            cmdTransitionImageLayout(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);

            ImageResource resultImage(image);
            resultImage.layout = finalLayout;
            return resultImage;
        }

        /*--
         * The staging buffers are buffers that are used to transfer data from the CPU to the GPU.
         * They cannot be freed until the data is transferred. So the command buffer must be completed, then the staging buffer can be cleared.
        -*/
        void freeStagingBuffers()
        {
            for (const auto& buffer : m_stagingBuffers)
            {
                destroyBuffer(buffer);
            }
            m_stagingBuffers.clear();
        }

        /*-- When leak are reported, set the ID of the leak here --*/
        void setLeakID(uint32_t id) { m_leakID = id; }

    private:
        VmaAllocator m_allocator{};
        VkDevice m_device{};
        std::vector<Buffer> m_stagingBuffers{};
        uint32_t m_leakID = ~0U;
    };

    /*--
     * Samplers are limited in Vulkan.
     * This class is used to create and store samplers, and to avoid creating the same sampler multiple times.
    -*/
    class SamplerPool
    {
    public:
        SamplerPool() = default;
        ~SamplerPool() { assert(m_device == VK_NULL_HANDLE && "Missing deinit()"); }
        // Initialize the sampler pool with the device reference, then we can later acquire samplers
        void init(VkDevice device) { m_device = device; }
        // Destroy internal resources and reset its initial state
        void deinit()
        {
            for (const auto& entry : m_samplerMap)
            {
                vkDestroySampler(m_device, entry.second, nullptr);
            }
            m_samplerMap.clear();
            *this = {};
        }

        // Get or create VkSampler based on VkSamplerCreateInfo
        VkSampler acquireSampler(const VkSamplerCreateInfo& createInfo)
        {
            if (auto it = m_samplerMap.find(createInfo); it != m_samplerMap.end())
            {
                // If found, return existing sampler
                return it->second;
            }

            // Otherwise, create a new sampler
            VkSampler newSampler = createSampler(createInfo);
            m_samplerMap[createInfo] = newSampler;
            return newSampler;
        }

        void releaseSampler(VkSampler sampler)
        {
            for (auto it = m_samplerMap.begin(); it != m_samplerMap.end();)
            {
                if (it->second == sampler)
                {
                    vkDestroySampler(m_device, it->second, nullptr);
                    it = m_samplerMap.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

    private:
        VkDevice m_device{};

        struct SamplerCreateInfoHash
        {
            std::size_t operator()(const VkSamplerCreateInfo& info) const
            {
                std::size_t seed{0};
                seed = hashCombine(seed, info.magFilter);
                seed = hashCombine(seed, info.minFilter);
                seed = hashCombine(seed, info.mipmapMode);
                seed = hashCombine(seed, info.addressModeU);
                seed = hashCombine(seed, info.addressModeV);
                seed = hashCombine(seed, info.addressModeW);
                seed = hashCombine(seed, info.mipLodBias);
                seed = hashCombine(seed, info.anisotropyEnable);
                seed = hashCombine(seed, info.maxAnisotropy);
                seed = hashCombine(seed, info.compareEnable);
                seed = hashCombine(seed, info.compareOp);
                seed = hashCombine(seed, info.minLod);
                seed = hashCombine(seed, info.maxLod);
                seed = hashCombine(seed, info.borderColor);
                seed = hashCombine(seed, info.unnormalizedCoordinates);

                return seed;
            }
        };

        struct SamplerCreateInfoEqual
        {
            bool operator()(const VkSamplerCreateInfo& lhs, const VkSamplerCreateInfo& rhs) const
            {
                return std::memcmp(&lhs, &rhs, sizeof(VkSamplerCreateInfo)) == 0;
            }
        };

        // Stores unique samplers with their corresponding VkSamplerCreateInfo
        std::unordered_map<VkSamplerCreateInfo, VkSampler, SamplerCreateInfoHash, SamplerCreateInfoEqual> m_samplerMap
            {};

        // Internal function to create a new VkSampler
        const VkSampler createSampler(const VkSamplerCreateInfo& createInfo) const
        {
            ASSERT(m_device, "Initialization was missing");
            VkSampler sampler{};
            VK_CHECK(vkCreateSampler(m_device, &createInfo, nullptr, &sampler));
            return sampler;
        }
    };


    //--- GBuffer ------------------------------------------------------------------------------------------------------------

    /*--
     * GBuffer creation info
    -*/
    struct GbufferCreateInfo
    {
        VkDevice device{}; // Vulkan Device
        utils::ResourceAllocator* alloc{}; // Allocator for the images
        VkExtent2D size{}; // Width and height of the buffers
        std::vector<VkFormat> color{}; // Array of formats for each color attachment (as many GBuffers as formats)
        VkFormat depth{VK_FORMAT_UNDEFINED}; // Format of the depth buffer (VK_FORMAT_UNDEFINED for no depth)
        VkSampler linearSampler{}; // Linear sampler for displaying the images
        VkSampleCountFlagBits sampleCount{VK_SAMPLE_COUNT_1_BIT}; // MSAA sample count (default: no MSAA)
    };

    /*--
     * GBuffer - Multiple render targets with depth management
     * 
     * This class manages multiple color buffers and a depth buffer for deferred rendering or 
     * other multi-target rendering techniques. It supports:
     * - Multiple color attachments with configurable formats
     * - Optional depth buffer
     * - MSAA support
     * - ImGui integration for debug visualization
     * - Automatic resource cleanup
     *
     * The GBuffer images can be used as:
     * - Color/Depth attachments (write)
     * - Texture sampling (read)
     * - Storage images (read/write)
     * - Transfer operations
    -*/
    class Gbuffer
    {
    public:
        Gbuffer() = default;
        ~Gbuffer() { assert(m_createInfo.device == VK_NULL_HANDLE && "Missing deinit()"); }

        /*--
         * Initialize the GBuffer with the specified configuration.
        -*/
        void init(VkCommandBuffer cmd, const GbufferCreateInfo& createInfo)
        {
            ASSERT(m_createInfo.color.empty(), "Missing deinit()");
            // The buffer must be cleared before creating a new one
            m_createInfo = createInfo; // Copy the creation info
            create(cmd);
        }

        // Destroy internal resources and reset its initial state
        void deinit()
        {
            destroy();
            *this = {};
        }

        void update(VkCommandBuffer cmd, VkExtent2D newSize)
        {
            if (newSize.width == m_createInfo.size.width && newSize.height == m_createInfo.size.height)
                return;

            destroy();
            m_createInfo.size = newSize;
            create(cmd);
        }


        //--- Getters for the GBuffer resources -------------------------
        ImTextureID getImTextureID(uint32_t i = 0) const { return reinterpret_cast<ImTextureID>(m_descriptorSet[i]); }
        VkExtent2D getSize() const { return m_createInfo.size; }
        VkImage getColorImage(uint32_t i = 0) const { return m_res.gBufferColor[i].image; }
        VkImage getDepthImage() const { return m_res.gBufferDepth.image; }
        VkImageView getColorImageView(uint32_t i = 0) const { return m_res.descriptor[i].imageView; }
        const VkDescriptorImageInfo& getDescriptorImageInfo(uint32_t i = 0) const { return m_res.descriptor[i]; }
        VkImageView getDepthImageView() const { return m_res.depthView; }
        VkFormat getColorFormat(uint32_t i = 0) const { return m_createInfo.color[i]; }
        VkFormat getDepthFormat() const { return m_createInfo.depth; }
        VkSampleCountFlagBits getSampleCount() const { return m_createInfo.sampleCount; }
        float getAspectRatio() const { return float(m_createInfo.size.width) / float(m_createInfo.size.height); }

    private:
        /*--
         * Create the GBuffer with the specified configuration
         *
         * Each color buffer is created with:
         * - Color attachment usage     : For rendering
         * - Sampled bit                : For sampling in shaders
         * - Storage bit                : For compute shader access
         * - Transfer dst bit           : For clearing/copying
         * 
         * The depth buffer is created with:
         * - Depth/Stencil attachment   : For depth testing
         * - Sampled bit                : For sampling in shaders
         *
         * All images are transitioned to GENERAL layout and cleared to black.
         * ImGui descriptors are created for debug visualization.
        -*/
        void create(VkCommandBuffer cmd)
        {
            DebugUtil& dutil = DebugUtil::getInstance();
            const VkImageLayout layout{VK_IMAGE_LAYOUT_GENERAL};

            const auto numColor = static_cast<uint32_t>(m_createInfo.color.size());

            m_res.gBufferColor.resize(numColor);
            m_res.descriptor.resize(numColor);
            m_res.uiImageViews.resize(numColor);
            m_descriptorSet.resize(numColor);

            for (uint32_t c = 0; c < numColor; c++)
            {
                {
                    // Color image
                    const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_STORAGE_BIT
                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    const VkImageCreateInfo info = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                        .imageType = VK_IMAGE_TYPE_2D,
                        .format = m_createInfo.color[c],
                        .extent = {m_createInfo.size.width, m_createInfo.size.height, 1},
                        .mipLevels = 1,
                        .arrayLayers = 1,
                        .samples = m_createInfo.sampleCount,
                        .usage = usage,
                    };
                    m_res.gBufferColor[c] = m_createInfo.alloc->createImage(info);
                    dutil.setObjectName(m_res.gBufferColor[c].image, "G-Color" + std::to_string(c));
                }
                {
                    // Image color view
                    VkImageViewCreateInfo info = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .image = m_res.gBufferColor[c].image,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = m_createInfo.color[c],
                        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
                    };
                    vkCreateImageView(m_createInfo.device, &info, nullptr, &m_res.descriptor[c].imageView);
                    dutil.setObjectName(m_res.descriptor[c].imageView, "G-Color" + std::to_string(c));

                    // UI Image color view
                    info.components.a = VK_COMPONENT_SWIZZLE_ONE; // Forcing the VIEW to have a 1 in the alpha channel
                    vkCreateImageView(m_createInfo.device, &info, nullptr, &m_res.uiImageViews[c]);
                    dutil.setObjectName(m_res.uiImageViews[c], "UI G-Color" + std::to_string(c));
                }

                // Set the sampler for the color attachment
                m_res.descriptor[c].sampler = m_createInfo.linearSampler;
            }

            if (m_createInfo.depth != VK_FORMAT_UNDEFINED)
            {
                // Depth buffer
                const VkImageCreateInfo createInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = m_createInfo.depth,
                    .extent = {m_createInfo.size.width, m_createInfo.size.height, 1},
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = m_createInfo.sampleCount,
                    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                };
                m_res.gBufferDepth = m_createInfo.alloc->createImage(createInfo);
                dutil.setObjectName(m_res.gBufferDepth.image, "G-Depth");

                // Image depth view
                const VkImageViewCreateInfo viewInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = m_res.gBufferDepth.image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = m_createInfo.depth,
                    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1},
                };
                vkCreateImageView(m_createInfo.device, &viewInfo, nullptr, &m_res.depthView);
                dutil.setObjectName(m_res.depthView, "G-Depth");
            }

            {
                // Change color image layout
                for (uint32_t c = 0; c < numColor; c++)
                {
                    cmdTransitionImageLayout(cmd, m_res.gBufferColor[c].image, VK_IMAGE_LAYOUT_UNDEFINED, layout);
                    m_res.descriptor[c].imageLayout = layout;

                    // Clear to avoid garbage data
                    const VkClearColorValue clear_value = {{0.F, 0.F, 0.F, 0.F}};
                    const std::array<VkImageSubresourceRange, 1> range = {
                        {{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}}
                    };
                    vkCmdClearColorImage(cmd, m_res.gBufferColor[c].image, layout, &clear_value, uint32_t(range.size()),
                                         range.data());
                }
            }

            // Descriptor Set for ImGUI
            if ((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
            {
                for (size_t d = 0; d < m_res.descriptor.size(); ++d)
                {
                    m_descriptorSet[d] = ImGui_ImplVulkan_AddTexture(m_createInfo.linearSampler, m_res.uiImageViews[d],
                                                                     layout);
                }
            }
        }

        /*--
         * Clean up all Vulkan resources
         * - Images and image views
         * - Samplers
         * - ImGui descriptors
         * 
         * This must be called before destroying the GBuffer or when
         * recreating with different parameters
        -*/
        void destroy()
        {
            if ((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
            {
                for (VkDescriptorSet set : m_descriptorSet)
                {
                    ImGui_ImplVulkan_RemoveTexture(set);
                }
                m_descriptorSet.clear();
            }

            for (utils::Image bc : m_res.gBufferColor)
            {
                m_createInfo.alloc->destroyImage(bc);
            }

            if (m_res.gBufferDepth.image != VK_NULL_HANDLE)
            {
                m_createInfo.alloc->destroyImage(m_res.gBufferDepth);
            }

            vkDestroyImageView(m_createInfo.device, m_res.depthView, nullptr);

            for (const VkDescriptorImageInfo& desc : m_res.descriptor)
            {
                vkDestroyImageView(m_createInfo.device, desc.imageView, nullptr);
            }

            for (const VkImageView& view : m_res.uiImageViews)
            {
                vkDestroyImageView(m_createInfo.device, view, nullptr);
            }
        }


        /*--
         * Resources holds all Vulkan objects for the GBuffer
         * This separation makes it easier to cleanup and recreate resources
        -*/
        struct Resources
        {
            std::vector<utils::Image> gBufferColor{}; // Color attachments
            utils::Image gBufferDepth{}; // Optional depth attachment
            VkImageView depthView{}; // View for the depth attachment
            std::vector<VkDescriptorImageInfo> descriptor{}; // Descriptor info for each color attachment
            std::vector<VkImageView> uiImageViews{}; // Special views for ImGui (alpha=1)
        };

        Resources m_res; // All Vulkan resources

        GbufferCreateInfo m_createInfo{}; // Configuration
        std::vector<VkDescriptorSet> m_descriptorSet{}; // ImGui descriptor sets
    };

    //--- Other helpers ------------------------------------------------------------------------------------------------------------


    /*--
     * Return the path to a file if it exists in one of the search paths.
    -*/
    static std::string findFile(const std::string& filename, const std::vector<std::string>& searchPaths)
    {
        for (const auto& path : searchPaths)
        {
            std::filesystem::path filePath = std::filesystem::path(path) / filename;
            if (std::filesystem::exists(filePath))
            {
                // Convert path to string with forward slashes:
                std::string result = filePath.make_preferred().string();
                std::replace(result.begin(), result.end(), '\\', '/');
                return result;
            }
        }
        LOGE("File not found: %s", filename.c_str());
        LOGI("Search under: ");
        for (const auto& path : searchPaths)
        {
            LOGI("  %s", path.c_str());
        }
        return "";
    }
} // namespace utils

//--- VulkanRendererAPI ------------------------------------------------------------------------------------------------------------
// Main class for the sample

/*--
 * The application is the main class that is used to create the window, the Vulkan context, the swapchain, and the resources.
 *  - run the main loop.
 *  - render the scene.
-*/
class VulkanRendererAPI : public RendererAPI
{
public:
    VulkanRendererAPI();
    explicit VulkanRendererAPI(const Config& config);
    ~VulkanRendererAPI();

    //expose variable------
    void SetVSync(bool enable) override
    {
        if (m_vSync != enable)
        {
            m_vSync = enable;
            RebuildSwapchain(); // trigger swapchain rebuild immediately
        }
    }

    bool GetVSync() const { return m_vSync; }

    bool IsWindowMinimized() const override
    {
        return SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED;
    }

    void RebuildSwapchain() override { m_swapchain.needToRebuild(); }

    ImageHandle GetGBufferImage() const override
    {
        return {reinterpret_cast<void*>(m_gBuffer.getColorImage())};
    }

    ImageHandle GetSwapchainImage() const override
    {
        return {reinterpret_cast<void*>(m_swapchain.getNextImage())};
    }

    ImageHandle GetImGuiTextureID(uint32_t index = 0) const override
    {
        return {reinterpret_cast<void*>(m_gBuffer.getImTextureID(index))};
    }

    Extent2D GetViewportSize() const override
    {
        return {m_viewportSize.width, m_viewportSize.height};
    }

    Extent2D GetWindowSize() const override
    {
        return {m_windowSize.width, m_windowSize.height};
    }

    WindowHandle GetWindowHandle() const override
    {
        return reinterpret_cast<void*>(m_window);
    }

    int GetImageID() const override
    {
        return m_imageID;
    }

    void SetImageID(int id) override
    {
        /*if (id >= 0 && id < m_gBuffer.getImageCount())
            m_imageID = id;*/ //maybe ??
        m_imageID = id;
    }

    uint32_t AddTextureToPool(utils::ImageResource&& imageResource);
    void RemoveTextureFromPool(uint32_t index);
    void ResizeDescriptor();
    uint32_t GetTextureCount() const { return static_cast<uint32_t>(m_image.size()); }
    uint32_t GetMaxTexture() const { return m_maxTextures; }
    bool IsTextureValid(uint32_t index) const { return index < m_image.size(); }
    bool IsInitialized() const { return s_instance != nullptr; }

    static VulkanRendererAPI& Get();

    utils::Context& GetContext() { return m_context; }
    VkCommandPool& GetTransientCmdPool() { return m_transientCmdPool; }
    utils::ResourceAllocator& GetAllocator() { return m_allocator; }
    static VulkanRendererAPI* s_instance;
    static void SetInstance(VulkanRendererAPI* rendererAPI);

    //expose variable end-------

private:
    /*--
     * Main initialization sequence
     * 
     * 1. Context Creation
     *    - Creates Vulkan instance with validation layers
     *    - Selects physical device (prefers discrete GPU)
     *    - Creates logical device with required extensions
     * 
     * 2. Resource Setup
     *    - Initializes VMA allocator for memory management
     *    - Creates command pools for graphics commands
     *    - Sets up swapchain for rendering
     * 
     * 3. Pipeline Creation
     *    - Creates descriptor layouts for textures and buffers
     *    - Sets up graphics pipeline with vertex/fragment shaders
     *    - Creates compute pipeline for vertex animation
     * 
     * 4. Resource Creation
     *    - Loads and uploads texture
     *    - Creates vertex and uniform buffers
     *    - Sets up ImGui for UI rendering
    -*/
    void init();
    void destroyGraphicsPipeline() const override;

    /*--
     * Destroy all resources and the Vulkan context
    -*/
    void destroy();

    /*--
     * Create a command pool for short lived operations
     * The command pool is used to allocate command buffers.
     * In the case of this sample, we only need one command buffer, for temporary execution.
    -*/
    void createTransientCommandPool();

    /*--
     * Creates a command pool (long life) and buffer for each frame in flight. Unlike the temporary command pool,
     * these pools persist between frames and don't use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT.
     * Each frame gets its own command buffer which records all rendering commands for that frame.
    -*/
    void createFrameSubmission(uint32_t numFrames);

    /*--
     * Main frame rendering function
     * 
     * 1. Frame Setup
     *    - Waits for previous frame to complete
     *    - Acquires next swapchain image
     *    - Begins command buffer recording
     * 
     * 2. Compute Pass
     *    - Updates vertex positions through compute shader
     *    - Synchronizes compute/graphics operations
     * 
     * 3. Graphics Pass
     *    - Updates scene uniform buffer
     *    - Begins dynamic rendering
     *    - Draws animated triangle
     *    - Draws textured triangle
     * 
     * 4. Finalization
     *    - Begins dynamic rendering in swapchain
     *    - Renders ImGui, which displays the GBuffer image
     * 
     * 4. Frame Submission
     *    - Submits command buffer
     *    - Presents the rendered image
     * 
     * Note: Uses dynamic rendering instead of traditional render passes
    -*/

    void drawFrame();

    /*---
     * Begin frame is the first step in the rendering process.
     * It looks if the swapchain require rebuild, which happens when the window is resized.
     * It resets the command pool to reuse the command buffer for recording new rendering commands for the current frame.
     * It acquires the image from the swapchain to render into.
     * And it returns the command buffer for the frame.
    -*/
    VkCommandBuffer beginFrame();
    //export
    VanKCommandBuffer BeginCommandBuffer() override;

    /*--
     * End the frame by submitting the command buffer to the GPU and presenting the image.
     * Adds binary semaphores to wait for the image to be available and signal when rendering is done.
     * Adds the timeline semaphore to signal when the frame is completed.
     * Moves to the next frame.
    -*/
    void endFrame(VanKCommandBuffer cmd) override;

    /*-- 
     * Call this function if the viewport size changes 
     * This happens when the window is resized, or when the ImGui viewport window is resized.
    -*/

    void OnViewportSizeChange(const Extent2D& size) override;


    /*--
     * We are using dynamic rendering, which is a more flexible way to render to the swapchain image.
    -*/
    void beginDynamicRenderingToSwapchain(VanKCommandBuffer cmd) override;

    /*--
     * End of dynamic rendering.
     * The image is transitioned back to the present layout, and the rendering is ended.
    -*/
    void endDynamicRenderingToSwapchain(VanKCommandBuffer cmd) override;

    /*-- 
     * This is invoking the compute shader to update the Vertex in the buffer (animation of triangle).
     * Where those vertices will be then used to draw the geometry
    -*/
    void recordComputeCommands(VanKCommandBuffer cmd) const override;

    /*-- 
     * The update of scene information buffer (UBO)
    -*/
    void updateSceneBuffer(VkCommandBuffer cmd) const;

    /*--
     * Recording the commands to render the scene
    -*/
    void recordGraphicCommands(VanKCommandBuffer cmd) override;

    /*--
     * The graphic pipeline is all the stages that are used to render a section of the scene.
     * Stages like: vertex shader, fragment shader, rasterization, and blending.
    -*/
    void createGraphicsPipeline() override;

    /*-- Wait until GPU is done using the pipeline to safly destroy --*/
    void waitForGraphicsQueueIdle() override;

    /*-- Initialize ImGui -*/
    void initImGui();

    /*-- Render ImGui -*/
    void renderImGui(VanKCommandBuffer cmd) override;

    /*-- Blit SwapChain --*/
    void BlitGBufferToSwapchain(VanKCommandBuffer cmd) override;

    /*--
     * The Descriptor Pool is used to allocate descriptor sets.
     * Currently, only ImGui requires a combined image sampler.
    -*/
    void createDescriptorPool();

    /*--
     * The Vulkan descriptor set defines the resources that are used by the shaders.
     * In this application we have two descriptor layout, one for the texture and one for the scene information.
     * But only one descriptor set (texture), the scene information is a push descriptor.
     * Set are used to group resources, and layout to define the resources.
     * Push are limited to a certain number of bindings, but are synchronized with the frame.
     * Set can be huge, but are not synchronized with the frame (command buffer).
     -*/
    void createGraphicDescriptorSet();

    /*--
     * The resources associated with the descriptor set must be set, in order to be used in the shaders.
     * This is actually updating the unbind array of textures
    -*/
    void updateGraphicsDescriptorSet();

    /*--
     * Loading an image using the stb_image library.
     * Create an image and upload the data to the GPU.
     * Create an image view (Image view are for the shaders, to access the image).
    -*/
    utils::ImageResource loadAndCreateImage(VkCommandBuffer cmd, const std::string& filename);

    // Creating the compute shader pipeline
    void createComputeShaderPipeline();

    // Helper to download color attachment 1 (entity ID buffer) to CPU and keep pointer
    int32_t* downloadColorAttachmentEntityID() override;

    void unmapDownloadedData();

    //--------------------------------------------------------------------------------------------------
private:
    SDL_Window* m_window{}; // The window
    utils::Context m_context; // The Vulkan context
    utils::ResourceAllocator m_allocator; // The VMA allocator
    utils::Swapchain m_swapchain; // The swapchain
    utils::Buffer m_vertexBuffer; // The vertex buffer (two triangles) (SSBO)
    utils::Buffer m_pointsBuffer; // The data buffer (SSBO)
    utils::Buffer m_sceneInfoBuffer; // The buffer used to pass data to the shader (UBO)
    utils::Buffer m_indexBuffer; // The index buffer for indexed rendering
    std::vector<utils::ImageResource> m_image; // The loaded image
    utils::SamplerPool m_samplerPool; // The sampler pool, used to create a sampler for the texture

    utils::Gbuffer m_gBuffer; // The G-Buffer

    VkSurfaceKHR m_surface{}; // The window surface
    VkExtent2D m_windowSize{800, 600}; // The window size
    VkExtent2D m_viewportSize{800, 600}; // The viewport area in the window
    VkPipelineLayout m_graphicPipelineLayout{}; // The pipeline layout use with graphics pipeline
    VkPipelineLayout m_computePipelineLayout{}; // The pipeline layout use with compute pipeline
    VkPipeline m_computePipeline{}; // The compute pipeline
    VkPipeline m_graphicsPipelineWithTexture{}; // The graphics pipeline with texture
    VkPipeline m_graphicsPipelineWithoutTexture{}; // The graphics pipeline without texture
    VkCommandPool m_transientCmdPool{}; // The command pool
    VkDescriptorPool m_descriptorPool{}; // Application descriptor pool
    VkDescriptorPool m_imguiDescriptorPool{}; // Separate pool for ImGui
    VkDescriptorSetLayout m_textureDescriptorSetLayout{}; // Descriptor set layout for all textures (set 0)
    VkDescriptorSetLayout m_graphicDescriptorSetLayout{}; // Descriptor set layout for the scene info (set 1)
    VkDescriptorSet m_textureDescriptorSet{}; // Application descriptor set (storing all textures)

    // Frame resources and synchronization
    struct FrameData
    {
        VkCommandPool cmdPool; // Command pool for recording commands for this frame
        VkCommandBuffer cmdBuffer; // Command buffer containing the frame's rendering commands
        uint64_t frameNumber; // Timeline value for synchronization (increases each frame)
    };

    std::vector<FrameData> m_frameData{}; // Collection of per-frame resources to support multiple frames in flight
    VkSemaphore m_frameTimelineSemaphore{}; // Timeline semaphore used to synchronize CPU submission with GPU completion
    uint32_t m_frameRingCurrent{0}; // Current frame index in the ring buffer (cycles through available frames)
    
    bool m_vSync{false}; // VSync on or off
    int m_imageID{0}; // The current image to display
    uint32_t m_maxTextures{1}; // Maximum textures allowed in the application
    
    int32_t* m_downloadedEntityIDPtr = nullptr; // Pointer to mapped entity ID buffer
    utils::Buffer m_downloadedEntityIDBuffer{}; // Buffer handle for cleanup
};
