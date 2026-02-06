#include <chrono>
#include <cmath>
#include <thread>
#include <vector>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "../Header/shader.hpp"
#include "../Header/model.hpp"
#include "../Header/ground.hpp"

// ============================================================================
// GLOBALS & STRUCTS
// ============================================================================
GLFWcursor *cursor;

struct TextureData {
    unsigned int textureID;
    int width;
    int height;
};

struct Point {
    float x, z;

    bool operator==(const Point &other) const {
        return std::abs(x - other.x) < 0.001f && std::abs(z - other.z) < 0.001f;
    }
};

struct DigitTextures {
    TextureData digits[10];
    TextureData dot;
};

struct WalkingState {
    float charPosX;
    float charPosZ;
    float charRotation;
    float totalDistance;
};

struct MeasuringState {
    std::vector<Point> points;
    float totalMeasuredDistance = 0.0f;
};

// ============================================================================
// TEXTURE LOADING
// ============================================================================
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

DigitTextures loadDigitTextures() {
    DigitTextures dt{};
    for (int i = 0; i < 10; ++i) {
        std::string path = "../resources/textures/digits/" + std::to_string(i) + ".png";
        dt.digits[i] = loadTexture(path.c_str());
    }
    dt.dot = loadTexture("../resources/textures/digits/dot.png");
    return dt;
}

// ============================================================================
// RENDERING FUNCTIONS
// ============================================================================
void renderImage(const unsigned int shaderProgram, const unsigned int VAO, const unsigned int textureID,
                 const float x, const float y, const float scaleX, const float scaleY) {
    glUseProgram(shaderProgram);
    glDisable(GL_DEPTH_TEST); // HUD elements shouldn't be depth tested

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));

    const int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "useCustomColor"), 0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void renderImageBottomRight(const unsigned int shaderProgram,
                            const unsigned int VAO,
                            const TextureData &tex,
                            const int screenWidth, const int screenHeight) {
    const float quadWidthNDC = static_cast<float>(tex.width) / screenWidth;
    const float quadHeightNDC = static_cast<float>(tex.height) / screenHeight;

    const float scaleX = quadWidthNDC;
    const float scaleY = quadHeightNDC;

    const float posX = 1.0f - scaleX;
    const float posY = -1.0f + scaleY;

    renderImage(shaderProgram, VAO, tex.textureID, posX, posY, scaleX, scaleY);
}

void renderModeIndicator(const unsigned int shaderProgram,
                         const unsigned int VAO,
                         const TextureData &tex,
                         const int screenWidth, const int screenHeight) {
    const float quadWidthNDC = static_cast<float>(tex.width) / screenWidth;
    const float quadHeightNDC = static_cast<float>(tex.height) / screenHeight;

    const float scaleX = quadWidthNDC;
    const float scaleY = quadHeightNDC;

    const float posX = -1.0f + scaleX;
    const float posY = 1.0f - scaleY;

    renderImage(shaderProgram, VAO, tex.textureID, posX, posY, scaleX, scaleY);
}

void renderNumber(const unsigned int shaderProgram, const unsigned int VAO,
                  const DigitTextures &dt, const float number, const float x, const float y, const float scale) {
    const std::string s = std::to_string(number);
    float offsetX = 0.0f;

    for (const char c: s) {
        if (c >= '0' && c <= '9') {
            const int digit = c - '0';
            renderImage(shaderProgram, VAO, dt.digits[digit].textureID, x + offsetX, y, scale, scale);
            offsetX += scale * 0.6f;
        } else if (c == '.') {
            renderImage(shaderProgram, VAO, dt.dot.textureID, x + offsetX, y, scale, scale);
            offsetX += scale * 0.6f;
        }
    }
}

// Simple Primitive Generator
unsigned int cubeVAO = 0, cubeVBO = 0;
void renderCube() {
    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderLine3D(const Shader &shader, float x1, float z1, float x2, float z2, float thickness = 0.05f) {
    shader.use();
    float dx = x2 - x1;
    float dz = z2 - z1;
    float length = std::sqrt(dx * dx + dz * dz);
    float angle = std::atan2(dz, dx);
    float midX = (x1 + x2) / 2.0f;
    float midZ = (z1 + z2) / 2.0f;
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(midX, 0.01f, midZ));
    model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(length, 0.02f, thickness));
    shader.setMat4("model", model);
    shader.setVec3("color", 1.0f, 1.0f, 1.0f);
    shader.setBool("useColor", true);
    shader.setBool("isEmissive", true);
    renderCube();
}

// ============================================================================
// INPUT & INTERACTION
// ============================================================================
bool isMouseOverIndicator(const double mouseX, const double mouseY, const int screenWidth, const int screenHeight,
                          const TextureData &tex) {
    const float ndcX = static_cast<float>(mouseX) / screenWidth * 2.0f - 1.0f;
    const float ndcY = 1.0f - static_cast<float>(mouseY) / screenHeight * 2.0f;

    const float quadWidthNDC = static_cast<float>(tex.width) / screenWidth;
    const float quadHeightNDC = static_cast<float>(tex.height) / screenHeight;

    const float posX = -1.0f + quadWidthNDC;
    const float posY = 1.0f - quadHeightNDC;

    return ndcX >= (posX - quadWidthNDC) && ndcX <= (posX + quadWidthNDC) &&
           ndcY >= (posY - quadHeightNDC) && ndcY <= (posY + quadHeightNDC);
}

void keyCallback(GLFWwindow *window, const int key, int scancode, const int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// ============================================================================
// MEASURING MODE HELPER FUNCTIONS
// ============================================================================
void handleMeasuringModeClick(MeasuringState &measuringState, double mouseX, double mouseY,
                              int screenWidth, int screenHeight, float mapSize, glm::mat4 view, glm::mat4 projection) {
    // Raycasting to find position on the map (y=0 plane)
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    glm::vec4 ray_clip = glm::vec4(x, y, -1.0, 1.0);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    glm::vec3 camPos = glm::vec3(glm::inverse(view)[3]);
    
    // Intersection with y=0 plane: camPos.y + t * ray_wor.y = 0  => t = -camPos.y / ray_wor.y
    if (std::abs(ray_wor.y) < 0.0001f) return;
    float t = -camPos.y / ray_wor.y;
    if (t < 0) return;
    
    glm::vec3 worldPos = camPos + t * ray_wor;

    // Check if within map bounds
    float halfSize = mapSize / 2.0f;
    if (worldPos.x < -halfSize || worldPos.x > halfSize || worldPos.z < -halfSize || worldPos.z > halfSize) return;

    bool clickedOnExistingPoint = false;
    size_t clickedIndex = 0;

    for (size_t i = 0; i < measuringState.points.size(); ++i) {
        const Point &p = measuringState.points[i];
        float dist = std::sqrt((p.x - worldPos.x) * (p.x - worldPos.x) + (p.z - worldPos.z) * (p.z - worldPos.z));

        if (dist < 0.5f) { // Threshold in world units
            clickedOnExistingPoint = true;
            clickedIndex = i;
            break;
        }
    }

    if (clickedOnExistingPoint) {
        measuringState.points.erase(measuringState.points.begin() + clickedIndex);
    } else {
        measuringState.points.push_back({worldPos.x, worldPos.z});
    }

    // Recalculate total distance
    measuringState.totalMeasuredDistance = 0.0f;
    for (size_t i = 1; i < measuringState.points.size(); ++i) {
        const Point &prev = measuringState.points[i - 1];
        const Point &curr = measuringState.points[i];
        float d = std::sqrt((prev.x - curr.x) * (prev.x - curr.x) + (prev.z - curr.z) * (prev.z - curr.z));
        measuringState.totalMeasuredDistance += d;
    }
}

// ============================================================================
// MODE SWITCHING
// ============================================================================
bool shouldSwitchMode(GLFWwindow *window, bool isWalkingMode, double currentTime,
                      double &lastSwitchTime, int screenWidth, int screenHeight,
                      const TextureData &walkingIndicator, const TextureData &measuringIndicator) {
    bool switchRequested = false;

    // Check keyboard switch
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && currentTime - lastSwitchTime > 0.2) {
        switchRequested = true;
    }

    // Check mouse click on indicator
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && currentTime - lastSwitchTime > 0.2) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        const TextureData &currentIndicator = isWalkingMode ? walkingIndicator : measuringIndicator;
        if (isMouseOverIndicator(mouseX, mouseY, screenWidth, screenHeight, currentIndicator)) {
            switchRequested = true;
        }
    }

    return switchRequested;
}

void performModeSwitch(bool &isWalkingMode, WalkingState &walkingState, MeasuringState &measuringState,
                       float &charPosX, float &charPosZ, float &totalDistanceWalked) {
    if (isWalkingMode) {
        walkingState.charPosX = charPosX;
        walkingState.charPosZ = charPosZ;
        walkingState.totalDistance = totalDistanceWalked;
    } else {
        charPosX = walkingState.charPosX;
        charPosZ = walkingState.charPosZ;
        totalDistanceWalked = walkingState.totalDistance;
    }

    isWalkingMode = !isWalkingMode;
}

// ============================================================================
// RENDER MODES
// ============================================================================
void renderWalkingMode(const Shader &sceneShader, const TextureData &modeIndicator, const unsigned int hudShader, const unsigned int hudVAO, 
                       const DigitTextures &digitTextures,
                       float &charPosX, float &charPosZ, float &charRot, float &totalDistanceWalked,
                       GLFWwindow *window, int screenWidth, int screenHeight,
                       float speed, double targetFPS, float mapSize) {
    
    float oldX = charPosX;
    float oldZ = charPosZ;
    bool moved = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { charPosZ -= speed / targetFPS; charRot = 180.0f; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { charPosZ += speed / targetFPS; charRot = 0.0f; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { charPosX -= speed / targetFPS; charRot = -90.0f; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { charPosX += speed / targetFPS; charRot = 90.0f; moved = true; }

    // Constraints
    float limit = mapSize / 2.0f;
    charPosX = std::clamp(charPosX, -limit, limit);
    charPosZ = std::clamp(charPosZ, -limit, limit);

    if (moved) {
        float dx = charPosX - oldX;
        float dz = charPosZ - oldZ;
        totalDistanceWalked += std::sqrt(dx * dx + dz * dz);
    }

    // Render character (Cube as placeholder)
    sceneShader.use();
    auto modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, glm::vec3(charPosX, 0.5f, charPosZ));
    modelMat = glm::rotate(modelMat, glm::radians(charRot), glm::vec3(0.0f, 1.0f, 0.0f));
    sceneShader.setMat4("model", modelMat);
    sceneShader.setVec3("color", 0.0f, 0.0f, 1.0f);
    sceneShader.setBool("useColor", true);
    renderCube();

    // Render HUD
    renderModeIndicator(hudShader, hudVAO, modeIndicator, screenWidth, screenHeight);
    renderNumber(hudShader, hudVAO, digitTextures, totalDistanceWalked, -0.95f, 0.9f, 0.05f);
}

void renderMeasuringMode(const Shader &sceneShader,
                         const TextureData &modeIndicator, const unsigned int hudShader, const unsigned int hudVAO,
                         const DigitTextures &digitTextures, MeasuringState &measuringState,
                         GLFWwindow *window, int screenWidth, int screenHeight,
                         float mapSize, glm::mat4 view, glm::mat4 projection, bool &leftMousePressed) {
    
    // Render pins and lines
    sceneShader.use();
    for (size_t i = 0; i < measuringState.points.size(); ++i) {
        const Point &p = measuringState.points[i];
        
        // Needle
        auto modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(p.x, 0.25f, p.z));
        modelMat = glm::scale(modelMat, glm::vec3(0.05f, 0.5f, 0.05f));
        sceneShader.setMat4("model", modelMat);
        sceneShader.setVec3("color", 0.5f, 0.5f, 0.5f);
        sceneShader.setBool("useColor", true);
        renderCube();

        // Ball
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(p.x, 0.5f, p.z));
        modelMat = glm::scale(modelMat, glm::vec3(0.2f));
        sceneShader.setMat4("model", modelMat);
        sceneShader.setVec3("color", 1.0f, 0.0f, 0.0f);
        sceneShader.setBool("isEmissive", true);
        renderCube();
        sceneShader.setBool("isEmissive", false);

        // Point light for pin
        std::string prefix = "pointLights[" + std::to_string(i) + "].";
        sceneShader.setVec3(prefix + "position", p.x, 0.5f, p.z);
        sceneShader.setVec3(prefix + "color", 1.0f, 0.0f, 0.0f);
        sceneShader.setFloat(prefix + "intensity", 2.0f);

        if (i > 0) {
            const Point &prev = measuringState.points[i - 1];
            renderLine3D(sceneShader, prev.x, prev.z, p.x, p.z);
        }
    }
    sceneShader.setInt("nrPointLights", static_cast<int>(measuringState.points.size()));

    // Render HUD
    renderModeIndicator(hudShader, hudVAO, modeIndicator, screenWidth, screenHeight);
    renderNumber(hudShader, hudVAO, digitTextures, measuringState.totalMeasuredDistance, -0.95f, 0.9f, 0.05f);

    // Handle mouse input
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftMousePressed) {
        leftMousePressed = true;
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (!isMouseOverIndicator(mouseX, mouseY, screenWidth, screenHeight, modeIndicator)) {
            handleMeasuringModeClick(measuringState, mouseX, mouseY, screenWidth, screenHeight,
                                     mapSize, view, projection);
        }
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        leftMousePressed = false;
    }
}

// ============================================================================
// BUFFER SETUP
// ============================================================================
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    constexpr float vertices[] = {
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ============================================================================
// CLEANUP
// ============================================================================
void cleanupResources(unsigned int VAO, unsigned int VBO, unsigned int EBO, unsigned int shaderProgram,
                      const TextureData &cornerImage, const TextureData &bgImage, const TextureData &pinImage,
                      const TextureData &walkingIndicator, const TextureData &measuringIndicator) {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &cornerImage.textureID);
    glDeleteTextures(1, &bgImage.textureID);
    glDeleteTextures(1, &pinImage.textureID);
    glDeleteTextures(1, &walkingIndicator.textureID);
    glDeleteTextures(1, &measuringIndicator.textureID);

    glfwDestroyCursor(cursor);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================
int main() {
    // Initialize GLFW and create window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Kretanje po mapi", monitor, nullptr);
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

    // Load textures
    const TextureData cornerImage = loadTexture("../resources/textures/student_info.png");
    const TextureData bgImage = loadTexture("../resources/textures/map.jpg");
    const TextureData pinImage = loadTexture("../resources/textures/pin.png");
    const TextureData walkingModeIndicator = loadTexture("../resources/textures/walking.png");
    const TextureData measuringModeIndicator = loadTexture("../resources/textures/ruler.png");
    DigitTextures digitTextures = loadDigitTextures();

    const unsigned int shaderProgram = createShader("../resources/shaders/hud.vert", "../resources/shaders/hud.frag");

    // Setup buffers
    unsigned int VBO, VAO, EBO;
    setupBuffers(VAO, VBO, EBO);

    // Game state
    int screenWidth, screenHeight;
    float charPosX = 0.0f;
    float charPosZ = 0.0f;
    float charRot = 0.0f;
    bool isWalkingMode = true;
    float totalDistanceWalked = 0.0f;

    WalkingState walkingState{};
    MeasuringState measuringState;

    // Timing constants
    constexpr double TARGET_FPS = 75.0;
    constexpr double FRAME_TIME = 1.0 / TARGET_FPS;
    constexpr float MAP_SPEED = 5.0f;
    constexpr float MAP_SIZE = 20.0f;

    // Load 3D Models (Placeholders)
    Ground mapGround(MAP_SIZE, MAP_SIZE, 1, bgImage.textureID);
    // Since we don't have model files yet, we can use simple cubes or 
    // load models if the user provides them. For now I'll use placeholders.
    // I will create a simple cube-based humanoid placeholder if needed.
    
    // For now, let's assume we use renderCube() for char and pins if models not loaded
    // or just use Model with empty path which might fail.
    // Let's use a simple cube for the humanoid and pin ball for now.

    Shader sceneShader("../resources/shaders/scene.vert", "../resources/shaders/scene.frag");
    Shader hudShaderObj("../resources/shaders/hud.vert", "../resources/shaders/hud.frag");

    // Input state
    static bool leftMousePressed = false;
    double lastSwitchTime = 0.0;

    glEnable(GL_DEPTH_TEST);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        glfwGetWindowSize(window, &screenWidth, &screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Handle mode switching
        double currentTime = glfwGetTime();
        if (shouldSwitchMode(window, isWalkingMode, currentTime, lastSwitchTime,
                             screenWidth, screenHeight, walkingModeIndicator, measuringModeIndicator)) {
            performModeSwitch(isWalkingMode, walkingState, measuringState,
                              charPosX, charPosZ, totalDistanceWalked);
            lastSwitchTime = currentTime;
        }

        // Camera setup
        float camY = isWalkingMode ? 5.0f : 15.0f;
        glm::vec3 camPos = glm::vec3(0.0f, camY, 10.0f);
        if (isWalkingMode) {
            camPos = glm::vec3(charPosX, camY, charPosZ + 5.0f);
        }
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(camPos.x, 0.0f, camPos.z - 5.0f), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / screenHeight, 0.1f, 100.0f);

        sceneShader.use();
        sceneShader.setMat4("view", view);
        sceneShader.setMat4("projection", projection);
        sceneShader.setVec3("viewPos", camPos);
        
        // Global light
        sceneShader.setVec3("globalLightPos", 0.0f, 10.0f, 0.0f);
        sceneShader.setVec3("globalLightColor", 1.0f, 1.0f, 1.0f);
        sceneShader.setFloat("globalLightIntensity", 0.5f);

        // Render Ground
        auto modelMat = glm::mat4(1.0f);
        sceneShader.setMat4("model", modelMat);
        sceneShader.setBool("useColor", false);
        sceneShader.setBool("isEmissive", false);
        mapGround.Draw(sceneShader);

        // Render current mode
        if (isWalkingMode) {
            renderWalkingMode(sceneShader, walkingModeIndicator, hudShaderObj.ID, VAO, digitTextures,
                              charPosX, charPosZ, charRot, totalDistanceWalked,
                              window, screenWidth, screenHeight, MAP_SPEED, TARGET_FPS, MAP_SIZE);
        } else {
            renderMeasuringMode(sceneShader, measuringModeIndicator, hudShaderObj.ID, VAO, digitTextures,
                                measuringState, window, screenWidth, screenHeight,
                                MAP_SIZE, view, projection, leftMousePressed);
        }

        // Render UI overlay
        renderImageBottomRight(shaderProgram, VAO, cornerImage, screenWidth, screenHeight);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Frame rate limiting
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frameEnd - frameStart;
        if (double sleepTime = FRAME_TIME - elapsed.count(); sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }
    }

    // Cleanup
    cleanupResources(VAO, VBO, EBO, shaderProgram, cornerImage, bgImage, pinImage,
                     walkingModeIndicator, measuringModeIndicator);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
