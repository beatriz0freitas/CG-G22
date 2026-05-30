#include "plane.h"

Model generatePlane(float length, int divisions) {
    Model m;
    float step = length / divisions;
    float half = length / 2.0f;

    for (int i = 0; i < divisions; ++i) {
        float x0 = -half + i * step, x1 = x0 + step;
        float u0 = (float)i / divisions;
        float u1 = (float)(i + 1) / divisions;
        for (int j = 0; j < divisions; ++j) {
            float z0 = -half + j * step, z1 = z0 + step;
            float v0 = (float)j / divisions;
            float v1 = (float)(j + 1) / divisions;

            // Normal do plano aponta para +Y 
            Vertex A = {x0, 0, z1,  0,1,0,  u0,v1};
            Vertex B = {x1, 0, z1,  0,1,0,  u1,v1};
            Vertex C = {x0, 0, z0,  0,1,0,  u0,v0};
            Vertex D = {x1, 0, z0,  0,1,0,  u1,v0};

            // Face de cima (normal +Y, CCW visto de cima)
            m.push_back(A); m.push_back(B); m.push_back(C);
            m.push_back(C); m.push_back(B); m.push_back(D);

            A.ny = B.ny = C.ny = D.ny = -1.0f;
            // Face de baixo (normal -Y, CCW visto de baixo)
            m.push_back(C); m.push_back(B); m.push_back(A);
            m.push_back(D); m.push_back(B); m.push_back(C);
        }
    }
    return m;
}
