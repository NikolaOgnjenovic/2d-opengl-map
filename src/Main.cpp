#include <chrono>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

GLFWcursor *cursor;

struct TextureData {
    unsigned int textureID;
    int width;
    int height;
};

TextureData loadTexture(const char *filePath) {
    TextureData data{};
    data.textureID = loadImageToTexture(filePath);

    int width, height, channels;
    if (unsigned char *imageData = stbi_load(filePath, &width, &height, &channels, 0)) {
        data.width = width;
        data.height = height;
        stbi_image_free(imageData);
    } else {
        data.width = 482;
        data.height = 100;
    }

    return data;
}

void renderImage(const unsigned int shaderProgram, const unsigned int VAO, const unsigned int textureID,
                 const float x, const float y, const float scaleX, const float scaleY) {
    glUseProgram(shaderProgram);

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));

    const int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void renderImageBottomRight(const unsigned int shaderProgram,
                            const unsigned int VAO,
                            const TextureData &tex,
                            const int screenWidth, const int screenHeight) {
    const float quadWidthNDC = static_cast<float>(tex.width) / screenWidth * 2.0f;
    const float quadHeightNDC = static_cast<float>(tex.height) / screenHeight * 2.0f;

    const float scaleX = quadWidthNDC / 2.0f;
    const float scaleY = quadHeightNDC / 2.0f;

    const float posX = 1.0f - scaleX;
    const float posY = -1.0f + scaleY;

    renderImage(shaderProgram, VAO, tex.textureID, posX, posY, scaleX, scaleY);
}

void keyCallback(GLFWwindow *window, const int key, int scancode, const int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // FULLSCREEN SETUP
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Kostur", monitor, nullptr);

    if (!window) {
        return endProgram("Prozor nije uspeo da se kreira.");
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    cursor = loadImageToCursor("../resources/cursors/compass.png");
    glfwSetCursor(window, cursor);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        return endProgram("GLAD nije uspeo da se inicijalizuje.");
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.6f, 0.8f, 0.87f, 1.0f);

    // Load image
    const TextureData cornerImage = loadTexture("../resources/textures/student_info.png");

    const unsigned int shaderProgram = createShader("../resources/shaders/hud.vert", "../resources/shaders/hud.frag");

    // Quad setup
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    constexpr float vertices[] = {
        // positions        // uv
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f
    };

    constexpr unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    int screenWidth, screenHeight;

    constexpr double TARGET_FPS = 75.0;
    constexpr double FRAME_TIME = 1.0 / TARGET_FPS;

    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(window, &screenWidth, &screenHeight);

        renderImageBottomRight(shaderProgram, VAO, cornerImage, screenWidth, screenHeight);

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frameEnd - frameStart;

        if (double sleepTime = FRAME_TIME - elapsed.count(); sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &cornerImage.textureID);

    glfwDestroyCursor(cursor);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
