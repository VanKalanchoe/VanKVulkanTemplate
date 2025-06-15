#pragma once

#include <SDL3/SDL_init.h>

#include "RenderCommand.h"

// No namespace, or use your project namespace if you want

class Renderer {
public:
    /*static void Init()            { RenderCommand::Init(); }
    static void BeginFrame()      { RenderCommand::BeginFrame(); }
    static void EndFrame()        { RenderCommand::EndFrame(); }
    static void Submit()          { RenderCommand::Submit(); }
    static void SetLineWidth(float width) { RenderCommand::SetLineWidth(width); }
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { RenderCommand::SetViewport(x, y, width, height); }
    static void Clear()           { RenderCommand::Clear(); }*/

    static SDL_AppResult drawFrame();
    
    static void initRenderer();
    static void useImGui();
    static void rendererEvent(SDL_Event* event);
    static void OnViewportSizeChange(const Extent2D& newSize); // Called from SDL or main loop

    static bool m_useImGui; // Flag to control ImGui rendering
    static bool m_windowResized;
    static Extent2D lastViewportSize;
};