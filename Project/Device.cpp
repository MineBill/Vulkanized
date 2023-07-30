#include "Device.h"
#include "Logger.h"
#include <vector>
#include <vulkan/vk_enum_string_helper.h>

#define VALIDATION_LAYERS
const char *g_RequiredDeviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

std::vector<const char *> GetRequiredExtensions() {
    u32 count;
    auto required_extensions = glfwGetRequiredInstanceExtensions(&count);
    if (required_extensions == nullptr) {
        ERROR("Call to glfwGetRequiredInstanceExtensions failed");
    }
    std::vector<const char *> retval(count);
    for (i32 i = 0; i < count; i++) {
        retval[i] = required_extensions[i];
    }

    retval.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return retval;
}

VkBool32 callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            ERROR(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            WARN(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            DEBUG(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            INFO(pCallbackData->pMessage);
            break;
        default:
            break;
    }
    return true;
}

Device::Device(Window &window) : m_window(window) {
    CreateInstance();
    SetupDebugCallback();
    CreateWindowSurface();
    PickPhysicalDevice();
    m_familyIndices = GetQueueFamilies(m_physicalDevice);
    CreateLogicalDevice();
    CreateCommandPool();
}

Device::~Device() {
    vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
    vkDestroyDevice(m_logicalDevice, nullptr);

#ifdef VALIDATION_LAYERS
    auto DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT"));
    DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
#endif

    vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
}

Device::Buffer Device::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    Buffer buffer{};

    VkBufferCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    auto result = vkCreateBuffer(m_logicalDevice, &info, nullptr, &buffer.Buffer);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create buffer: {}", string_VkResult(result));
    }

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(m_logicalDevice, buffer.Buffer, &requirements);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &physicalDeviceMemoryProperties);
    u32 memoryTypeIndex;
    for (u32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
        }
    }

    VkMemoryAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = memoryTypeIndex,
    };

    result = vkAllocateMemory(m_logicalDevice, &allocateInfo, nullptr, &buffer.Memory);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to allocate buffer memory: {}", string_VkResult(result));
    }

    result = vkBindBufferMemory(m_logicalDevice, buffer.Buffer, buffer.Memory, 0);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to bind buffer memory: {}", string_VkResult(result));
    }
}

void Device::CreateInstance() {
    VkApplicationInfo ApplicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkanized",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3,
    };

    auto extensions = GetRequiredExtensions();

    VkInstanceCreateInfo InstanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &ApplicationInfo,
            .enabledLayerCount = 0,
            .enabledExtensionCount = static_cast<u32>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
    };

#ifdef VALIDATION_LAYERS
    INFO("Setting up validation layers and debug messenger");
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
            .pfnUserCallback = callback,
    };

    const char *VulkanLayers[] = {"VK_LAYER_KHRONOS_validation"};
    InstanceCreateInfo.enabledLayerCount = 1;
    InstanceCreateInfo.ppEnabledLayerNames = VulkanLayers;
    InstanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoExt;
#endif

    INFO("Creating Vulkan instance");
    if (vkCreateInstance(&InstanceCreateInfo, nullptr, &m_vkInstance) != VK_SUCCESS) {
        FATAL("Failed to create Vulkan instance");
    }
    INFO("Vulkan instance created!");
}

void Device::SetupDebugCallback() {
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
            .pfnUserCallback = callback,
    };

    auto CreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT"));
    auto result = CreateDebugUtilsMessengerEXT(m_vkInstance, &debugUtilsMessengerCreateInfoExt, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS) {
        ERROR("Failed to create debug messenger");
    }
}

void Device::CreateWindowSurface() {
    auto result = glfwCreateWindowSurface(m_vkInstance, m_window.NativeHandle(), nullptr, &m_surface);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create window surface: {}", static_cast<int>(result));
    }
}

void Device::PickPhysicalDevice() {
    u32 count;
    vkEnumeratePhysicalDevices(m_vkInstance, &count, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(count);

    vkEnumeratePhysicalDevices(m_vkInstance, &count, physicalDevices.data());

    for (auto device: physicalDevices) {
        if (IsDeviceSuitable(device)) {
            m_physicalDevice = device;
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            INFOF("Selected GPU: {}",  properties.deviceName);
            INFOF("\tDriver version: {}.{}.{}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion));
            INFOF("\tAPI version: {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));
            return;
        }
    }
}

void Device::CreateLogicalDevice() {
    auto uniqueFamilies = m_familyIndices.GetUniqueIndex();

    std::vector<VkDeviceQueueCreateInfo> queueInfos{};

    for (auto &family: uniqueFamilies) {
        f32 queuePriority[]{1.0f};
        queueInfos.emplace_back(VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = family,
                .queueCount = 1,
                .pQueuePriorities = queuePriority,
        });
    }

    VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = static_cast<u32>(queueInfos.size()),
            .pQueueCreateInfos = queueInfos.data(),
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = g_RequiredDeviceExtensions,
            .pEnabledFeatures = nullptr,
    };

    auto result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice);
    if (result != VK_SUCCESS) {
        ERROR("Failed to create logical device");
    }

    vkGetDeviceQueue(m_logicalDevice, *m_familyIndices.GraphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, *m_familyIndices.PresentFamily, 0, &m_presentQueue);
}

void Device::CreateCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = *m_familyIndices.GraphicsFamily,
    };

    auto result = vkCreateCommandPool(m_logicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        ERROR("Failed to create command pool");
    }
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    u32 count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> extensionProperties;
    extensionProperties.resize(count);

    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensionProperties.data());

    for (auto &RequiredDeviceExtension: g_RequiredDeviceExtensions) {
        bool found = false;
        for (auto property: extensionProperties) {
            if (strcmp(RequiredDeviceExtension, property.extensionName) == 0) {
                found = true;
            }
        }

        if (!found) {
            ERRORF("Required device extension '{}' not found.", RequiredDeviceExtension);
            return false;
        }
    }
    return true;
}

bool Device::IsDeviceSuitable(VkPhysicalDevice device) {
    auto indices = GetQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapchainGood = false;
    if (extensionsSupported) {
        m_swapchainSupport = QuerySwapchainSupport(device);
        swapchainGood = !m_swapchainSupport.PresentModes.empty() && !m_swapchainSupport.Formats.empty();
    }
    return indices.IsComplete() && extensionsSupported && swapchainGood;
}

QueueFamilyIndices Device::GetQueueFamilies(VkPhysicalDevice device) {
    u32 count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> familyProperties;
    familyProperties.resize(count);

    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, familyProperties.data());

    QueueFamilyIndices indices;
    u32 index = 0;
    for (auto property: familyProperties) {
        if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.GraphicsFamily = index;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_surface, &presentSupport);
        if (presentSupport) {
            indices.PresentFamily = index;
        }
        if (indices.IsComplete()) {
            break;
        }
        index++;
    }

    return indices;
}

SwapchainSupportDetails Device::QuerySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.Capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.Formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.PresentModes.data());
    }
    return details;
}

VkFormat Device::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (auto format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    ERROR("Failed to find supported format");
    return candidates[0];
}

Image Device::CreateImage(const VkImageCreateInfo &info, VkMemoryPropertyFlags properties) {
    Image image{};
    auto result = vkCreateImage(m_logicalDevice, &info, nullptr, &image.Image);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to create image: {}", string_VkResult(result));
    }

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(m_logicalDevice, image.Image, &requirements);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &physicalDeviceMemoryProperties);

    u32 memoryTypeIndex;
    for (u32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
        }
    }

    VkMemoryAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = memoryTypeIndex
    };

    result = vkAllocateMemory(m_logicalDevice, &allocateInfo, nullptr, &image.Memory);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to allocate image memory: {}", string_VkResult(result));
    }

    result = vkBindImageMemory(m_logicalDevice, image.Image, image.Memory, 0);
    if (result != VK_SUCCESS) {
        ERRORF("Failed to bind image memory: {}", string_VkResult(result));
    }

    return image;
}


std::vector<u32> QueueFamilyIndices::GetUniqueIndex() const {
    // ASSERT(IsComplete(), "");
    if (*GraphicsFamily == *PresentFamily) {
        return {*GraphicsFamily};
    }
    ERROR("Present and Graphics indices differ, do something");
    return {};
}
