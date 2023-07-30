#include "Model.h"

Model::Model(Device &device, std::vector<Vertex> const &vertices) : m_device(device)
{
    CreateVertexBuffers(vertices);
}

Model::~Model()
{
    vkDestroyBuffer(m_device.LogicalDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(m_device.LogicalDevice(), m_vertexBufferMemory, nullptr);
}

void Model::Bind(VkCommandBuffer commandBuffer)
{
    VkBuffer buffers[] = {
            m_vertexBuffer,
    };

    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Model::Draw(VkCommandBuffer commandBuffer)
{
    vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
}

void Model::CreateVertexBuffers(const std::vector<Vertex> &vertices)
{
    m_vertexCount = static_cast<u32>(vertices.size());
    VkDeviceSize bufferSize = sizeof(Vertex) * m_vertexCount;

    auto buffer = m_device.CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    m_vertexBuffer = buffer.Buffer;
    m_vertexBufferMemory = buffer.Memory;

    void *data;
    vkMapMemory(m_device.LogicalDevice(), m_vertexBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device.LogicalDevice(), m_vertexBufferMemory);
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::BindingDescription()
{
    return {{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }};
}
std::vector<VkVertexInputAttributeDescription> Model::Vertex::AttributeDescription()
{
    return {{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0,
    }};
}
