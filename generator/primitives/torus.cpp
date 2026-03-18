#include "torus.h"
#include <cmath>

// Um torus é parametrizado por dois ângulos:
//   phi   (0..2π) — volta em torno do eixo Y (anel principal)
//   theta (0..2π) — volta em torno do tubo
//
// Posição:
//   P(phi, theta) = ((R + r*cos(theta))*cos(phi),
//                     r*sin(theta),
//                    (R + r*cos(theta))*sin(phi))
//
// Normal (aponta do centro do tubo para fora):
//   N(phi, theta) = (cos(theta)*cos(phi), sin(theta), cos(theta)*sin(phi))
//   (já unitária por construção)
//
// UV:
//   u = phi / (2π)   — posição ao longo do anel
//   v = theta / (2π) — posição ao longo do tubo

Model generateTorus(float outerRadius, float innerRadius, int sides, int rings) {
    Model m;

    const float PI2 = 2.0f * (float)M_PI;
    const float dPhi   = PI2 / rings;
    const float dTheta = PI2 / sides;

    for (int i = 0; i < rings; ++i) {
        float phi0 = i * dPhi;
        float phi1 = phi0 + dPhi;

        float u0 = (float)i / rings;
        float u1 = (float)(i + 1) / rings;

        for (int j = 0; j < sides; ++j) {
            float theta0 = j * dTheta;
            float theta1 = theta0 + dTheta;

            float v0 = (float)j / sides;
            float v1 = (float)(j + 1) / sides;

            // Gera um vértice a partir dos ângulos
            auto makeVert = [&](float phi, float theta, float u, float v) -> Vertex {
                float ct = cosf(theta), st = sinf(theta);
                float cp = cosf(phi),   sp = sinf(phi);

                // Posição
                float px = (outerRadius + innerRadius * ct) * cp;
                float py = innerRadius * st;
                float pz = (outerRadius + innerRadius * ct) * sp;

                // Normal (vetor do centro do tubo para o ponto, normalizado)
                float nx = ct * cp;
                float ny = st;
                float nz = ct * sp;

                return { px, py, pz, nx, ny, nz, u, v };
            };

            Vertex v00 = makeVert(phi0, theta0, u0, v0);
            Vertex v01 = makeVert(phi0, theta1, u0, v1);
            Vertex v10 = makeVert(phi1, theta0, u1, v0);
            Vertex v11 = makeVert(phi1, theta1, u1, v1);

            // Triângulo 1 (CCW visto de fora)
            m.push_back(v00); m.push_back(v01); m.push_back(v11);
            // Triângulo 2
            m.push_back(v00); m.push_back(v11); m.push_back(v10);
        }
    }
    return m;
}