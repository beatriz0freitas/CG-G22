#include "box.h"

// Emite um quad com a normal explícita para cada face
static void quad(Model& m,
                 Vertex a, Vertex b, Vertex c, Vertex d) {
    m.push_back(a); m.push_back(b); m.push_back(c);
    m.push_back(c); m.push_back(b); m.push_back(d);
}

Model generateBox(float size, int divisions) {
    Model m;
    float step = size / divisions;
    float h    = size / 2.0f;

    for (int i = 0; i < divisions; ++i) {
        float a = -h + i * step, b = a + step;
        for (int j = 0; j < divisions; ++j) {
            float c = -h + j * step, d = c + step;

            // Cada face tem a sua normal a apontar para fora.
            // U,V ficam a 0 na Fase 1, preenchidos na Fase 4.
            // Topo (+Y)
            quad(m, {a,h,d, 0,1,0, 0,0}, {b,h,d, 0,1,0, 0,0},
                    {a,h,c, 0,1,0, 0,0}, {b,h,c, 0,1,0, 0,0});
            // Fundo (-Y)
            quad(m, {b,-h,d, 0,-1,0, 0,0}, {a,-h,d, 0,-1,0, 0,0},
                    {b,-h,c, 0,-1,0, 0,0}, {a,-h,c, 0,-1,0, 0,0});
            // Frente (+Z)
            quad(m, {a,c,h, 0,0,1, 0,0}, {b,c,h, 0,0,1, 0,0},
                    {a,d,h, 0,0,1, 0,0}, {b,d,h, 0,0,1, 0,0});
            // Trás (-Z)
            quad(m, {b,c,-h, 0,0,-1, 0,0}, {a,c,-h, 0,0,-1, 0,0},
                    {b,d,-h, 0,0,-1, 0,0}, {a,d,-h, 0,0,-1, 0,0});
            // Direita (+X)
            quad(m, {h,c,b, 1,0,0, 0,0}, {h,c,a, 1,0,0, 0,0},
                    {h,d,b, 1,0,0, 0,0}, {h,d,a, 1,0,0, 0,0});
            // Esquerda (-X)
            quad(m, {-h,c,a, -1,0,0, 0,0}, {-h,c,b, -1,0,0, 0,0},
                    {-h,d,a, -1,0,0, 0,0}, {-h,d,b, -1,0,0, 0,0});
        }
    }
    return m;
}