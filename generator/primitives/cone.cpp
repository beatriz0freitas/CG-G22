#include "cone.h"
#include <cmath>

Model generateCone(float radius, float height, int slices, int stacks)
{
    Model m;
    float dTheta = 2.0f * (float)M_PI / slices;
    float dStack = 1.0f / stacks;

    // A normal da superfície lateral de um cone aponta para fora e para baixo.
    // O ângulo de inclinação da normal é determinado pela relação raio/altura.
    // sin(a) = height/slant,  cos(a) = radius/slant
    float slant = sqrtf(radius * radius + height * height);
    float sinA = radius / slant; // componente Y da normal (inclinação)
    float cosA = height / slant; // componente radial da normal

    // ── Superfície lateral ──
    for (int i = 0; i < stacks; ++i)
    {
        float t0 = i * dStack, t1 = t0 + dStack;
        float y0 = height * t0, y1 = height * t1;
        float r0 = radius * (1 - t0), r1 = radius * (1 - t1);

        for (int j = 0; j < slices; ++j)
        {
            float theta0 = j * dTheta;
            float theta1 = theta0 + dTheta;
            float u0 = (float)j / slices;
            float u1 = (float)(j + 1) / slices;

            // Normal lateral: componente radial * cosA + Y * sinA
            auto makeLatVertex = [&](float r, float y, float theta, float u, float v) -> Vertex
            {
                float nx = cosA * sinf(theta);
                float ny = sinA;
                float nz = cosA * cosf(theta);
                return {r * sinf(theta), y, r * cosf(theta),
                        nx, ny, nz,
                        u, v};
            };

            Vertex v00 = makeLatVertex(r0, y0, theta0, u0, t0);
            Vertex v01 = makeLatVertex(r0, y0, theta1, u1, t0);
            Vertex v10 = makeLatVertex(r1, y1, theta0, u0, t1);
            Vertex v11 = makeLatVertex(r1, y1, theta1, u1, t1);

            m.push_back(v00);
            m.push_back(v01);
            m.push_back(v10);
            m.push_back(v01);
            m.push_back(v11);
            m.push_back(v10);
        }
    }

    // ── Base circular (y=0, normal aponta para -Y) ──
    for (int j = 0; j < slices; ++j)
    {
        float theta0 = j * dTheta;
        float theta1 = theta0 + dTheta;
        float s0 = sinf(theta0), c0 = cosf(theta0);
        float s1 = sinf(theta1), c1 = cosf(theta1);

        m.push_back({0, 0, 0, 0, -1, 0, 0.5f, 0.5f});
        m.push_back({radius * s1, 0, radius * c1, 0, -1, 0, 0.5f + s1 * 0.5f, 0.5f + c1 * 0.5f});
        m.push_back({radius * s0, 0, radius * c0, 0, -1, 0, 0.5f + s0 * 0.5f, 0.5f + c0 * 0.5f});
    }

    return m;
}
