#include <chrono>
#include <cmath>
#include <thread>
#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "util.h"
#include "util2D.h"
#include "util3D.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "model.h"
#include "ground.h"

GLFWcursor *cursor;

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

    //Testiranje dubine
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        glEnable(GL_DEPTH_TEST); //Ukljucivanje testiranja Z bafera
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        glDisable(GL_DEPTH_TEST);
    }

    //Odstranjivanje lica (Prethodno smo podesili koje lice uklanjamo sa glCullFace)
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        glEnable(GL_CULL_FACE);
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        glDisable(GL_CULL_FACE);
    }
}

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

        if (dist < 0.5f) {
            // Threshold in world units
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

void renderWalkingMode(const Shader &sceneShader, Model &humanModel, const TextureData &modeIndicator,
                       const unsigned int hudShader, const unsigned int hudVAO,
                       const DigitTextures &digitTextures,
                       float &charPosX, float &charPosZ, float &charRot, float &totalDistanceWalked,
                       GLFWwindow *window, int screenWidth, int screenHeight,
                       float speed, double targetFPS, float mapSize) {
    float oldX = charPosX;
    float oldZ = charPosZ;
    bool moved = false;

    float moveX = 0.0f;
    float moveZ = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveZ -= 1.0f;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveZ += 1.0f;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        moveX -= 1.0f;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        moveX += 1.0f;
        moved = true;
    }

    if (moved) {
        float moveLen = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (moveLen > 0.0f) {
            const float angle = std::atan2(moveX, moveZ);
            charRot = glm::degrees(angle);

            const glm::vec2 direction = glm::normalize(glm::vec2(moveX, moveZ));
            charPosX += direction.x * (speed / targetFPS);
            charPosZ += direction.y * (speed / targetFPS);
        }
    }

    // Constraints
    float limit = mapSize / 2.0f;
    charPosX = std::clamp(charPosX, -limit, limit);
    charPosZ = std::clamp(charPosZ, -limit, limit);

    if (moved) {
        float dx = charPosX - oldX;
        float dz = charPosZ - oldZ;
        totalDistanceWalked += std::sqrt(dx * dx + dz * dz);
    }

    // Render character (human Model)
    sceneShader.use();
    sceneShader.setInt("nrPointLights", 0);
    auto modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, glm::vec3(charPosX, 0.6f, charPosZ));
    modelMat = glm::rotate(modelMat, glm::radians(charRot), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(0.5f));
    sceneShader.setMat4("model", modelMat);
    sceneShader.setBool("useColor", false);
    // glDisable(GL_CULL_FACE);
    humanModel.Draw(const_cast<Shader &>(sceneShader));
    // glEnable(GL_CULL_FACE);

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
        modelMat = glm::translate(modelMat, glm::vec3(p.x, 0.28f, p.z));
        modelMat = glm::scale(modelMat, glm::vec3(0.05f, 0.5f, 0.05f));
        sceneShader.setMat4("model", modelMat);
        sceneShader.setVec3("color", 0.5f, 0.5f, 0.5f);
        sceneShader.setBool("useColor", true);
        renderCylinder();

        // Ball (now glowing circle)
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(p.x, 0.52f, p.z));
        modelMat = glm::scale(modelMat, glm::vec3(0.15f)); // Smaller circle and moved up
        sceneShader.setMat4("model", modelMat);
        sceneShader.setVec3("color", 1.0f, 0.0f, 0.0f);
        sceneShader.setBool("isEmissive", true);
        renderCircle();
        sceneShader.setBool("isEmissive", false);

        if (i > 0) {
            const Point &prev = measuringState.points[i - 1];
            renderLine3D(sceneShader, prev.x, prev.z, p.x, p.z);
        }
    }

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

    cleanupPrimitives();

    glfwDestroyCursor(cursor);
}

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

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return endProgram("GLEW nije uspeo da se inicijalizuje.");
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
    float camPosX = 0.0f;
    float camPosZ = 5.0f;
    float charRot = 0.0f;
    bool isWalkingMode = true;
    float totalDistanceWalked = 0.0f;

    WalkingState walkingState{};
    MeasuringState measuringState;

    // Timing constants
    constexpr double TARGET_FPS = 75.0;
    constexpr double FRAME_TIME = 1.0 / TARGET_FPS;
    constexpr float MAP_SIZE = 20.0f;

    // Load 3D Models
    Ground mapGround(MAP_SIZE, MAP_SIZE, 1, bgImage.textureID);
    Model humanModel("../resources/models/Hercules.obj");

    Shader sceneShader("../resources/shaders/scene.vert", "../resources/shaders/scene.frag");
    Shader hudShaderObj("../resources/shaders/hud.vert", "../resources/shaders/hud.frag");

    // Input state
    static bool leftMousePressed = false;
    double lastSwitchTime = 0.0;

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glCullFace(GL_BACK);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        constexpr float MAP_SPEED = 5.0f;
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

        // Camera movement with arrow keys
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camPosZ -= MAP_SPEED / TARGET_FPS;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camPosZ += MAP_SPEED / TARGET_FPS;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camPosX -= MAP_SPEED / TARGET_FPS;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camPosX += MAP_SPEED / TARGET_FPS;

        // Camera constraints
        float limit = MAP_SIZE / 2.0f;
        camPosX = std::clamp(camPosX, -limit, limit);
        camPosZ = std::clamp(camPosZ, -limit + 5.0f, limit + 5.0f);

        // Camera setup
        float camY = isWalkingMode ? 5.0f : 15.0f;
        auto camPos = glm::vec3(camPosX, camY, camPosZ);
        auto lookAtTarget = glm::vec3(camPos.x, 0.0f, camPos.z - 5.0f);
        glm::mat4 view = glm::lookAt(camPos, lookAtTarget, glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(screenWidth) / screenHeight,
                                                0.1f, 100.0f);

        sceneShader.use();
        sceneShader.setMat4("view", view);
        sceneShader.setMat4("projection", projection);
        sceneShader.setVec3("viewPos", camPos);

        // Global light
        sceneShader.setVec3("globalLightPos", 0.0f, 10.0f, 0.0f);
        sceneShader.setVec3("globalLightColor", 1.0f, 1.0f, 1.0f);
        sceneShader.setFloat("globalLightIntensity", 1.0f);

        if (isWalkingMode) {
            sceneShader.setInt("nrPointLights", 0);
        } else {
            sceneShader.setInt("nrPointLights", static_cast<int>(measuringState.points.size()));
            for (size_t i = 0; i < measuringState.points.size(); ++i) {
                const Point &p = measuringState.points[i];
                std::string prefix = "pointLights[" + std::to_string(i) + "].";
                sceneShader.setVec3(prefix + "position", p.x, 0.5f, p.z);
                sceneShader.setVec3(prefix + "color", 1.0f, 0.0f, 0.0f);
                sceneShader.setFloat(prefix + "intensity", 2.0f);
            }
        }

        // Render Ground
        auto modelMat = glm::mat4(1.0f);
        sceneShader.setMat4("model", modelMat);
        sceneShader.setBool("useColor", false);
        sceneShader.setBool("isEmissive", false);
        mapGround.Draw(sceneShader);

        // Render current mode
        if (isWalkingMode) {
            renderWalkingMode(sceneShader, humanModel, walkingModeIndicator, hudShaderObj.ID, VAO, digitTextures,
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

    cleanupResources(VAO, VBO, EBO, shaderProgram, cornerImage, bgImage, pinImage,
                     walkingModeIndicator, measuringModeIndicator);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
