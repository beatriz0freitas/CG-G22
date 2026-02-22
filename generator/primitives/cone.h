#pragma once
#include "../filewriter.h"

// Gera um cone com a base no plano XZ e o vértice em (0, height, 0).
// radius = raio da base
// height = altura
// slices = divisões horizontais (volta completa)
// stacks = divisões verticais da superfície lateral
Model generateCone(float radius, float height, int slices, int stacks);