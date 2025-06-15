// No implementation needed, all methods are inline and static
#include "Renderer.h"

#include <imgui_internal.h>
#include <iostream>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"

bool Renderer::m_useImGui = true;           // initialize to default false
bool Renderer::m_windowResized = false;
Extent2D Renderer::lastViewportSize = {0,0}; // initialize as needed

void Renderer::initRenderer()
{
    RendererAPI::Config config;
    config.width = 800;
    config.height = 600;

    RenderCommand::SetConfig(config);  // Provide config to RenderCommand
    RenderCommand::Init();             // RenderCommand creates and initializes RendererAPI instance
}

void Renderer::useImGui()
{
    m_useImGui = !m_useImGui;
}

void Renderer::rendererEvent(SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void Renderer::OnViewportSizeChange(const Extent2D& newSize)
{
    if (newSize.width != lastViewportSize.width || newSize.height != lastViewportSize.height)
    {
        lastViewportSize = newSize;
        RenderCommand::OnViewportSizeChange(newSize);
    }
}

SDL_AppResult Renderer::drawFrame()
{
    auto window = reinterpret_cast<SDL_Window*>(RenderCommand::GetWindowHandle());
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10); // Idle while minimized
        return SDL_APP_CONTINUE;
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    /*--
     * IMGUI Docking
     * Create a dockspace and dock the viewport and settings window.
     * The central node is named "Viewport", which can be used later with Begin("Viewport")
     * to render the final image.
    -*/
    const ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode |
        ImGuiDockNodeFlags_NoDockingInCentralNode;
    ImGuiID dockID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockFlags);
    // Docking layout, must be done only if it doesn't exist
    if (!ImGui::DockBuilderGetNode(dockID)->IsSplitNode() && !ImGui::FindWindowByName("Viewport"))
    {
        ImGui::DockBuilderDockWindow("Viewport", dockID); // Dock "Viewport" to  central node
        ImGui::DockBuilderGetCentralNode(dockID)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        // Remove "Tab" from the central node
        ImGuiID leftID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.2f, nullptr, &dockID);
        // Split the central node
        ImGui::DockBuilderDockWindow("Settings", leftID); // Dock "Settings" to the left node
    }
    // [optional] Show the menu bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            bool vSync = RenderCommand::GetVSync();
            if (ImGui::MenuItem("vSync", "", &vSync))
                RenderCommand::SetVSync(vSync);// Recreate the swapchain with the new vSync setting rebuildswapchain builtin
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                {
                    SDL_Event quitEvent = {};            // Clear the struct (good practice)
                    quitEvent.type = SDL_EVENT_QUIT;    // Set event type
                    SDL_PushEvent(&quitEvent);         // Push to the SDL event queue
                }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // We define "viewport" with no padding an retrieve the rendering area
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImGui::End();
    ImGui::PopStyleVar();

    // Verify if the viewport has a new size and resize the G-Buffer accordingly.
    /*const VkExtent2D viewportSize = {uint32_t(windowSize.x), uint32_t(windowSize.y)};*/
    if (m_windowResized)
    {
        if (m_useImGui)
            Renderer::OnViewportSizeChange({ static_cast<uint32_t>(windowSize.x), static_cast<uint32_t>(windowSize.y) });
        else
        {
            int w = 0, h = 0;
            SDL_GetWindowSize(reinterpret_cast<SDL_Window*>(RenderCommand::GetWindowHandle()), &w, &h);
            Renderer::OnViewportSizeChange({ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
        }
        m_windowResized = false;
    }
    
    // ImGui::ShowDemoWindow();
    
//___drawFrame();
    VanKCommandBuffer cmd = RenderCommand::BeginCommandBuffer();
    if (!cmd)
        return SDL_APP_FAILURE;

    if (m_useImGui)
    {
        // ImGui UI logic (unchanged)
        if (ImGui::Begin("Viewport"))
        {
            ImGui::Image(reinterpret_cast<ImTextureID>(RenderCommand::GetImGuiTextureID(0).handle), ImGui::GetContentRegionAvail());
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        }
        ImGui::End();
        if (ImGui::Begin("Settings"))
        {
            int imageID = RenderCommand::GetImageID(); // get current value
            
            if (ImGui::RadioButton("Image 1", imageID == 0))
            {
                RenderCommand::SetImageID(0);
            }
            if (ImGui::RadioButton("Image 2", imageID == 1))
            {
                RenderCommand::SetImageID(1);
            }
            
            ImGui::Checkbox("Use ImGui", &m_useImGui);
        }
        ImGui::End();
        ImGui::Render();
    }

    RenderCommand::recordComputeCommands(cmd);
    RenderCommand::recordGraphicCommands(cmd);

    if (m_useImGui)
    {
        RenderCommand::beginDynamicRenderingToSwapchain(cmd);
        RenderCommand::renderImGui(cmd);
        RenderCommand::endDynamicRenderingToSwapchain(cmd);
    }
    else
    {
        RenderCommand::BlitGBufferToSwapchain(cmd);
    }

    RenderCommand::endFrame(cmd);
    //std::cout << RenderCommand::downloadColorAttachmentEntityID()[10] << '\n';

    //endFream______

    // Update and Render additional Platform Windows (floating windows)
    ImGui::EndFrame();
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    
    return SDL_APP_CONTINUE;
}
