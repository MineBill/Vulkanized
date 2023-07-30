#include "Window.h"
#include <exception>

Window::Window(u32 width, u32 height, std::string title) : m_Width(width), m_Height(height), m_Title(std::move(title)) {
    if (glfwInit() != GLFW_TRUE) {
        throw std::exception("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(static_cast<int>(m_Width), static_cast<int>(m_Height), m_Title.c_str(), nullptr, nullptr);
}

void Window::Update() {
    glfwPollEvents();
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(m_Window);
}
