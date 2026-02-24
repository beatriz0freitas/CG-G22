#pragma once
#include "../filewriter.h"

// Gera um cilindro com o eixo em Y, base inferior no plano XZ (y=0) e base superior em y=height.
// radius    = raio
// height    = altura
// slices    = divisões angulares (volta completa)
// stacks    = divisões verticais da superfície lateral
// capped    = true (padrão) inclui as tampas; false só a superfície lateral
Model generateCylinder(float radius, float height, int slices, int stacks, bool capped = true);