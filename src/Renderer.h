#pragma once

#include <SDL3/SDL_init.h>

#include "RenderCommand.h"
#include <array>
#include <glm/glm.hpp>

namespace shaderio
{
    using namespace glm;
#include "shaders/shader_io.h"
}

/*// 2x3 vertices with a position, color and texCoords, make two CCW triangles
static const auto s_vertices = std::to_array<shaderio::Vertex>({
    {{0.0F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.5F, 0.5F}},  // Colored triangle
    {{-0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.5F, 0.5F}},
    {{0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {0.5F, 0.5F}},
    //
    {{0.1F, -0.4F, 0.75F}, {.3F, .3F, .3F}, {0.5F, 1.0F}},  // White triangle (textured)
    {{-0.4F, 0.6F, 0.25F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F}},
    {{0.6F, 0.6F, 0.75F}, {.7F, .7F, .7F}, {0.0F, 0.0F}},
});


// Indices for the two triangles
static const auto s_indices = std::to_array<uint32_t>({
    0, 1, 2, // First triangle (colored)
    3, 4, 5 // Second triangle (textured)
});*/

static const auto s_vertices = std::to_array<shaderio::Vertex>({
    {{-0.5F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}}, // 0
    {{-0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}}, // 1
    {{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}}, // 2
    {{0.5F, -0.5F, 0.5F}, {1.0F, 1.0F, 0.0F}, {1.0F, 0.0F}}, // 3
     //
    {{-0.4F, -0.4F, 0.75F}, {0.3F, 0.3F, 0.3F}, {0.0F, 1.0F}}, // 4 Textured triangle 
    {{-0.4F, 0.6F, 0.25F}, {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F}}, // 5
    {{0.6F, 0.6F, 0.25F}, {0.7F, 0.7F, 0.7F}, {1.0F, 0.0F}}, // 6
    {{0.6F, -0.4F, 0.75F}, {0.7F, 0.7F, 0.7F}, {1.0F, 1.0F}}, // 7
});

// Indices for the two triangles
static const auto s_indices = std::to_array<uint32_t>({
    0, 1, 2, // First triangle (colored)
    0, 2, 3,
    4, 5, 6, // Second triangle (textured)
    4, 6, 7
});

// No namespace, or use your project namespace if you want

class Renderer
{
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

    static void reloadGraphicsPipeline();

    static bool m_useImGui; // Flag to control ImGui rendering
    static bool m_windowResized;
    static Extent2D lastViewportSize;
};
