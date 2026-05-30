#include "sphere.h"
#include <cmath>

Model generateSphere(float radius, int slices, int stacks, bool invert)
{
    Model m;
    float dPhi = (float)M_PI / stacks;
    float dTheta = 2.0f * (float)M_PI / slices;

    for (int i = 0; i < stacks; ++i)
    {
        float phi0 = -(float)M_PI / 2.0f + i * dPhi;
        float phi1 = phi0 + dPhi;

        for (int j = 0; j < slices; ++j)
        {
            float theta0 = j * dTheta;
            float theta1 = theta0 + dTheta;

            // Para uma esfera centrada na origem, a normal em qualquer ponto é simplesmente a posição normalizada (dividida pelo raio).
            auto makeVertex = [&](float phi, float theta, float u, float v) -> Vertex
            {
                float nx = cosf(phi) * sinf(theta);
                float ny = sinf(phi);
                float nz = cosf(phi) * cosf(theta);
                return {radius * nx, radius * ny, radius * nz,
                        nx, ny, nz,
                        u, v};
            };

            float u0 = (float)j / slices;
            float u1 = (float)(j + 1) / slices;
            float v0 = (float)i / stacks;
            float v1 = (float)(i + 1) / stacks;

            Vertex v00 = makeVertex(phi0, theta0, u0, v0);
            Vertex v01 = makeVertex(phi0, theta1, u1, v0);
            Vertex v10 = makeVertex(phi1, theta0, u0, v1);
            Vertex v11 = makeVertex(phi1, theta1, u1, v1);

            if (invert) {
                // Winding invertido: visível de dentro; normais apontam para o centro
                v00.nx=-v00.nx; v00.ny=-v00.ny; v00.nz=-v00.nz;
                v01.nx=-v01.nx; v01.ny=-v01.ny; v01.nz=-v01.nz;
                v10.nx=-v10.nx; v10.ny=-v10.ny; v10.nz=-v10.nz;
                v11.nx=-v11.nx; v11.ny=-v11.ny; v11.nz=-v11.nz;
                m.push_back(v00); m.push_back(v10); m.push_back(v01);
                m.push_back(v01); m.push_back(v10); m.push_back(v11);
            } else {
                m.push_back(v00); m.push_back(v01); m.push_back(v10);
                m.push_back(v01); m.push_back(v11); m.push_back(v10);
            }
        }
    }
    return m;
}
