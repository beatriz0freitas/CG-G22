#include "cone.h"
#include <cmath>

Model generateCone(float radius, float height, int slices, int stacks) {
    Model m;
    float dTheta = 2.0f * (float)M_PI / slices;
    float dStack = 1.0f / stacks;

    // A normal da superfície lateral de um cone aponta para fora e para baixo.
    // O ângulo de inclinação da normal é determinado pela relação raio/altura.
    // sin(a) = height/slant,  cos(a) = radius/slant
    float slant = sqrtf(radius*radius + height*height);
    float sinA  = radius / slant;   // componente Y da normal (inclinação)
    float cosA  = height / slant;   // componente radial da normal

    // ── Superfície lateral ──
    for (int i = 0; i < stacks; ++i) {
        float t0 = i * dStack,      t1 = t0 + dStack;
        float y0 = height * t0,     y1 = height * t1;
        float r0 = radius * (1-t0), r1 = radius * (1-t1);

        for (int j = 0; j < slices; ++j) {
            float theta0 = j * dTheta;
            float theta1 = theta0 + dTheta;

            // Normal lateral: componente radial * cosA + Y * sinA
            auto makeLatVertex = [&](float r, float y, float theta) -> Vertex {
                float nx = cosA * sinf(theta);
                float ny = sinA;
                float nz = cosA * cosf(theta);
                return { r*sinf(theta), y, r*cosf(theta),
                         nx, ny, nz,
                         0, 0 }; // u,v na Fase 4
            };

            Vertex v00 = makeLatVertex(r0, y0, theta0);
            Vertex v01 = makeLatVertex(r0, y0, theta1);
            Vertex v10 = makeLatVertex(r1, y1, theta0);
            Vertex v11 = makeLatVertex(r1, y1, theta1);

            m.push_back(v00); m.push_back(v01); m.push_back(v11);
            m.push_back(v00); m.push_back(v11); m.push_back(v10);
        }
    }

    // ── Base circular (y=0, normal aponta para -Y) ──
    for (int j = 0; j < slices; ++j) {
        float theta0 = j * dTheta;
        float theta1 = theta0 + dTheta;
        m.push_back({0, 0, 0,                              0,-1,0,  0,0});
        m.push_back({radius*sinf(theta1), 0, radius*cosf(theta1),  0,-1,0,  0,0});
        m.push_back({radius*sinf(theta0), 0, radius*cosf(theta0),  0,-1,0,  0,0});
    }

    return m;
}