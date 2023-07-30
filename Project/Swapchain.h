#pragma once
#include "Definitions.h"
#include "Device.h"
#include <vector>
#include <vulkan/vulkan.h>

class Swapchain {
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent{};

    std::vector<VkFramebuffer> m_swapchainFrameBuffers;
    VkRenderPass m_renderPass{};

    std::vector<VkImage> m_depthImages;
    std::vector<VkDeviceMemory> m_depthImageMemories;
    std::vector<VkImageView> m_depthImageViews;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    Device &m_device;
    VkSwapchainKHR m_swapchain;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    u32 m_currentFrame = 0;

public:
    static constexpr u32 MaxFramesInFlight = 2;

    explicit Swapchain(Device &device);
    ~Swapchain();

    MUST_USE u32 ImageCount() const { return m_swapchainImages.size(); }
    MUST_USE VkRenderPass RenderPass() const { return m_renderPass; }
    MUST_USE VkExtent2D Extent() const { return m_swapchainExtent; }

    MUST_USE VkFramebuffer GetFramebuffer(u32 index) const { return m_swapchainFrameBuffers[index]; }
    MUST_USE u32 AcquireNextImage();

    void SubmitCommandBuffers(VkCommandBuffer const* buffers, u32 imageIndex);

private:
    void CreateSwapchain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateDepthResources();
    void CreateFramebuffers();
    void CreateSyncObjects();

    MUST_USE VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities);
};
