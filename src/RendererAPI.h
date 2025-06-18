#pragma once

#include <memory>
#include <cstdint>

// Forward declare an opaque struct (incomplete type)
struct VanKCommandBuffer_T;
using VanKCommandBuffer = VanKCommandBuffer_T*;

// In your engine's core headers
// Abstracted engine-side types
struct ImageHandle { void* handle = nullptr; }; // Opaque
struct Extent2D { uint32_t width, height; };
using WindowHandle = void*;


enum class RenderAPIType
{
    None = 0, Vulkan = 1, Metal = 2
};

class RendererAPI {
public:
    virtual ~RendererAPI() = default;

    /*virtual void Init() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Submit() = 0;
    virtual void SetLineWidth(float width) = 0;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void Clear() = 0;*/

    virtual VanKCommandBuffer BeginCommandBuffer() { return nullptr; }
    virtual void endFrame(VanKCommandBuffer cmd) = 0;
    virtual void beginDynamicRenderingToSwapchain(VanKCommandBuffer cmd) = 0;
    virtual void endDynamicRenderingToSwapchain(VanKCommandBuffer cmd) = 0;
    virtual void recordComputeCommands(VanKCommandBuffer cmd) const = 0;
    virtual void recordGraphicCommands(VanKCommandBuffer cmd) = 0;
    virtual void renderImGui(VanKCommandBuffer cmd) = 0;
    virtual void BlitGBufferToSwapchain(VanKCommandBuffer cmd) = 0;
    virtual int32_t* downloadColorAttachmentEntityID() = 0;
    virtual void destroyGraphicsPipeline() const = 0;
    virtual void createGraphicsPipeline() = 0;
    virtual void createComputeShaderPipeline() = 0;
    virtual void destroyComputePipeline() const = 0;
    virtual void waitForGraphicsQueueIdle() = 0;

    // In RendererAPI.h
    virtual void SetVSync(bool enable) = 0;
    virtual bool GetVSync() const = 0;
    virtual bool IsWindowMinimized() const { return false; }
    virtual void RebuildSwapchain() {}
    virtual void OnViewportSizeChange(const Extent2D& size) = 0;
    
    virtual ImageHandle GetGBufferImage() const = 0;
    virtual ImageHandle GetSwapchainImage() const = 0;
    virtual ImageHandle GetImGuiTextureID(uint32_t index = 0) const = 0;
    
    virtual Extent2D GetViewportSize() const = 0;
    virtual Extent2D GetWindowSize() const = 0;
    
    virtual WindowHandle GetWindowHandle() const = 0;

    virtual int GetImageID() const = 0;
    virtual void SetImageID(int id) = 0;

    static RenderAPIType GetAPI() { return s_API; }
    static void SetAPI(RenderAPIType api) { s_API = api; }

    // --- New: Configuration ---
    struct Config {
        uint32_t width = 800;
        uint32_t height = 600;
    };
    
    static std::unique_ptr<RendererAPI> Create(const Config& config);

private:
    static RenderAPIType s_API;
};