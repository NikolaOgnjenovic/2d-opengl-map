#include "util3D.h"
#include <vector>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

static unsigned int cubeVAO = 0, cubeVBO = 0;

void renderCube() {
    if (cubeVAO == 0) {
        const float vertices[] = {
            // back face (CCW: bottom-right, top-left, bottom-left, bottom-right, top-right, top-left)
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face (CCW: bottom-left, bottom-right, top-right, bottom-left, top-right, top-left)
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top-left
            // left face (CCW: bottom-left, bottom-right, top-right, bottom-left, top-right, top-left)
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-right
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
            // right face (CCW: bottom-left, bottom-right, top-right, bottom-left, top-right, top-left)
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
            // bottom face (CCW: bottom-left, bottom-right, top-right, bottom-left, top-right, top-left)
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-right
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-right
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-left
            // top face (CCW: bottom-left, bottom-right, top-right, bottom-left, top-right, top-left)
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f // top-right
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void *>(nullptr));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

static unsigned int circleVAO = 0, circleVBO = 0;
static constexpr int circleSegments = 32;

void renderCircle() {
    if (circleVAO == 0) {
        std::vector<float> vertices;
        // Center point
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f); // Pos
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f); // Normal
        vertices.push_back(0.5f);
        vertices.push_back(0.5f); // Tex

        for (int i = 0; i <= circleSegments; i++) {
            const float angle = -2.0f * 3.1415926535f * static_cast<float>(i) / static_cast<float>(circleSegments);
            float x = std::cos(angle);
            float z = std::sin(angle);
            vertices.push_back(x);
            vertices.push_back(0.0f);
            vertices.push_back(z); // Pos
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f); // Normal
            vertices.push_back((x + 1.0f) * 0.5f);
            vertices.push_back((z + 1.0f) * 0.5f); // Tex
        }

        glGenVertexArrays(1, &circleVAO);
        glGenBuffers(1, &circleVBO);
        glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(circleVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void *>(nullptr));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);
    glBindVertexArray(0);
}

static unsigned int cylinderVAO = 0, cylinderVBO = 0;

void renderCylinder() {
    if (cylinderVAO == 0) {
        std::vector<float> vertices;
        for (int i = 0; i <= circleSegments; i++) {
            const float angle = -2.0f * 3.1415926535f * static_cast<float>(i) / static_cast<float>(circleSegments);
            float x = std::cos(angle);
            float z = std::sin(angle);

            // Bottom circle (now first for CCW in strip)
            vertices.push_back(x);
            vertices.push_back(-0.5f);
            vertices.push_back(z);
            vertices.push_back(x);
            vertices.push_back(0.0f);
            vertices.push_back(z);
            vertices.push_back(static_cast<float>(i) / circleSegments);
            vertices.push_back(0.0f);

            // Top circle
            vertices.push_back(x);
            vertices.push_back(0.5f);
            vertices.push_back(z);
            vertices.push_back(x);
            vertices.push_back(0.0f);
            vertices.push_back(z);
            vertices.push_back(static_cast<float>(i) / circleSegments);
            vertices.push_back(1.0f);
        }

        glGenVertexArrays(1, &cylinderVAO);
        glGenBuffers(1, &cylinderVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(cylinderVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void *>(nullptr));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cylinderVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (circleSegments + 1) * 2);
    glBindVertexArray(0);
}

void renderLine3D(const Shader &shader, float x1, float z1, float x2, float z2, float thickness) {
    shader.use();
    float dx = x2 - x1;
    float dz = z2 - z1;
    float length = std::sqrt(dx * dx + dz * dz);
    float angle = std::atan2(dz, dx);
    float midX = (x1 + x2) / 2.0f;
    float midZ = (z1 + z2) / 2.0f;
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(midX, 0.03f, midZ));
    model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(length, 0.01f, thickness));
    shader.setMat4("model", model);
    shader.setVec3("color", 1.0f, 1.0f, 1.0f);
    shader.setBool("useColor", true);
    shader.setBool("isEmissive", true);
    renderCube();
}

void cleanupPrimitives() {
    if (circleVAO != 0) {
        glDeleteVertexArrays(1, &circleVAO);
        glDeleteBuffers(1, &circleVBO);
        circleVAO = 0;
        circleVBO = 0;
    }
    if (cylinderVAO != 0) {
        glDeleteVertexArrays(1, &cylinderVAO);
        glDeleteBuffers(1, &cylinderVBO);
        cylinderVAO = 0;
        cylinderVBO = 0;
    }
    if (cubeVAO != 0) {
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteBuffers(1, &cubeVBO);
        cubeVAO = 0;
        cubeVBO = 0;
    }
}
