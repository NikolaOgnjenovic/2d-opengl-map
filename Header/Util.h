#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
int endProgram(std::string message);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);