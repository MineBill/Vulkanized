#include "Application.h"
#include "Logger.h"
#include <vulkan/vk_enum_string_helper.h>

void Sierpinski(
        std::vector<Model::Vertex> &vertices,
        int depth,
        glm::vec2 left,
        glm::vec2 right,
        glm::vec2 top)
{
    if (depth <= 0) {
        vertices.push_back({top});
        vertices.push_back({left});
        vertices.push_back({right});
    } else {
        auto leftTop = 0.5f * (left + top);
        auto rightTop = 0.5f * (right + top);
        auto leftRight = 0.5f * (left + right);
        Sierpinski(vertices, depth - 1, left, leftRight, leftTop);
        Sierpinski(vertices, depth - 1, leftRight, right, rightTop);
        Sierpinski(vertices, depth - 1, leftTop, rightTop, top);
    }
}

void Application::Initialize()
{
    auto config = PipelineConfigInfo::Default(m_Window.Width(), m_Window.Height());
    config.Layout = CreatePipelineLayout();
    m_pipelineLayout = config.Layout;
    config.RenderPass = m_swapchain.RenderPass();
    m_pipeline = std::make_unique<Pipeline>(m_device, config);

    std::vector<Model::Vertex> vertices;//{{{0.0f, -0.5}}, {{0.5f, 0.5f}}, {{-0.5f, 0.5f}}};
    Sierpinski(vertices, 8, {0.0f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f});
    m_model = std::make_unique<Model>(m_device, vertices);
    CreateCommandBuffers();
}

Application::~Application()
{
    vkDestroyPipelineLayout(m_device.LogicalDevice(), m_pipelineLayout, nullptr);
}

void Application::Run()
{
    Initialize();

    Logger::DefaultLogger()->SetLogLevel(LogLevel::Info);
    FATAL("This is a fatal message.");
    ERROR("This is an error message.");
    WARN("This is a warning message.");
    INFO("This is an info message.");
    DEBUG("This is a debug message.");

    INFOF("This is the {} message with {} formatting.", 2, "custom");
    while (!m_Window.ShouldClose()) {
        m_Window.Update();
        // Main stuff
        DrawFrame();
    }

    vkDeviceWaitIdle(m_device.LogicalDevice());
}

VkPipelineLayout Application::CreatePipelineLayout()
{
    VkPipelineLayoutCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };

    VkPipelineLayout layout;
    auto result = vkCreatePipelineLayout(m_device.LogicalDevice(), &info, nullptr, &layout);
    if (result != VK_SUCCESS) {
        ERROR("Failed to create pipeline layout");
    }
    return layout;
}

void Application::CreateCommandBuffers()
{
    m_commandBuffers.resize(m_swapchain.ImageCount());

    VkCommandBufferAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_device.CommandPool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = m_swapchain.ImageCount(),
    };

    auto result = vkAllocateCommandBuffers(m_device.LogicalDevice(), &allocateInfo, m_commandBuffers.data());
    if (result != VK_SUCCESS) {
        ERRORF("Failed to allocate command buffers: {}", string_VkResult(result));
    }

    for (u32 i = 0; i < m_commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

        result = vkBeginCommandBuffer(m_commandBuffers[i], &info);
        if (result != VK_SUCCESS) {
            ERRORF("Failed to begin command buffers: {}", string_VkResult(result));
        }

        VkRenderPassBeginInfo renderPassBeginInfo{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = m_swapchain.RenderPass(),
                .framebuffer = m_swapchain.GetFramebuffer(i),
                .renderArea = VkRect2D{
                        .offset = {0, 0},
                        .extent = m_swapchain.Extent(),
                }};

        VkClearValue clearValues[2] = {
                VkClearValue{
                        .color = {0.1f, 0.1f, 0.1f, 1.0f},
                },
                VkClearValue{
                        .depthStencil = {1.0f, 0}}};

        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_pipeline->BindCommandBuffer(m_commandBuffers[i]);

        m_model->Bind(m_commandBuffers[i]);
        m_model->Draw(m_commandBuffers[i]);

        // vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(m_commandBuffers[i]);

        result = vkEndCommandBuffer(m_commandBuffers[i]);

        if (result != VK_SUCCESS) {
            ERRORF("Failed to record command buffer: {}", string_VkResult(result));
        }
    }
}

void Application::DrawFrame()
{
    u32 imageIndex = m_swapchain.AcquireNextImage();

    m_swapchain.SubmitCommandBuffers(&m_commandBuffers[imageIndex], imageIndex);
}
