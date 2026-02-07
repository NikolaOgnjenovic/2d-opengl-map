#pragma once
#include "model.h"

class Ground : public Model {
public:
    Ground(float width, float depth, int subdivisions, unsigned int texID);

private:
    void addTexture(unsigned int texID);
    void generateGroundMesh(float width, float depth, int subdivisions);
};