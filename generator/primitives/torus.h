#pragma once
#include "../filewriter.h"

// Gera um torus (donut) centrado na origem no plano XZ.
// outerRadius = distância do centro do torus ao centro do tubo
// innerRadius = raio do tubo (innerRadius < outerRadius)
// sides       = divisões em torno do tubo
// rings       = divisões em torno do eixo central
Model generateTorus(float outerRadius, float innerRadius, int sides, int rings);