#include "cylinder.h"
#include <cmath>

// Superfície lateral — normal radial, UV cilíndrica
// Tampa inferior (y=0) — normal (0,-1,0)
// Tampa superior (y=height) — normal (0,1,0)
//
// UV da superfície lateral:
//   u = theta / (2π)   (volta em torno do eixo)
//   v = y / height     (altura normalizada)
//
// UV das tampas:
//   Projeção planar centrada — u = 0.5 + nx*0.5, v = 0.5 + nz*0.5

Model generateCylinder(float radius, float height, int slices, int stacks, bool capped) {
    Model m;
    const float PI2 = 2.0f * (float)M_PI;
    const float dTheta = PI2 / slices;
    const float dStack = height / stacks;

    // ── Superfície lateral ──
    for (int i = 0; i < stacks; ++i) {
        float y0 = i * dStack;
        float y1 = y0 + dStack;
        float tv0 = y0 / height;
        float tv1 = y1 / height;

        for (int j = 0; j < slices; ++j) {
            float theta0 = j * dTheta;
            float theta1 = theta0 + dTheta;
            float tu0 = (float)j / slices;
            float tu1 = (float)(j + 1) / slices;

            auto makeLatVert = [&](float theta, float y, float tu, float tv) -> Vertex {
                float nx = sinf(theta);
                float nz = cosf(theta);
                return { radius * nx, y, radius * nz,
                         nx, 0.0f, nz,
                         tu, tv };
            };

            Vertex v00 = makeLatVert(theta0, y0, tu0, tv0);
            Vertex v01 = makeLatVert(theta1, y0, tu1, tv0);
            Vertex v10 = makeLatVert(theta0, y1, tu0, tv1);
            Vertex v11 = makeLatVert(theta1, y1, tu1, tv1);

            // CCW visto de fora
            m.push_back(v00); m.push_back(v01); m.push_back(v11);
            m.push_back(v00); m.push_back(v11); m.push_back(v10);
        }
    }

    if (!capped) return m;

    // ── Tampa inferior (y=0, normal -Y) ──
    for (int j = 0; j < slices; ++j) {
        float theta0 = j * dTheta;
        float theta1 = theta0 + dTheta;

        float s0 = sinf(theta0), c0 = cosf(theta0);
        float s1 = sinf(theta1), c1 = cosf(theta1);

        // Centro
        Vertex ctr = { 0.0f, 0.0f, 0.0f,
                        0.0f, -1.0f, 0.0f,
                        0.5f, 0.5f };
        Vertex a = { radius*s0, 0, radius*c0,  0,-1,0,  0.5f+s0*0.5f, 0.5f+c0*0.5f };
        Vertex b = { radius*s1, 0, radius*c1,  0,-1,0,  0.5f+s1*0.5f, 0.5f+c1*0.5f };

        // CCW visto de baixo (-Y)
        m.push_back(ctr); m.push_back(b); m.push_back(a);
    }

    // ── Tampa superior (y=height, normal +Y) ──
    for (int j = 0; j < slices; ++j) {
        float theta0 = j * dTheta;
        float theta1 = theta0 + dTheta;

        float s0 = sinf(theta0), c0 = cosf(theta0);
        float s1 = sinf(theta1), c1 = cosf(theta1);

        Vertex ctr = { 0.0f, height, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.5f, 0.5f };
        Vertex a = { radius*s0, height, radius*c0,  0,1,0,  0.5f+s0*0.5f, 0.5f+c0*0.5f };
        Vertex b = { radius*s1, height, radius*c1,  0,1,0,  0.5f+s1*0.5f, 0.5f+c1*0.5f };

        // CCW visto de cima (+Y)
        m.push_back(ctr); m.push_back(a); m.push_back(b);
    }

    return m;
}