#pragma once
#include "../filewriter.h"

// Gera um plano quadrado no plano XZ, centrado na origem.
// length    = comprimento do lado
// divisions = número de divisões em cada eixo
Model generatePlane(float length, int divisions);