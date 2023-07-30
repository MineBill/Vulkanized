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
}

void Model::Draw(VkCommandBuffer commandBuffer)
{
}

void Model::CreateVertexBuffers(const std::vector<Vertex> &vertices)
{
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::BindingDescription()
{
}
std::vector<VkVertexInputAttributeDescription> Model::Vertex::AttributeDescription()
{
}
