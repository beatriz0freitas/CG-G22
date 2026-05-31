#include "renderer.h"
#include "imageLoader.h"

#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <unordered_map>

namespace std {
template<> struct hash<Vertex> {
    size_t operator()(const Vertex& v) const noexcept {
        size_t s = 0;
        for (float f : {v.x, v.y, v.z, v.nx, v.ny, v.nz, v.u, v.v})
            s ^= hash<float>{}(f) + 0x9e3779b9u + (s << 6) + (s >> 2);
        return s;
    }
};
}

#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#  include <GL/glext.h>
#endif

// ── Estado global do renderer ──
RenderMode g_renderMode = RenderMode::SOLID;
bool       g_showAxes   = true;
float      g_time       = 0.0f;

void toggleRenderMode() {
    switch (g_renderMode) {
        case RenderMode::WIREFRAME:  g_renderMode = RenderMode::SOLID;      break;
        case RenderMode::SOLID:      g_renderMode = RenderMode::SOLID_WIRE; break;
        case RenderMode::SOLID_WIRE: g_renderMode = RenderMode::WIREFRAME;  break;
    }
}

// ── Catmull-Rom ──
// gt = t global em unidades de segmento (0 a n, wraps)
static void catmullRomPoint(const std::vector<Vec3>& pts, float gt,
                             Vec3& pos, Vec3& deriv) {
    int   n   = (int)pts.size();
    int   seg = (int)gt % n;
    float t   = gt - floorf(gt);
    float t2  = t*t, t3 = t2*t;
 
    const Vec3& p0 = pts[((seg - 1) + n) % n];
    const Vec3& p1 = pts[seg];
    const Vec3& p2 = pts[(seg + 1) % n];
    const Vec3& p3 = pts[(seg + 2) % n];
 
    // Posição  q(t) = 0.5*[(-t³+2t²-t)*p0 + (3t³-5t²+2)*p1 + (-3t³+4t²+t)*p2 + (t³-t²)*p3]
    auto crP = [&](float a, float b, float c, float d) {
        return 0.5f * ((-t3 + 2*t2 - t)*a  + (3*t3 - 5*t2 + 2)*b +
                       (-3*t3 + 4*t2 + t)*c + (t3 - t2)*d);
    };
    // Derivada q'(t)
    auto crD = [&](float a, float b, float c, float d) {
        return 0.5f * ((-3*t2 + 4*t - 1)*a + (9*t2 - 10*t)*b +
                       (-9*t2 + 8*t + 1)*c  + (3*t2 - 2*t)*d);
    };
 
    pos   = { crP(p0.x,p1.x,p2.x,p3.x), crP(p0.y,p1.y,p2.y,p3.y), crP(p0.z,p1.z,p2.z,p3.z) };
    deriv = { crD(p0.x,p1.x,p2.x,p3.x), crD(p0.y,p1.y,p2.y,p3.y), crD(p0.z,p1.z,p2.z,p3.z) };
}

static Vec3 cross(const Vec3& a, const Vec3& b) {
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}

static bool normalize(Vec3& v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 1e-6f) return false;
    v = { v.x/len, v.y/len, v.z/len };
    return true;
}

static void applyMaterial(const Material& mat) {
    glMaterialfv(GL_FRONT, GL_DIFFUSE,  mat.diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT,  mat.ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat.specular);
    glMaterialfv(GL_FRONT, GL_EMISSION, mat.emissive);
    glMaterialf(GL_FRONT, GL_SHININESS, mat.shininess);
}

static unsigned int loadTexture(const std::string& filename) {
    static std::map<std::string, unsigned int> cache;

    auto it = cache.find(filename);
    if (it != cache.end())
        return it->second;

    Image img;
    if (!loadJpegImage(filename, img) || img.pixels.empty()) {
        fprintf(stderr, "Aviso: nao foi possivel carregar textura '%s'\n",
                filename.c_str());
        cache[filename] = 0;
        return 0;
    }

    unsigned int texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 img.width, img.height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, img.pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);
    cache[filename] = texId;
    printf("  Textura: '%s' (%dx%d)\n", filename.c_str(), img.width, img.height);
    return texId;
}

static void setupLights(const Scene& scene) {
    if (scene.lights.empty()) {
        glDisable(GL_LIGHTING);
        for (int i = 0; i < 8; ++i)
            glDisable(GL_LIGHT0 + i);
        return;
    }

    glEnable(GL_LIGHTING);

    const GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    for (int i = 0; i < 8; ++i)
        glDisable(GL_LIGHT0 + i);

    for (size_t i = 0; i < scene.lights.size() && i < 8; ++i) {
        GLenum id = GL_LIGHT0 + (GLenum)i;
        const Light& light = scene.lights[i];

        glEnable(id);
        glLightfv(id, GL_DIFFUSE, light.color);
        glLightfv(id, GL_SPECULAR, light.color);
        glLightfv(id, GL_AMBIENT, black);

        if (light.type == LightType::DIRECTIONAL) {
            GLfloat pos[4] = {light.direction.x, light.direction.y,
                              light.direction.z, 0.0f};
            glLightfv(id, GL_POSITION, pos);
            glLightf(id, GL_SPOT_CUTOFF, 180.0f);
        } else {
            GLfloat pos[4] = {light.position.x, light.position.y,
                              light.position.z, 1.0f};
            glLightfv(id, GL_POSITION, pos);

            if (light.type == LightType::SPOT) {
                GLfloat dir[3] = {light.direction.x, light.direction.y,
                                  light.direction.z};
                glLightfv(id, GL_SPOT_DIRECTION, dir);
                glLightf(id, GL_SPOT_CUTOFF, light.cutoff);
            } else {
                glLightf(id, GL_SPOT_CUTOFF, 180.0f);
            }
        }
    }
}

// Constrói uma matriz de rotação (coluna-major) que alinha o eixo X local com a tangente da curva.
static void buildAlignMatrix(const Vec3& T_raw, Vec3& up, float mat[16]) {
    Vec3 X = T_raw;
    if (!normalize(X)) {
        memset(mat, 0, 16*sizeof(float));
        mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
        return;
    }

    Vec3 Z = cross(X, up);
    if (!normalize(Z)) {
        Vec3 fallback = (fabsf(X.y) < 0.99f) ? Vec3{0,1,0} : Vec3{0,0,1};
        Z = cross(X, fallback);
        if (!normalize(Z)) {
            memset(mat, 0, 16*sizeof(float));
            mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
            return;
        }
    }
    Vec3 Y = cross(Z, X);
    normalize(Y);
    up = Y;

    // Matriz coluna-major: cols = X(tangente), Y(cima), Z(lateral).
    mat[0]=X.x; mat[4]=Y.x; mat[8] =Z.x; mat[12]=0;
    mat[1]=X.y; mat[5]=Y.y; mat[9] =Z.y; mat[13]=0;
    mat[2]=X.z; mat[6]=Y.z; mat[10]=Z.z; mat[14]=0;
    mat[3]=0;   mat[7]=0;   mat[11]=0;   mat[15]=1;
}
 

// Desenha eixos XYZ
static void drawAxes() {
    GLboolean lightingWas = glIsEnabled(GL_LIGHTING);
    GLboolean textureWas = glIsEnabled(GL_TEXTURE_2D);
    if (lightingWas) glDisable(GL_LIGHTING);
    if (textureWas) glDisable(GL_TEXTURE_2D);

    glLineWidth(1.0f);

    glBegin(GL_LINES);
        glColor3f(0, 1, 0); glVertex3f(0, -100, 0); glVertex3f(0,  100, 0);
        glColor3f(1, 0, 0); glVertex3f(-100, 0, 0); glVertex3f( 100, 0, 0);
        glColor3f(0, 0, 1); glVertex3f(0, 0, -100); glVertex3f(0, 0,  100);
    glEnd();

    glDisable(GL_LINE_STIPPLE);

    if (textureWas) glEnable(GL_TEXTURE_2D);
    if (lightingWas) glEnable(GL_LIGHTING);
}

struct GeoCache { unsigned int vboId, indexVboId; int vboCount, indexCount; };
static std::map<std::string, GeoCache> s_geoCache;

// Faz upload da geometria de todos os Mesh para VBOs na GPU.
// Meshes com o mesmo ficheiro partilham os mesmos VBOs (zero uploads duplicados).
void buildVBOs(Group& g) {
    for (auto& mesh : g.meshes) {
        if (!mesh.textureFile.empty())
            mesh.textureId = loadTexture(mesh.textureFile);

        if (mesh.verts.empty()) continue;

        auto it = s_geoCache.find(mesh.filename);
        if (it != s_geoCache.end()) {
            mesh.vboId      = it->second.vboId;
            mesh.indexVboId = it->second.indexVboId;
            mesh.vboCount   = it->second.vboCount;
            mesh.indexCount = it->second.indexCount;
            mesh.verts.clear();
            mesh.verts.shrink_to_fit();
            continue;
        }

        // ── Deduplicação de vértices e geração de índices ──
        std::vector<Vertex> deduplicatedVerts;
        std::vector<unsigned int> indices;
        std::unordered_map<Vertex, unsigned int> vertexToIndex;

        for (const auto& vert : mesh.verts) {
            auto jt = vertexToIndex.find(vert);
            if (jt != vertexToIndex.end()) {
                indices.push_back(jt->second);
            } else {
                unsigned int index = (unsigned int)deduplicatedVerts.size();
                deduplicatedVerts.push_back(vert);
                indices.push_back(index);
                vertexToIndex[vert] = index;
            }
        }

        mesh.verts.clear();
        mesh.verts.shrink_to_fit();

        glGenBuffers(1, &mesh.vboId);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(deduplicatedVerts.size() * sizeof(Vertex)),
                     deduplicatedVerts.data(), GL_STATIC_DRAW);
        mesh.vboCount = (int)deduplicatedVerts.size();

        glGenBuffers(1, &mesh.indexVboId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVboId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(indices.size() * sizeof(unsigned int)),
                     indices.data(), GL_STATIC_DRAW);
        mesh.indexCount = (int)indices.size();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        s_geoCache[mesh.filename] = {mesh.vboId, mesh.indexVboId,
                                     mesh.vboCount, mesh.indexCount};
    }

    // Pré-computa as curvas Catmull-Rom em VBOs (só uma vez, não por frame)
    const int CURVE_SAMPLES = 100;
    for (auto& op : g.transforms) {
        if (op.type != TransformType::ANIM_TRANSLATE) continue;
        if (op.points.size() < 4 || op.time <= 0.0f) continue;

        std::vector<float> pts;
        pts.reserve(CURVE_SAMPLES * 3);
        for (int i = 0; i < CURVE_SAMPLES; ++i) {
            float gt = (float)i / CURVE_SAMPLES * (float)op.points.size();
            Vec3 pos, deriv;
            catmullRomPoint(op.points, gt, pos, deriv);
            pts.push_back(pos.x); pts.push_back(pos.y); pts.push_back(pos.z);
        }
        glGenBuffers(1, &op.curveVboId);
        glBindBuffer(GL_ARRAY_BUFFER, op.curveVboId);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(pts.size() * sizeof(float)),
                     pts.data(), GL_STATIC_DRAW);
        op.curveVboCount = CURVE_SAMPLES;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    for (auto& child : g.children)
        buildVBOs(child);
}

// Desenha todos os triângulos de um Group via VBOs com índices
static void renderGroupGeometry(const Group& group, bool useAppearance) {
    GLboolean lightingWas = glIsEnabled(GL_LIGHTING);
    GLboolean textureWas = glIsEnabled(GL_TEXTURE_2D);

    if (!useAppearance) {
        if (lightingWas) glDisable(GL_LIGHTING);
        if (textureWas) glDisable(GL_TEXTURE_2D);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    if (useAppearance)
        glEnableClientState(GL_NORMAL_ARRAY);

    for (const auto& mesh : group.meshes) {
        if (mesh.vboId == 0 || mesh.indexVboId == 0) continue;
        
        // Ativa o VBO de vértices
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex),
                        (void*)offsetof(Vertex, x));

        if (useAppearance) {
            applyMaterial(mesh.material);
            glNormalPointer(GL_FLOAT, sizeof(Vertex),
                            (void*)offsetof(Vertex, nx));

            if (mesh.textureId != 0) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mesh.textureId);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex),
                                  (void*)offsetof(Vertex, u));
            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
        }

        // Ativa o VBO de índices e desenha com glDrawElements
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVboId);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (useAppearance)
        glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (textureWas) glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);

    if (!useAppearance && lightingWas)
        glEnable(GL_LIGHTING);
}

static void renderGroup(const Group& group) {
    glPushMatrix();   //guarda a matriz corrente na pilha

    // Aplica as transforms deste grupo, pela ordem em que estão no XML
    for (auto& op : group.transforms) {
        switch (op.type) {
            case TransformType::TRANSLATE:
                glTranslatef(op.a, op.b, op.c);
                break;
            case TransformType::ROTATE:
                glRotatef(op.a, op.b, op.c, op.d);   // angle, x, y, z
                break;
            case TransformType::SCALE:
                glScalef(op.a, op.b, op.c);
                break;
            case TransformType::ANIM_TRANSLATE: {
                if (op.points.size() < 4 || op.time <= 0.0f) break;

                // Desenha a curva Catmull-Rom a partir do VBO pré-computado
                if (op.curveVboId != 0) {
                    GLboolean lightingWas = glIsEnabled(GL_LIGHTING);
                    GLboolean textureWas  = glIsEnabled(GL_TEXTURE_2D);
                    if (lightingWas) glDisable(GL_LIGHTING);
                    if (textureWas)  glDisable(GL_TEXTURE_2D);

                    glColor3f(1.0f, 1.0f, 0.0f);
                    glBindBuffer(GL_ARRAY_BUFFER, op.curveVboId);
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glVertexPointer(3, GL_FLOAT, 0, nullptr);
                    glDrawArrays(GL_LINE_LOOP, 0, op.curveVboCount);
                    glDisableClientState(GL_VERTEX_ARRAY);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);

                    if (textureWas)  glEnable(GL_TEXTURE_2D);
                    if (lightingWas) glEnable(GL_LIGHTING);
                }

                // Aplica a translação animada
                float t  = fmodf(g_time / op.time, 1.0f);
                float gt = t * op.points.size();
                Vec3 pos, deriv;
                catmullRomPoint(op.points, gt, pos, deriv);
                glTranslatef(pos.x, pos.y, pos.z);
                if (op.align) {
                    float mat[16];
                    buildAlignMatrix(deriv, op.Yant, mat);
                    glMultMatrixf(mat);
                }
                break;
            }
            case TransformType::ANIM_ROTATE: {
                if (op.time <= 0.0f) break;
                float angle = fmodf(g_time / op.time * 360.0f, 360.0f);
                glRotatef(angle, op.b, op.c, op.d);
                break;
            }
        }
    }


    // Desenha a geometria deste grupo
    switch (g_renderMode) {
        case RenderMode::WIREFRAME:
            glColor3f(0.0f, 0.0f, 0.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderGroupGeometry(group, false);
            break;

        case RenderMode::SOLID:
            glColor3f(0.75f, 0.75f, 0.75f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            renderGroupGeometry(group, true);
            break;

        case RenderMode::SOLID_WIRE: {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            glColor3f(0.45f, 0.55f, 0.65f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            renderGroupGeometry(group, true);
            glDisable(GL_POLYGON_OFFSET_FILL);

            glColor3f(0.15f, 0.85f, 0.55f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderGroupGeometry(group, false);
            break;
        }
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Filhos herdam a transform acumulada deste grupo
    for (const auto& child : group.children)
        renderGroup(child);

    glPopMatrix();   // restaura a matriz que existia antes deste grupo
}

void renderScene(const Scene& scene) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    const Camera& c = scene.camera;
    // up = (0,1,0) fixo — seguro porque |beta| < 1.5 rad (ver main.cpp)
    gluLookAt(c.position.x, c.position.y, c.position.z,
              c.lookAt.x,   c.lookAt.y,   c.lookAt.z,
              0.0f, 1.0f, 0.0f);

    setupLights(scene);
    renderGroup(scene.root);

    if (g_showAxes)
        drawAxes();
}
