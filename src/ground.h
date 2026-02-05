#pragma once
#include "../Header/ground.hpp"

// konstruktor
Ground::Ground(float width, float depth, int subdivisions, unsigned int texID)
    : Model("") // ignorisemo loadModel iz Model klase
{
    meshes.clear();           // obrisemo sve sto je Model eventualno napravio
    addTexture(texID);        // dodajemo teksturu
    generateGroundMesh(width, depth, subdivisions); // generisemo mesh
}

// dodavanje teksture
void Ground::addTexture(unsigned int texID) {
    Texture tex;
    tex.id = texID;
    tex.type = "uDiffMap";
    tex.path = "";
    textures_loaded.push_back(tex);
}

// generisanje mesh-a za ground
void Ground::generateGroundMesh(float width, float depth, int subdivisions) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float dx = width / subdivisions;
    float dz = depth / subdivisions;
    float startX = -width / 2.0f;
    float startZ = -depth / 2.0f;

    for (int i = 0; i <= subdivisions; i++) {
        for (int j = 0; j <= subdivisions; j++) {
            Vertex vertex;
            vertex.Position = glm::vec3(startX + j * dx, 0.0f, startZ + i * dz);
            vertex.Normal = glm::vec3(0, 1, 0);

            float repeat = 1.0f; // broj puta da se tekstura ponovi po kvadratu tj subdivision-u
            vertex.TexCoords = glm::vec2(j * repeat, i * repeat);

            vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < subdivisions; i++) {
        for (int j = 0; j < subdivisions; j++) {
            int row1 = i * (subdivisions + 1);
            int row2 = (i + 1) * (subdivisions + 1);

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