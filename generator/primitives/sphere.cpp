#include "sphere.h"
#include <cmath>

Model generateSphere(float radius, int slices, int stacks)
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
            auto makeVertex = [&](float phi, float theta) -> Vertex
            {
                float nx = cosf(phi) * sinf(theta);
                float ny = sinf(phi);
                float nz = cosf(phi) * cosf(theta);
                return {radius * nx, radius * ny, radius * nz,
                        nx, ny, nz,
                        0, 0};
            };

            Vertex v00 = makeVertex(phi0, theta0);
            Vertex v01 = makeVertex(phi0, theta1);
            Vertex v10 = makeVertex(phi1, theta0);
            Vertex v11 = makeVertex(phi1, theta1);

            m.push_back(v00);
            m.push_back(v01);
            m.push_back(v11);
            m.push_back(v00);
            m.push_back(v11);
            m.push_back(v10);
        }
    }
    return m;
}