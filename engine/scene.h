#pragma once
#include <string>
#include <vector>

// scene.h — estruturas de dados da scene. A lógica de parsing e de rendering estão noutros ficheiros e não precisam de ser reescritos quando estas estruturas crescem.

// Vec3 — usado para posições e direções (câmara, transforms, etc.)
struct Vec3 { float x, y, z; };

// Vertex — contém toda a informação geométrica por vértice. - Os campos existem desde já para o formato .3d ser consistente nas 4 fases.
struct Vertex {
    float x,  y,  z;   // posição
    float nx, ny, nz;  // normal       (lido do .3d mas não usado até Fase 4)
    float u,  v;       // textura UV   (lido do .3d mas não usado até Fase 4)
};

// Mesh - verts carregados do .3d; após buildVBOs() os dados residem na GPU.
struct Mesh {
    std::string         filename;
    std::vector<Vertex> verts;
    unsigned int        vboId    = 0;
    int                 vboCount = 0;
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
    std::vector<Vec3> points; // pontos de controlo Catmull-Rom
};


// Group - lista plana de meshes, sem transform, sem filhos.
struct Group {
    std::vector<TransformOp> transforms;
    std::vector<Mesh>  meshes;
    std::vector<Group> children;
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
    Group  root;
};