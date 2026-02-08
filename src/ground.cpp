#include "ground.h"

Ground::Ground(const float width, const float depth, const int subdivisions, const unsigned int texID)
    : Model("") {
    meshes.clear();
    addTexture(texID);
    generateGroundMesh(width, depth, subdivisions);
}

void Ground::addTexture(const unsigned int texID) {
    Texture tex;
    tex.id = texID;
    tex.type = "uDiffMap";
    tex.path = "";
    textures_loaded.push_back(tex);
}

void Ground::generateGroundMesh(const float width, const float depth, const int subdivisions) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float dx = width / subdivisions;
    const float dz = depth / subdivisions;
    const float startX = -width / 2.0f;
    const float startZ = -depth / 2.0f;

    for (int i = 0; i <= subdivisions; i++) {
        for (int j = 0; j <= subdivisions; j++) {
            Vertex vertex;
            vertex.Position = glm::vec3(startX + j * dx, 0.0f, startZ + i * dz);
            vertex.Normal = glm::vec3(0, 1, 0);

            const float tx = static_cast<float>(j) / subdivisions;
            const float ty = 1.0f - static_cast<float>(i) / subdivisions;
            vertex.TexCoords = glm::vec2(tx, ty);

            vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < subdivisions; i++) {
        for (int j = 0; j < subdivisions; j++) {
            const int row1 = i * (subdivisions + 1);
            const int row2 = (i + 1) * (subdivisions + 1);

            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);

            indices.push_back(row1 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row1 + j + 1);
        }
    }

    meshes.push_back(Mesh(vertices, indices, textures_loaded));
}
