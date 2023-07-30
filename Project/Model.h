#pragma once
#include "Device.h"
#include "Types.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>

class Model {
    Device &m_device;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    u32 m_vertexCount;

public:
    struct Vertex {
        glm::vec2 Position;

        static std::vector<VkVertexInputBindingDescription> BindingDescription();
        static std::vector<VkVertexInputAttributeDescription> AttributeDescription();
    };

    Model(Device &device, std::vector<Vertex> const& vertices);
    ~Model();
    Model(Model const &other) = delete;
    Model &operator=(Model const &other) = delete;

    void Bind(VkCommandBuffer commandBuffer);
    void Draw(VkCommandBuffer commandBuffer);

private:
    void CreateVertexBuffers(std::vector<Vertex> const& vertices);
};
