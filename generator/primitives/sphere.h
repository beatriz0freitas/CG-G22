#pragma once
#include "../filewriter.h"

// Gera uma esfera UV centrada na origem.
// radius = raio
// slices = divisões horizontais (volta completa)
// stacks = divisões verticais (polo a polo)
Model generateSphere(float radius, int slices, int stacks);