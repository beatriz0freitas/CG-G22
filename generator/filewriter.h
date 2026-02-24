#pragma once
#include <string>
#include <vector>

// Um vértice contém toda a informação geométrica necessária nas 4 fases - Fase 1: só x, y, z são usados. nx, ny, nz, u, v ficam a 0.
struct Vertex {
    float x,  y,  z;   // posição
    float nx, ny, nz;  // normal      
    float u,  v;       // textura UV  
};

using Model = std::vector<Vertex>;

// Guarda o modelo num ficheiro .3d.
// Formato:
//   <número de vértices>
//   x y z nx ny nz u v
//   x y z nx ny nz u v
//   ...
bool saveToFile(const Model& model, const std::string& filename);