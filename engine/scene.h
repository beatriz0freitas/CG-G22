#pragma once
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────
// scene.h — estruturas de dados da cena.
// Este ficheiro cresce fase a fase. A lógica de parsing e de rendering
// estão noutros ficheiros e não precisam de ser reescritos quando
// estas estruturas crescem.
// ─────────────────────────────────────────────────────────────────────

// Vec3 — usado para posições e direções (câmara, transforms, etc.)
struct Vec3 { float x, y, z; };

// Vertex — contém toda a informação geométrica por vértice.
// Fase 1: só x, y, z são usados pelo renderer.
// Fase 4: nx, ny, nz (normais) e u, v (textura) passam a ser usados.
// Os campos existem desde já para o formato .3d ser consistente nas 4 fases.
struct Vertex {
    float x,  y,  z;   // posição
    float nx, ny, nz;  // normal       (lido do .3d mas não usado até Fase 4)
    float u,  v;       // textura UV   (lido do .3d mas não usado até Fase 4)
};

// ── Mesh ─────────────────────────────────────────────────────────────
// Fase 1: verts carregados do .3d, apenas posição usada no rendering.
// Fase 4: nx, ny, nz e u, v passam a ser passados ao OpenGL.
struct Mesh {
    std::string        filename;
    std::vector<Vertex> verts;
};

// ── Transform ────────────────────────────────────────────────────────
// Fase 1: não usado (identidade implícita).
// Fase 2: translate/rotate/scale estáticos.
// Fase 3: translate com curva Catmull-Rom, rotate com tempo.
struct Transform {
    // Fase 2 →
    // Vec3  translate = {0,0,0};
    // Vec3  rotAxis   = {0,1,0};
    // float rotAngle  = 0.0f;
    // Vec3  scale     = {1,1,1};

    // Fase 3 →
    // float             transTime = 0;
    // bool              align     = false;
    // std::vector<Vec3> catmullPoints;
    // float             rotTime   = 0;
};

// ── Group ────────────────────────────────────────────────────────────
// Fase 1: lista plana de meshes, sem transform, sem filhos.
// Fase 2: transform + filhos (hierarquia).
struct Group {
    Transform          transform;  // ignorado na Fase 1
    std::vector<Mesh>  meshes;
    std::vector<Group> children;   // usados na Fase 2
};

// ── Camera ───────────────────────────────────────────────────────────
struct Camera {
    Vec3  position = {3, 2, 1};
    Vec3  lookAt   = {0, 0, 0};
    Vec3  up       = {0, 1, 0};
    float fov      = 60.0f;
    float nearP    = 1.0f;
    float farP     = 1000.0f;
};

// ── Light ────────────────────────────────────────────────────────────
// Fase 4 →
// struct Light {
//     enum class Type { POINT, DIRECTIONAL, SPOT } type;
//     Vec3  pos, dir;
//     float cutoff;
// };

// ── Scene ────────────────────────────────────────────────────────────
struct Scene {
    int    winW = 512;
    int    winH = 512;
    Camera camera;
    Group  root;
    // Fase 4 → std::vector<Light> lights;
};