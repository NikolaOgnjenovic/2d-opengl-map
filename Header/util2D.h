#pragma once
#include <vector>
#include <string>

struct TextureData {
    unsigned int textureID;
    int width;
    int height;
};

struct DigitTextures {
    TextureData digits[10];
    TextureData dot;
};

struct Point {
    float x, z;

    bool operator==(const Point &other) const {
        return std::abs(x - other.x) < 0.001f && std::abs(z - other.z) < 0.001f;
    }
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

TextureData loadTexture(const char *filePath);

DigitTextures loadDigitTextures();

void renderImage(unsigned int shaderProgram, unsigned int VAO, unsigned int textureID, float x, float y, float scaleX,
                 float scaleY);

void renderImageBottomRight(unsigned int shaderProgram, unsigned int VAO, const TextureData &tex,
                            int screenWidth, int screenHeight);

void renderModeIndicator(unsigned int shaderProgram, unsigned int VAO, const TextureData &tex,
                         int screenWidth, int screenHeight);

void renderNumber(unsigned int shaderProgram, unsigned int VAO, const DigitTextures &dt,
                  float number, float x, float y, float scale);
