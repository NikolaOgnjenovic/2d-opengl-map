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

#include "stb_image.h"

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
    float x, y;

    bool operator==(const Point &other) const {
        return std::abs(x - other.x) < 0.001f && std::abs(y - other.y) < 0.001f;
    }
};

struct DigitTextures {
    TextureData digits[10];
    TextureData dot;
};

struct WalkingState {
    float mapPosX;
    float mapPosY;
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

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(scaleX, scaleY, 1.0f));

    const int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
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

void renderPin(const unsigned int shaderProgram, const unsigned int VAO, const unsigned int textureID) {
    constexpr float pinScale = 0.05f;
    renderImage(shaderProgram, VAO, textureID, 0.0f, 0.0f, pinScale, pinScale);
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

void renderLine(const unsigned int shaderProgram, const unsigned int VAO,
                float x1, float y1, float x2, float y2, float thickness = 0.005f) {
    glUseProgram(shaderProgram);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt(dx * dx + dy * dy);
    float angle = std::atan2(dy, dx);

    float midX = (x1 + x2) / 2.0f;
    float midY = (y1 + y2) / 2.0f;

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(midX, midY, 0.0f));
    model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(length, thickness, 1.0f));

    const int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glUniform3f(glGetUniformLocation(shaderProgram, "customColor"), 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useCustomColor"), 1);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glUniform1i(glGetUniformLocation(shaderProgram, "useCustomColor"), 0);
}

void renderPoint(const unsigned int shaderProgram, const unsigned int VAO,
                 float x, float y, float size = 0.02f) {
    glUseProgram(shaderProgram);

    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(size, size, 1.0f));

    const int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glUniform3f(glGetUniformLocation(shaderProgram, "customColor"), 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useCustomColor"), 1);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glUniform1i(glGetUniformLocation(shaderProgram, "useCustomColor"), 0);
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
                              int screenWidth, int screenHeight, float mapScale, float fullscreenScale) {
    float ndcX = static_cast<float>(mouseX) / screenWidth * 2.0f - 1.0f;
    float ndcY = 1.0f - static_cast<float>(mouseY) / screenHeight * 2.0f;

    bool clickedOnExistingPoint = false;
    size_t clickedIndex = 0;

    for (size_t i = 0; i < measuringState.points.size(); ++i) {
        const Point &p = measuringState.points[i];
        float dist = std::sqrt((p.x - ndcX) * (p.x - ndcX) + (p.y - ndcY) * (p.y - ndcY));

        if (dist < 0.03f) {
            clickedOnExistingPoint = true;
            clickedIndex = i;
            break;
        }
    }

    if (clickedOnExistingPoint) {
        // Recalculate total distance from scratch to avoid accumulation errors
        measuringState.totalMeasuredDistance = 0.0f;

        // Remove the clicked point
        measuringState.points.erase(measuringState.points.begin() + clickedIndex);

        // Recalculate total distance for remaining points
        for (size_t i = 1; i < measuringState.points.size(); ++i) {
            const Point &prev = measuringState.points[i - 1];
            const Point &curr = measuringState.points[i];

            float ndcDistance = std::sqrt(
                (prev.x - curr.x) * (prev.x - curr.x) +
                (prev.y - curr.y) * (prev.y - curr.y)
            );

            float convertedDistance = ndcDistance * (mapScale / fullscreenScale);
            measuringState.totalMeasuredDistance += convertedDistance;
        }
    } else {
        measuringState.points.push_back({ndcX, ndcY});

        if (measuringState.points.size() > 1) {
            const Point &prev = measuringState.points[measuringState.points.size() - 2];
            const Point &curr = measuringState.points.back();

            float ndcDistance = std::sqrt(
                (prev.x - curr.x) * (prev.x - curr.x) +
                (prev.y - curr.y) * (prev.y - curr.y)
            );

            float convertedDistance = ndcDistance * (mapScale / fullscreenScale);
            measuringState.totalMeasuredDistance += convertedDistance;
        }
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
                       float &mapPosX, float &mapPosY, float &totalDistanceWalked) {
    if (isWalkingMode) {
        walkingState.mapPosX = mapPosX;
        walkingState.mapPosY = mapPosY;
        walkingState.totalDistance = totalDistanceWalked;

        mapPosX = 0.0f;
        mapPosY = 0.0f;
    } else {
        mapPosX = walkingState.mapPosX;
        mapPosY = walkingState.mapPosY;
        totalDistanceWalked = walkingState.totalDistance;
    }

    isWalkingMode = !isWalkingMode;
}

// ============================================================================
// RENDER MODES
// ============================================================================
void renderWalkingMode(const unsigned int shaderProgram, const unsigned int VAO,
                       const TextureData &bgImage, const TextureData &pinImage,
                       const TextureData &modeIndicator, const DigitTextures &digitTextures,
                       float &mapPosX, float &mapPosY, float &totalDistanceWalked,
                       GLFWwindow *window, int screenWidth, int screenHeight,
                       float mapSpeed, double targetFPS, float mapScale) {
    // Handle movement
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveY = -mapSpeed / targetFPS;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveY = mapSpeed / targetFPS;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveX = mapSpeed / targetFPS;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveX = -mapSpeed / targetFPS;

    mapPosX += moveX;
    mapPosY += moveY;
    totalDistanceWalked += std::sqrt(moveX * moveX + moveY * moveY);

    // Render scene
    renderImage(shaderProgram, VAO, bgImage.textureID, mapPosX, mapPosY, mapScale, mapScale);
    renderPin(shaderProgram, VAO, pinImage.textureID);
    renderModeIndicator(shaderProgram, VAO, modeIndicator, screenWidth, screenHeight);
    renderNumber(shaderProgram, VAO, digitTextures, totalDistanceWalked, -0.95f, 0.9f, 0.05f);
}

void renderMeasuringMode(const unsigned int shaderProgram, const unsigned int VAO,
                         const TextureData &bgImage, const TextureData &modeIndicator,
                         const DigitTextures &digitTextures, MeasuringState &measuringState,
                         GLFWwindow *window, int screenWidth, int screenHeight,
                         float fullscreenScale, float mapScale, bool &leftMousePressed) {
    // Render fullscreen map
    renderImage(shaderProgram, VAO, bgImage.textureID, 0.0f, 0.0f, fullscreenScale, fullscreenScale);
    renderModeIndicator(shaderProgram, VAO, modeIndicator, screenWidth, screenHeight);

    // Render points and lines
    for (size_t i = 0; i < measuringState.points.size(); ++i) {
        const Point &p = measuringState.points[i];
        renderPoint(shaderProgram, VAO, p.x, p.y);

        if (i > 0) {
            const Point &prev = measuringState.points[i - 1];
            renderLine(shaderProgram, VAO, prev.x, prev.y, p.x, p.y);
        }
    }

    renderNumber(shaderProgram, VAO, digitTextures, measuringState.totalMeasuredDistance, -0.95f, 0.9f, 0.05f);

    // Handle mouse input
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftMousePressed) {
        leftMousePressed = true;
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        handleMeasuringModeClick(measuringState, mouseX, mouseY, screenWidth, screenHeight,
                                 mapScale, fullscreenScale);
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
    float mapPosX = 0.0f;
    float mapPosY = 0.0f;
    bool isWalkingMode = true;
    float totalDistanceWalked = 0.0f;

    WalkingState walkingState{};
    MeasuringState measuringState;

    // Timing constants
    constexpr double TARGET_FPS = 75.0;
    constexpr double FRAME_TIME = 1.0 / TARGET_FPS;
    constexpr float MAP_SPEED = 0.4f;
    constexpr float MAP_SCALE = 8.0f;
    constexpr float FULLSCREEN_SCALE = 2.0f;

    // Input state
    static bool leftMousePressed = false;
    double lastSwitchTime = 0.0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        glfwGetWindowSize(window, &screenWidth, &screenHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        // Handle mode switching
        double currentTime = glfwGetTime();
        if (shouldSwitchMode(window, isWalkingMode, currentTime, lastSwitchTime,
                             screenWidth, screenHeight, walkingModeIndicator, measuringModeIndicator)) {
            performModeSwitch(isWalkingMode, walkingState, measuringState,
                              mapPosX, mapPosY, totalDistanceWalked);
            lastSwitchTime = currentTime;
        }

        // Render current mode
        if (isWalkingMode) {
            renderWalkingMode(shaderProgram, VAO, bgImage, pinImage, walkingModeIndicator,
                              digitTextures, mapPosX, mapPosY, totalDistanceWalked,
                              window, screenWidth, screenHeight, MAP_SPEED, TARGET_FPS, MAP_SCALE);
        } else {
            renderMeasuringMode(shaderProgram, VAO, bgImage, measuringModeIndicator,
                                digitTextures, measuringState, window, screenWidth, screenHeight,
                                FULLSCREEN_SCALE, MAP_SCALE, leftMousePressed);
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
