#include "plane.h"

Model generatePlane(float length, int divisions) {
    Model m;
    float step = length / divisions;
    float half = length / 2.0f;

    for (int i = 0; i < divisions; ++i) {
        float x0 = -half + i * step, x1 = x0 + step;
        for (int j = 0; j < divisions; ++j) {
            float z0 = -half + j * step, z1 = z0 + step;

            // Normal do plano aponta para +Y — preenchida desde já
            // U,V ficam a 0 na Fase 1, preenchidos na Fase 4
            Vertex A = {x0, 0, z1,  0,1,0,  0,0};
            Vertex B = {x1, 0, z1,  0,1,0,  0,0};
            Vertex C = {x0, 0, z0,  0,1,0,  0,0};
            Vertex D = {x1, 0, z0,  0,1,0,  0,0};

            // Triângulo 1
            m.push_back(A); m.push_back(B); m.push_back(C);
            // Triângulo 2
            m.push_back(C); m.push_back(B); m.push_back(D);
        }
    }
    return m;
}