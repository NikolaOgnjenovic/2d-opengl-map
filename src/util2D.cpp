#include "util2D.h"
#include "util.h"
#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

TextureData loadTexture(const char *filePath) {
    TextureData data{};
    stbi_set_flip_vertically_on_load(true);
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

void renderImage(const unsigned int shaderProgram, const unsigned int VAO, const unsigned int textureID,
                 const float x, const float y, const float scaleX, const float scaleY) {
    glUseProgram(shaderProgram);
    glDisable(GL_DEPTH_TEST);

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

void renderImageBottomRight(const unsigned int shaderProgram, const unsigned int VAO, const TextureData &tex,
                            const int screenWidth, const int screenHeight) {
    const float quadWidthNDC = static_cast<float>(tex.width) / screenWidth;
    const float quadHeightNDC = static_cast<float>(tex.height) / screenHeight;

    const float scaleX = quadWidthNDC;
    const float scaleY = quadHeightNDC;

    const float posX = 1.0f - scaleX;
    const float posY = -1.0f + scaleY;

    renderImage(shaderProgram, VAO, tex.textureID, posX, posY, scaleX, scaleY);
}

void renderModeIndicator(const unsigned int shaderProgram, const unsigned int VAO, const TextureData &tex,
                         const int screenWidth, const int screenHeight) {
    const float quadWidthNDC = static_cast<float>(tex.width) / screenWidth;
    const float quadHeightNDC = static_cast<float>(tex.height) / screenHeight;

    const float scaleX = quadWidthNDC;
    const float scaleY = quadHeightNDC;

    const float posX = -1.0f + scaleX;
    const float posY = 1.0f - scaleY;

    renderImage(shaderProgram, VAO, tex.textureID, posX, posY, scaleX, scaleY);
}

void renderNumber(const unsigned int shaderProgram, const unsigned int VAO, const DigitTextures &dt,
                  const float number, const float x, const float y, const float scale) {
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
