#include <chrono>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"

GLFWcursor *cursor;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Kostur", nullptr, nullptr);
    if (window == nullptr) {
        return endProgram("Prozor nije uspeo da se kreira.");
    }
    glfwMakeContextCurrent(window);

    cursor = loadImageToCursor("../resources/cursors/compass.png");
    glfwSetCursor(window, cursor);

    // Use glad instead of glew
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        return endProgram("GLAD nije uspeo da se inicijalizuje.");
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.6f, 0.8f, 0.87f, 1.0f);

    constexpr double TARGET_FPS = 75.0;
    constexpr double FRAME_TIME = 1.0 / TARGET_FPS;

    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // --- RENDER ---
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // --- FRAME LIMITER ---
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frameEnd - frameStart;

        if (double sleepTime = FRAME_TIME - elapsed.count(); sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
