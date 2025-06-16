#pragma once
#include "RendererAPI.h"

class RenderCommand
{
public:
    static void Init()
    {
        s_RendererAPI = RendererAPI::Create(s_Config);
        /*if (s_RendererAPI) s_RendererAPI->Init();*/
    }

    /*static void BeginFrame() { if (s_RendererAPI) s_RendererAPI->BeginFrame(); }
    static void EndFrame()   { if (s_RendererAPI) s_RendererAPI->EndFrame(); }
    static void Submit()     { if (s_RendererAPI) s_RendererAPI->Submit(); }
    static void SetLineWidth(float width) { if (s_RendererAPI) s_RendererAPI->SetLineWidth(width); }
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { if (s_RendererAPI) s_RendererAPI->SetViewport(x, y, width, height); }
    static void Clear() { if (s_RendererAPI) s_RendererAPI->Clear(); }*/

    static VanKCommandBuffer BeginCommandBuffer()
    {
        return s_RendererAPI ? s_RendererAPI->BeginCommandBuffer() : nullptr;
    }

    static void endFrame(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->endFrame(cmd); }

    static void beginDynamicRenderingToSwapchain(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->beginDynamicRenderingToSwapchain(cmd); }

    static void endDynamicRenderingToSwapchain(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->endDynamicRenderingToSwapchain(cmd); }
    
    static void recordComputeCommands(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->recordComputeCommands(cmd); }

    static void recordGraphicCommands(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->recordGraphicCommands(cmd); }

    static void renderImGui(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->renderImGui(cmd); }

    static void BlitGBufferToSwapchain(VanKCommandBuffer cmd) { if (s_RendererAPI) s_RendererAPI->BlitGBufferToSwapchain(cmd); }
    
    static int32_t* downloadColorAttachmentEntityID() { return s_RendererAPI ? s_RendererAPI->downloadColorAttachmentEntityID() : nullptr; }

    static void destroyGraphicsPipeline() { if (s_RendererAPI) s_RendererAPI->destroyGraphicsPipeline(); }

    static void createGraphicsPipeline() { if (s_RendererAPI) s_RendererAPI->createGraphicsPipeline(); }

    static void waitForGraphicsQueueIdle() { if (s_RendererAPI) s_RendererAPI->waitForGraphicsQueueIdle(); }

    static void SetVSync(bool enable)
    {
        if (s_RendererAPI)
            s_RendererAPI->SetVSync(enable);
    }

    static bool GetVSync()
    {
        return s_RendererAPI ? s_RendererAPI->GetVSync() : false;
    }


    static bool IsWindowMinimized()
    {
        return s_RendererAPI ? s_RendererAPI->IsWindowMinimized() : false;
    }

    static void RebuildSwapchain()
    {
        if (s_RendererAPI) s_RendererAPI->RebuildSwapchain();
    }

    static void OnViewportSizeChange(const Extent2D& size)
    {
        if (s_RendererAPI)
            s_RendererAPI->OnViewportSizeChange(size);
    }

    static ImageHandle GetGBufferImage()
    {
        return s_RendererAPI ? s_RendererAPI->GetGBufferImage() : ImageHandle{};
    }

    static ImageHandle GetSwapchainImage()
    {
        return s_RendererAPI ? s_RendererAPI->GetSwapchainImage() : ImageHandle{};
    }

    static ImageHandle GetImGuiTextureID(uint32_t index = 0)
    {
        return s_RendererAPI ? s_RendererAPI->GetImGuiTextureID(index) : ImageHandle{};
    }

    static Extent2D GetViewportSize()
    {
        return s_RendererAPI ? s_RendererAPI->GetViewportSize() : Extent2D{};
    }

    static Extent2D GetWindowSize()
    {
        return s_RendererAPI ? s_RendererAPI->GetWindowSize() : Extent2D{};
    }

    static WindowHandle GetWindowHandle()
    {
        return s_RendererAPI ? s_RendererAPI->GetWindowHandle() : nullptr;
    }

    static int GetImageID()
    {
        return s_RendererAPI ? s_RendererAPI->GetImageID() : 0;
    }

    static void SetImageID(int id) { if (s_RendererAPI) s_RendererAPI->SetImageID(id); }

    static void SetConfig(const RendererAPI::Config& cfg) { s_Config = cfg; }

private:
    static RendererAPI::Config s_Config;
    static std::unique_ptr<RendererAPI> s_RendererAPI;
};
