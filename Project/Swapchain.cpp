#include "Swapchain.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>

Swapchain::Swapchain(Device &device) : m_device(device) {
    CreateSwapchain();
    CreateImageViews();
    CreateRenderPass();
    CreateDepthResources();
    CreateFramebuffers();
    CreateSyncObjects();
}

Swapchain::~Swapchain() {
    for (auto view : m_swapchainImageViews) {
        vkDestroyImageView(m_device.LogicalDevice(), view, nullptr);
    }

    vkDestroySwapchainKHR(m_device.LogicalDevice(), m_swapchain, nullptr);

    for (u32 i = 0; i < m_depthImages.size(); i++) {
        vkDestroyImageView(m_device.LogicalDevice(), m_depthImageViews[i], nullptr);
        vkDestroyImage(m_device.LogicalDevice(), m_depthImages[i], nullptr);
        vkFreeMemory(m_device.LogicalDevice(), m_depthImageMemories[i], nullptr);
    }

    for (auto framebuffer: m_swapchainFrameBuffers) {
        vkDestroyFramebuffer(m_device.LogicalDevice(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(m_device.LogicalDevice(), m_renderPass, nullptr);

    for (u32 i = 0; i < MaxFramesInFlight; i++) {
        vkDestroySemaphore(m_device.LogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device.LogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device.LogicalDevice(), m_inFlightFences[i], nullptr);
    }
}

void Swapchain::CreateSwapchain() {
    auto swapchainSupport = m_device.SwapchainSupport();

    auto presentMode = swapchainSupport.ChooseOptimalPresentMode();
    auto surfaceFormat = swapchainSupport.ChooseOptimalFormat();
    m_swapchainImageFormat = surfaceFormat.format;

    m_swapchainExtent = ChooseSwapExtent(swapchainSupport.Capabilities);

    auto imageCount = swapchainSupport.Capabilities.minImageCount + 1;
    if (swapchainSupport.Capabilities.maxImageCount > 0 && imageCount > swapchainSupport.Capabilities.maxImageCount) {
        imageCount = swapchainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfoKhr{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_device.Surface(),
            .minImageCount = imageCount,
            .imageFormat = m_swapchainImageFormat,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = m_swapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = swapchainSupport.Capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = true,
            .oldSwapchain = VK_NULL_HANDLE,
    };

    INFO("Creating swapchain");
    auto result = vkCreateSwapchainKHR(m_device.LogicalDevice(), &swapchainCreateInfoKhr, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create swapchain: {}", string_VkResult(result));
    }
    INFO("Successfully created swapchain!");

    u32 swapchainImageCount;
    vkGetSwapchainImagesKHR(m_device.LogicalDevice(), m_swapchain, &swapchainImageCount, nullptr);
    m_swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(m_device.LogicalDevice(), m_swapchain, &swapchainImageCount, m_swapchainImages.data());
}

void Swapchain::CreateImageViews() {
    m_swapchainImageViews.reserve(m_swapchainImages.size());

    for (auto image: m_swapchainImages) {
        VkImageViewCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_swapchainImageFormat,
                .components = VkComponentMapping{
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

        VkImageView view;
        vkCreateImageView(m_device.LogicalDevice(), &info, nullptr, &view);
        m_swapchainImageViews.push_back(view);
    }
}

void Swapchain::CreateRenderPass() {
    VkAttachmentDescription attachmentDescription{
            .format = m_swapchainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference attachmentRef{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescription,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency};

    INFO("Creating render pass");
    auto result = vkCreateRenderPass(m_device.LogicalDevice(), &renderPassCreateInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create render pass: {}", string_VkResult(result));
    }
    INFO("Successfully created render pass");
}

void Swapchain::CreateDepthResources() {
    INFO("Creating depth resources");
    VkFormat depthFormat = m_device.FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    auto imageCount = m_swapchainImages.size();
    m_depthImages.resize(imageCount);
    m_depthImageMemories.resize(imageCount);
    m_depthImageViews.resize(imageCount);

    for (int i = 0; i < m_depthImages.size(); i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_swapchainExtent.width;
        imageInfo.extent.height = m_swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        auto image = m_device.CreateImage(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_depthImages[i] = image.Image;
        m_depthImageMemories[i] = image.Memory;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        auto result = vkCreateImageView(m_device.LogicalDevice(), &viewInfo, nullptr, &m_depthImageViews[i]);
        if (result != VK_SUCCESS) {
            ERRORF("Failed to create texture image view: {}", string_VkResult(result));
        }
    }
}

void Swapchain::CreateFramebuffers() {
    INFO("Creating framebuffers");
    m_swapchainFrameBuffers.resize(m_swapchainImages.size());

    u32 i = 0;
    for (auto view: m_swapchainImageViews) {
        VkFramebufferCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_renderPass,
                .attachmentCount = 1,
                .pAttachments = &view,
                .width = m_swapchainExtent.width,
                .height = m_swapchainExtent.height,
                .layers = 1,
        };

        auto result = vkCreateFramebuffer(m_device.LogicalDevice(), &info, nullptr, &m_swapchainFrameBuffers[i++]);
        if (result != VK_SUCCESS) {
            ERRORF("Failed to create framebuffer: {}", string_VkResult(result));
        }
    }
}

void Swapchain::CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(MaxFramesInFlight);
    m_renderFinishedSemaphores.resize(MaxFramesInFlight);
    m_inFlightFences.resize(MaxFramesInFlight);
    m_imagesInFlight.resize(ImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    for (u32 i = 0; i < MaxFramesInFlight; i++) {
        auto result = vkCreateSemaphore(m_device.LogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
        if (result != VK_SUCCESS) {
            ERRORF("Failed to create ImageAvailable semaphore for frame '{}'", i);
        }
        result = vkCreateSemaphore(m_device.LogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
        if (result != VK_SUCCESS) {
            ERRORF("Failed to create RenderFinished semaphore for frame '{}'", i);
        }
        result = vkCreateFence(m_device.LogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]);
        if (result != VK_SUCCESS) {
            ERRORF("Failed to create fence for frame '{}'", i);
        }
    }
}

VkExtent2D Swapchain::ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<u32>().max()) {
        DEBUGF("Choosing swap extent: VkExtent2D({}, {})", capabilities.currentExtent.width, capabilities.currentExtent.height);
        return capabilities.currentExtent;
    }
    i32 width, height;
    glfwGetFramebufferSize(m_device.GetWindow().NativeHandle(), &width, &height);
    VkExtent2D extent;
    extent.width = std::clamp(
            static_cast<u32>(width),
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
    extent.height = std::clamp(
            static_cast<u32>(height),
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
    return extent;
}

u32 Swapchain::AcquireNextImage() {
    vkWaitForFences(m_device.LogicalDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<u64>::max());

    u32 index;
    auto result = vkAcquireNextImageKHR(m_device.LogicalDevice(), m_swapchain, std::numeric_limits<u64>::max(), m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &index);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to acquire next image: {}", string_VkResult(result));
    }
    return index;
}

void Swapchain::SubmitCommandBuffers(VkCommandBuffer const* buffers, u32 imageIndex) {
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_device.LogicalDevice(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};

    VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = buffers,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,
    };

    vkResetFences(m_device.LogicalDevice(), 1, &m_inFlightFences[m_currentFrame]);
    auto result = vkQueueSubmit(m_device.GraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to submit draw command buffer: {}", string_VkResult(result));
    }

    VkSwapchainKHR swapChains[] = {m_swapchain};
    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
    };

    result = vkQueuePresentKHR(m_device.PresentQueue(), &presentInfo);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to present queue: {}", string_VkResult(result));
    }

    m_currentFrame = (m_currentFrame + 1) % MaxFramesInFlight;
}
