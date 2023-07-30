#pragma once
#include "Types.h"
#include <string>
#include <GLFW/glfw3.h>

class Window {
    u32 m_Width{0}, m_Height{0};
    std::string m_Title{};
    GLFWwindow *m_Window{};

public:
    Window(u32 width, u32 height, std::string title);

    void Update();
    bool ShouldClose();

    u32 Width() const {return m_Width;}
    u32 Height() const {return m_Height;}

    GLFWwindow* NativeHandle() const {return m_Window;}
};
