#pragma once
#include "../filewriter.h"

// Gera um cubo centrado na origem.
// size      = comprimento do lado
// divisions = número de divisões por face em cada eixo
Model generateBox(float size, int divisions);