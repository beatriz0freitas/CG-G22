#pragma once
#include <string>
#include <vector>

// scene.h — estruturas de dados da scene. A lógica de parsing e de rendering estão noutros ficheiros e não precisam de ser reescritos quando estas estruturas crescem.

// Vec3 — usado para posições e direções (câmara, transforms, etc.)
struct Vec3 { float x, y, z; };

// Vertex — contém toda a informação geométrica por vértice.
struct Vertex {
    float x,  y,  z;   // posição
    float nx, ny, nz;  // normal
    float u,  v;       // textura UV

    bool operator==(const Vertex& o) const {
        return x==o.x && y==o.y && z==o.z &&
               nx==o.nx && ny==o.ny && nz==o.nz &&
               u==o.u && v==o.v;
    }
    bool operator<(const Vertex& other) const {
    if (x != other.x) return x < other.x;
    if (y != other.y) return y < other.y;
    if (z != other.z) return z < other.z;
    if (nx != other.nx) return nx < other.nx;
    if (ny != other.ny) return ny < other.ny;
    if (nz != other.nz) return nz < other.nz;
    if (u != other.u) return u < other.u;
    return v < other.v;
}
};

struct Material {
    float diffuse[4]  = {200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.0f};
    float ambient[4]  = { 50.0f / 255.0f,  50.0f / 255.0f,  50.0f / 255.0f, 1.0f};
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float emissive[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float shininess   = 0.0f;
};

// Mesh - verts carregados do .3d; após buildVBOs() os dados residem na GPU.
struct Mesh {
    std::string         filename;
    std::vector<Vertex> verts;
    Material            material;
    std::string         textureFile;
    unsigned int        textureId  = 0;
    unsigned int        vboId      = 0;   // Buffer com vértices únicos
    int                 vboCount   = 0;   // Número de vértices únicos
    unsigned int        indexVboId = 0;   // Buffer com índices (elemento array buffer)
    int                 indexCount = 0;   // Número de índices (3x número de triângulos)
};

enum class TransformType {
    TRANSLATE,        // estático: a=x, b=y, c=z
    ROTATE,           // estático: a=angle, b=x, c=y, d=z
    SCALE,            // estático: a=x, b=y, c=z
    ANIM_TRANSLATE,   // Catmull-Rom: time + align + points
    ANIM_ROTATE,      // rotação contínua: time + eixo (b,c,d)
};

struct TransformOp {
    TransformType     type;
    float a = 0, b = 0, c = 0, d = 0;
    float time  = 0;          // duração de um ciclo completo (segundos)
    bool  align = false;      // orientar o objeto ao longo da curva
    mutable Vec3 Yant = {0, 1, 0};   // Y_{i-1} do referencial de alinhamento
    std::vector<Vec3> points; // pontos de controlo Catmull-Rom
    unsigned int curveVboId  = 0;   // VBO pré-computado da curva (visualização)
    int          curveVboCount = 0;
};


// Group - lista plana de meshes, sem transform, sem filhos.
struct Group {
    std::vector<TransformOp> transforms;
    std::vector<Mesh>  meshes;
    std::vector<Group> children;
};

enum class LightType {
    POINT,
    DIRECTIONAL,
    SPOT
};

struct Light {
    LightType type = LightType::POINT;
    Vec3 position  = {0, 0, 0};
    Vec3 direction = {0, -1, 0};
    float cutoff   = 180.0f;
};

struct Camera {
    Vec3  position = {3, 2, 1};
    Vec3  lookAt   = {0, 0, 0};
    Vec3  up       = {0, 1, 0};
    float fov      = 60.0f;
    float nearP    = 1.0f;
    float farP     = 1000.0f;
};

struct Scene {
    int    winW = 512;
    int    winH = 512;
    Camera camera;
    std::vector<Light> lights;
    Group  root;
};
