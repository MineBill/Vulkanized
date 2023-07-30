#pragma once
#include "Device.h"
#include "Window.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include <vector>
#include "Model.h"
#include "Types.h"

class Application {
    Window m_Window{600, 400, "Window"};
    Device m_device{m_Window};
    Ptr<Pipeline> m_pipeline{};
    Ptr<Model> m_model{};
    Swapchain m_swapchain{m_device};

    VkPipelineLayout m_pipelineLayout{};

    std::vector<VkCommandBuffer> m_commandBuffers{};

public:
    ~Application();
    void Initialize();
    void Run();

private:
    VkPipelineLayout CreatePipelineLayout();
    void CreateCommandBuffers();
    void DrawFrame();
};
