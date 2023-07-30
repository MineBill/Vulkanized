#pragma once
#include <vulkan/vulkan.h>
#include "Device.h"
#include <vector>

struct PipelineConfigInfo {
    VkViewport Viewport;
    VkRect2D Scissor;
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo RasterizationInfo;
    VkPipelineMultisampleStateCreateInfo MultisampleInfo;
    VkPipelineColorBlendAttachmentState ColorBlendAttachmentInfo;
    VkPipelineColorBlendStateCreateInfo ColorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo;
    VkPipelineLayout Layout;
    VkRenderPass RenderPass;
    u32 SubPass;

    static PipelineConfigInfo Default(u32 width, u32 height);
};

class Pipeline {
    VkPipeline m_pipelineHandle{};
    Device& m_device;
    VkShaderModule m_vertexModule, m_fragmentModule;
public:
    explicit Pipeline(Device& device, PipelineConfigInfo info);
    ~Pipeline();

    void BindCommandBuffer(VkCommandBuffer commandBuffer);

private:
    VkShaderModule CreateShaderModule(std::vector<char> const& byteCode);
    static std::vector<char> LoadShaderByteCode(const char* filePath);
};
