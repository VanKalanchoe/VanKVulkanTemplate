#include "RenderCommand.h"

RendererAPI::Config RenderCommand::s_Config = {};
std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = nullptr;