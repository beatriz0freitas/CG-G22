#pragma once
#include "../filewriter.h"
#include <string>

// Gera uma malha a partir de patches de Bézier bicúbicos (formato .patch).
// patchFile = caminho para o ficheiro de patches
// tess      = número de divisões por lado de cada patch
Model generateBezier(const std::string& patchFile, int tess);