#pragma once

#include "tempo.h"
#include <GLFW/glfw3.h>

namespace Tempo {
    class App;

    std::string strToPathFriendly(const std::string& str);
    std::string nameToAppConfigFile(const std::string& name);

    GLFWmonitor* getCurrentMonitor(GLFWwindow* window);

    // To avoid including tempo.h in this include, I am using a void pointer
    // because otherwise, glad.h gets included twice for no reason and it does
    // compile anymore
    void renderApplication(GLFWwindow* window, int width, int height, App* application);
}