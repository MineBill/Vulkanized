#pragma once
#include "Definitions.h"
#include "Logger.h"
#include <vulkan/vulkan.h>
#include "Window.h"
#include <optional>
#include <vector>

struct QueueFamilyIndices {
    std::optional<u32> GraphicsFamily;
    std::optional<u32> PresentFamily;

    MUST_USE bool IsComplete() const {
        return GraphicsFamily.has_value() && PresentFamily.has_value();
    }

    MUST_USE std::vector<u32> GetUniqueIndex() const;
};

// TODO: Fledge this out
template<typename T>
class Slice {
    T *m_ptr{};
    u32 m_length{0};

public:
    Slice() : m_ptr(nullptr), m_length(0) {}
    Slice(T *ptr, u32 length) : m_ptr(ptr), m_length(length) {}
    Slice(const Slice &other) : m_ptr(other.m_ptr), m_length(other.m_length) {}

    T operator[](u32 i) {
        if (i >= m_length) {
            std::abort();
        }
        return m_ptr[i];
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;

    MUST_USE VkPresentModeKHR ChooseOptimalPresentMode() const {
        for (auto mode: PresentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    MUST_USE VkSurfaceFormatKHR ChooseOptimalFormat() const {
        for (auto format: Formats) {
            if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        WARN("Could not find optimal surface format");
        return Formats[0];
    }
};

struct Image {
    VkImage Image;
    VkDeviceMemory Memory;
};

class Device {
    Window &m_window;
    VkInstance m_vkInstance{};
    VkDebugUtilsMessengerEXT m_debugMessenger{};
    VkSurfaceKHR m_surface{};
    VkPhysicalDevice m_physicalDevice{};
    VkDevice m_logicalDevice{};
    VkQueue m_graphicsQueue{}, m_presentQueue{};
    VkCommandPool m_commandPool{};
    QueueFamilyIndices m_familyIndices{};
    SwapchainSupportDetails m_swapchainSupport{};

public:
    explicit Device(Window &window);
    ~Device();

    MUST_USE VkDevice LogicalDevice() const { return m_logicalDevice; }
    MUST_USE VkCommandPool CommandPool() const { return m_commandPool; }
    MUST_USE Window &GetWindow() const { return m_window; }
    MUST_USE VkSurfaceKHR Surface() const { return m_surface; }
    MUST_USE VkQueue GraphicsQueue() const { return m_graphicsQueue; }
    MUST_USE VkQueue PresentQueue() const { return m_presentQueue; }

    MUST_USE SwapchainSupportDetails const &SwapchainSupport() const { return m_swapchainSupport; }

    MUST_USE VkFormat FindSupportedFormat(std::vector<VkFormat> const &canditates, VkImageTiling tiling, VkFormatFeatureFlags features);
    MUST_USE Image CreateImage(VkImageCreateInfo const &info, VkMemoryPropertyFlags properties);

    struct Buffer {
        VkBuffer Buffer;
        VkDeviceMemory Memory;
    };
    MUST_USE Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

private:
    void CreateInstance();
    void SetupDebugCallback();
    void CreateWindowSurface();

    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();
    bool IsDeviceSuitable(VkPhysicalDevice device);

    QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
};
