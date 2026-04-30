#include "bezier.h"
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <stdexcept>

// ── Bases de Bernstein de grau 3 e as suas derivadas ──
static float B(int i, float t) {
    float u = 1.0f - t;
    switch (i) {
        case 0: return u*u*u;
        case 1: return 3.0f*t*u*u;
        case 2: return 3.0f*t*t*u;
        case 3: return t*t*t;
    }
    return 0.0f;
}

static float dB(int i, float t) {
    float u = 1.0f - t;
    switch (i) {
        case 0: return -3.0f*u*u;
        case 1: return  3.0f*u*u - 6.0f*t*u;
        case 2: return  6.0f*t*u - 3.0f*t*t;
        case 3: return  3.0f*t*t;
    }
    return 0.0f;
}

struct V3 { float x, y, z; };

// Avalia posição e normal num ponto (u,v) do patch
static void evalPatch(const V3 cp[4][4], float u, float v,
                      V3& pos, V3& nor) {
    pos = {0,0,0};
    V3 du{0,0,0}, dv{0,0,0};

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float bib = B(i,u) * B(j,v);
            float dib = dB(i,u) * B(j,v);
            float bdi = B(i,u) * dB(j,v);
            pos.x += bib*cp[i][j].x;  pos.y += bib*cp[i][j].y;  pos.z += bib*cp[i][j].z;
            du.x  += dib*cp[i][j].x;  du.y  += dib*cp[i][j].y;  du.z  += dib*cp[i][j].z;
            dv.x  += bdi*cp[i][j].x;  dv.y  += bdi*cp[i][j].y;  dv.z  += bdi*cp[i][j].z;
        }
    }

    // normal = du × dv
    nor.x = du.y*dv.z - du.z*dv.y;
    nor.y = du.z*dv.x - du.x*dv.z;
    nor.z = du.x*dv.y - du.y*dv.x;
    float len = sqrtf(nor.x*nor.x + nor.y*nor.y + nor.z*nor.z);
    if (len > 1e-6f) { nor.x /= len; nor.y /= len; nor.z /= len; }
}

Model generateBezier(const std::string& patchFile, int tess) {
    std::ifstream f(patchFile);
    if (!f) throw std::runtime_error("Nao foi possivel abrir: " + patchFile);

    // ── Leitura do numero de patches e indices ──
    int nPatches;
    f >> nPatches;

    std::vector<std::array<int,16>> patches(nPatches);
    for (int i = 0; i < nPatches; ++i) {
        char c;
        for (int j = 0; j < 16; ++j) {
            f >> patches[i][j];
            if (j < 15) f >> c;   // vírgula
        }
    }

    // ── Leitura dos pontos de controlo ──
    int nPoints;
    f >> nPoints;

    std::vector<V3> cpts(nPoints);
    for (int i = 0; i < nPoints; ++i) {
        char c;
        f >> cpts[i].x >> c >> cpts[i].y >> c >> cpts[i].z;
    }

    // ── Tessellação ──
    Model m;
    float step = 1.0f / tess;

    for (const auto& patch : patches) {
        // Monta a grelha 4×4 de pontos de controlo do patch
        V3 cp[4][4];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                cp[i][j] = cpts[patch[i*4 + j]];

        for (int ui = 0; ui < tess; ++ui) {
            float u0 = ui*step, u1 = u0+step;
            for (int vi = 0; vi < tess; ++vi) {
                float v0 = vi*step, v1 = v0+step;

                V3 p00,p01,p10,p11, n00,n01,n10,n11;
                evalPatch(cp, u0, v0, p00, n00);
                evalPatch(cp, u0, v1, p01, n01);
                evalPatch(cp, u1, v0, p10, n10);
                evalPatch(cp, u1, v1, p11, n11);

                auto mkv = [](V3 p, V3 n, float u, float v) -> Vertex {
                    return {p.x,p.y,p.z, n.x,n.y,n.z, u, v};
                };

                // Dois triângulos por quad (CCW)
                m.push_back(mkv(p00, n00, u0, v0));
                m.push_back(mkv(p10, n10, u1, v0));
                m.push_back(mkv(p11, n11, u1, v1));

                m.push_back(mkv(p00, n00, u0, v0));
                m.push_back(mkv(p11, n11, u1, v1));
                m.push_back(mkv(p01, n01, u0, v1));
            }
        }
    }
    return m;
}