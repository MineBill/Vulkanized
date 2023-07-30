#pragma once
#include "Device.h"
#include "Window.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include <memory>
#include <vector>

class Application {
    Window m_Window{600, 400, "Window"};
    Device m_device{m_Window};
    std::unique_ptr<Pipeline> m_pipeline{};
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
