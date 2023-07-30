#include "Pipeline.h"
#include "Logger.h"
#include <fstream>
#include <vulkan/vk_enum_string_helper.h>

PipelineConfigInfo PipelineConfigInfo::Default(u32 width, u32 height) {
    PipelineConfigInfo pipelineConfigInfo{
            .InputAssemblyInfo = VkPipelineInputAssemblyStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                    .primitiveRestartEnable = false,
            },
            .RasterizationInfo = VkPipelineRasterizationStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                    .depthClampEnable = false,
                    .rasterizerDiscardEnable = false,
                    .polygonMode = VK_POLYGON_MODE_FILL,
                    .cullMode = VK_CULL_MODE_BACK_BIT,
                    .frontFace = VK_FRONT_FACE_CLOCKWISE,
                    .depthBiasEnable = false,
                    .lineWidth = 1,
            },
            .MultisampleInfo = VkPipelineMultisampleStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                    .sampleShadingEnable = false,
            },
            .ColorBlendAttachmentInfo = VkPipelineColorBlendAttachmentState{
                    .blendEnable = true,
                    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            },
            .ColorBlendInfo = VkPipelineColorBlendStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                    .logicOpEnable = false,
                    .attachmentCount = 1,
                    .pAttachments = &pipelineConfigInfo.ColorBlendAttachmentInfo,
            },
            .DepthStencilInfo = VkPipelineDepthStencilStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .depthTestEnable = true,
                    .depthWriteEnable = true,
                    .depthCompareOp = VK_COMPARE_OP_LESS,
                    .depthBoundsTestEnable = false,
                    .stencilTestEnable = false,
                    .front = {},
                    .back = {},
                    .minDepthBounds = 0.0f,
                    .maxDepthBounds = 1.0f,
            },
            .Layout = VK_NULL_HANDLE,
            .RenderPass = VK_NULL_HANDLE,
            .SubPass = 0,
    };

    pipelineConfigInfo.Viewport = VkViewport{
            .x = 0,
            .y = 0,
            .width = static_cast<f32>(width),
            .height = static_cast<f32>(height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    pipelineConfigInfo.Scissor = VkRect2D{
            .offset = {0, 0},
            .extent = {.width = width, .height = height},
    };

    return pipelineConfigInfo;
}


Pipeline::Pipeline(Device &device, PipelineConfigInfo config) : m_device(device) {
    auto vertCode = LoadShaderByteCode("Assets/Shaders/Builtin.Object.vert.spv");
    auto fragCode = LoadShaderByteCode("Assets/Shaders/Builtin.Object.frag.spv");

    m_vertexModule = CreateShaderModule(vertCode);
    m_fragmentModule = CreateShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = m_vertexModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = m_fragmentModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo stages[] = {vertStageInfo, fragStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .vertexAttributeDescriptionCount = 0,
    }
    ;
   VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &config.Viewport,
            .scissorCount = 1,
            .pScissors = &config.Scissor,
    };


    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = stages,
            .pVertexInputState = &vertexInputStateCreateInfo,
            .pInputAssemblyState = &config.InputAssemblyInfo,
            .pViewportState = &viewportStateCreateInfo,
            .pRasterizationState = &config.RasterizationInfo,
            .pMultisampleState = &config.MultisampleInfo,
            .pDepthStencilState = &config.DepthStencilInfo,
            .pColorBlendState = &config.ColorBlendInfo,
            .pDynamicState = nullptr,

            .layout = config.Layout,
            .renderPass = config.RenderPass,
            .subpass = config.SubPass,

            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
    };

    INFO("Creating graphics pipeline");
    auto result = vkCreateGraphicsPipelines(m_device.LogicalDevice(), nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_pipelineHandle);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create graphics pipeline: {}", string_VkResult(result));
        return;
    }
    INFO("Successfully created graphics pipeline");
}

Pipeline::~Pipeline() {
    vkDestroyShaderModule(m_device.LogicalDevice(), m_vertexModule, nullptr);
    vkDestroyShaderModule(m_device.LogicalDevice(), m_fragmentModule, nullptr);
    vkDestroyPipeline(m_device.LogicalDevice(), m_pipelineHandle, nullptr);
}

std::vector<char> Pipeline::LoadShaderByteCode(const char *filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        ERRORF("Failed to open file '{}'", filePath);
        return {};
    }
    INFOF("Loading shader bytecode from '{}'", filePath);

    auto size = file.tellg();
    INFOF("\tBytecode size is {} bytes", static_cast<i32>(file.tellg()));
    std::vector<char> byteCode(size);
    file.seekg(0, std::ifstream::beg);

    file.read(byteCode.data(), static_cast<std::streamsize>(byteCode.size()));
    return byteCode;
}

VkShaderModule Pipeline::CreateShaderModule(const std::vector<char> &byteCode) {
    INFO("Creating shader module");
    VkShaderModuleCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = static_cast<u32>(byteCode.size()),
            .pCode = reinterpret_cast<const u32 *>(byteCode.data()),
    };

    VkShaderModule module;
    auto result = vkCreateShaderModule(m_device.LogicalDevice(), &info, nullptr, &module);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create shader module: {}", string_VkResult(result));
    }

    return module;
}

void Pipeline::BindCommandBuffer(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineHandle);
}
