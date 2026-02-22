#pragma once
#include <string>
#include <vector>

// Um vértice contém toda a informação geométrica necessária nas 4 fases.
// Fase 1: só x, y, z são usados. nx, ny, nz, u, v ficam a 0.
// Fase 4: normais (nx, ny, nz) e coordenadas de textura (u, v) são preenchidas.
struct Vertex {
    float x,  y,  z;   // posição
    float nx, ny, nz;  // normal       (0 na Fase 1, preenchida na Fase 4)
    float u,  v;       // textura UV   (0 na Fase 1, preenchida na Fase 4)
};

using Model = std::vector<Vertex>;

// Guarda o modelo num ficheiro .3d.
// Formato:
//   <número de vértices>
//   x y z nx ny nz u v
//   x y z nx ny nz u v
//   ...
bool saveToFile(const Model& model, const std::string& filename);