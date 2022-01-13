#pragma once

#include "tempo.h"
#include <GLFW/glfw3.h>

namespace Tempo {
    GLFWmonitor* getCurrentMonitor(GLFWwindow* window);

    // To avoid including tempo.h in this include, I am using a void pointer
    // because otherwise, glad.h gets included twice for no reason and it does
    // compile anymore
    void renderApplication(GLFWwindow* window, int width, int height, App* application);
}