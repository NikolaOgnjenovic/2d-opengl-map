#pragma once
#include "shader.h"

void renderCube();

void renderCircle();

void renderCylinder();

void renderLine3D(const Shader &shader, float x1, float z1, float x2, float z2, float thickness = 0.05f);

void cleanupPrimitives();
