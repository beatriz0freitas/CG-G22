#include "renderer.h"

#include <cmath>
#include <cstring>
#include <map>

#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#  include <GL/glext.h>
#endif

// ── Estado global do renderer ──
RenderMode g_renderMode = RenderMode::WIREFRAME;
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
 
// Constrói uma matriz de rotação (coluna-major) que alinha o eixo Z local com T
static void buildAlignMatrix(const Vec3& T_raw, float mat[16]) {
    float len = sqrtf(T_raw.x*T_raw.x + T_raw.y*T_raw.y + T_raw.z*T_raw.z);
    if (len < 1e-6f) {
        memset(mat, 0, 16*sizeof(float));
        mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
        return;
    }
    Vec3 T = { T_raw.x/len, T_raw.y/len, T_raw.z/len };
 
    // Vetor "cima" que não seja paralelo a T
    Vec3 up = (fabsf(T.y) < 0.99f) ? Vec3{0,1,0} : Vec3{0,0,1};
 
    // Direita = T × up
    Vec3 R = { T.y*up.z - T.z*up.y,
               T.z*up.x - T.x*up.z,
               T.x*up.y - T.y*up.x };
    len = sqrtf(R.x*R.x + R.y*R.y + R.z*R.z);
    R = { R.x/len, R.y/len, R.z/len };
 
    // Cima ortogonal = R × T
    Vec3 U = { R.y*T.z - R.z*T.y,
               R.z*T.x - R.x*T.z,
               R.x*T.y - R.y*T.x };
 
    // Matriz coluna-major: cols = R, U, T
    mat[0]=R.x; mat[4]=U.x; mat[8] =T.x; mat[12]=0;
    mat[1]=R.y; mat[5]=U.y; mat[9] =T.y; mat[13]=0;
    mat[2]=R.z; mat[6]=U.z; mat[10]=T.z; mat[14]=0;
    mat[3]=0;   mat[7]=0;   mat[11]=0;   mat[15]=1;
}
 

// Desenha eixos XYZ
static void drawAxes() {
    glLineWidth(1.0f);

    glBegin(GL_LINES);
        glColor3f(0, 1, 0); glVertex3f(0, -100, 0); glVertex3f(0,  100, 0);
        glColor3f(1, 0, 0); glVertex3f(-100, 0, 0); glVertex3f( 100, 0, 0);
        glColor3f(0, 0, 1); glVertex3f(0, 0, -100); glVertex3f(0, 0,  100);
    glEnd();

    glDisable(GL_LINE_STIPPLE);
}

// Faz upload da geometria de todos os Mesh para VBOs na GPU
void buildVBOs(Group& g) {
    for (auto& mesh : g.meshes) {
        if (mesh.verts.empty()) continue;

        // ── Deduplicação de vértices e geração de índices ──
        std::vector<Vertex> deduplicatedVerts;
        std::vector<unsigned int> indices;
        std::map<Vertex, unsigned int> vertexToIndex;

        for (const auto& vert : mesh.verts) {
            auto it = vertexToIndex.find(vert);
            if (it != vertexToIndex.end()) {
                // Vértice já existe, reutiliza o índice
                indices.push_back(it->second);
            } else {
                // Vértice novo, adiciona à lista e guarda o índice
                unsigned int index = (unsigned int)deduplicatedVerts.size();
                deduplicatedVerts.push_back(vert);
                indices.push_back(index);
                vertexToIndex[vert] = index;
            }
        }

        // ── VBO para vértices únicos ──
        glGenBuffers(1, &mesh.vboId);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(deduplicatedVerts.size() * sizeof(Vertex)),
                     deduplicatedVerts.data(),
                     GL_STATIC_DRAW);
        mesh.vboCount = (int)deduplicatedVerts.size();

        // ── VBO para índices (element array buffer) ──
        glGenBuffers(1, &mesh.indexVboId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVboId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(indices.size() * sizeof(unsigned int)),
                     indices.data(),
                     GL_STATIC_DRAW);
        mesh.indexCount = (int)indices.size();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    for (auto& child : g.children)
        buildVBOs(child);
}

// Desenha todos os triângulos de um Group via VBOs com índices
static void renderGroupGeometry(const Group& group) {
    glEnableClientState(GL_VERTEX_ARRAY);
    for (const auto& mesh : group.meshes) {
        if (mesh.vboId == 0 || mesh.indexVboId == 0) continue;
        
        // Ativa o VBO de vértices
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)0);

        // Ativa o VBO de índices e desenha com glDrawElements
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVboId);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void renderGroup(const Group& group) {
    glPushMatrix();   //guarda a matriz corrente na pilha

    // Aplica as transforms deste grupo, pela ordem em que estão no XML
    for (const auto& op : group.transforms) {
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
                if (op.points.size() < 4) break;

                // Desenha a curva Catmull-Rom como linha fechada
                glColor3f(1.0f, 1.0f, 0.0f);
                glBegin(GL_LINE_LOOP);
                const int CURVE_SAMPLES = 100;
                for (int i = 0; i < CURVE_SAMPLES; ++i) {
                    float ct  = (float)i / CURVE_SAMPLES;
                    float cgt = ct * op.points.size();
                    Vec3 cpos, cder;
                    catmullRomPoint(op.points, cgt, cpos, cder);
                    glVertex3f(cpos.x, cpos.y, cpos.z);
                }
                glEnd();

                // Aplica a translação animada
                float t  = fmodf(g_time / op.time, 1.0f);
                float gt = t * op.points.size();
                Vec3 pos, deriv;
                catmullRomPoint(op.points, gt, pos, deriv);
                glTranslatef(pos.x, pos.y, pos.z);
                if (op.align) {
                    float mat[16];
                    buildAlignMatrix(deriv, mat);
                    glMultMatrixf(mat);
                }
                break;
            }
            case TransformType::ANIM_ROTATE: {
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
            renderGroupGeometry(group);
            break;

        case RenderMode::SOLID:
            glColor3f(0.75f, 0.75f, 0.75f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            renderGroupGeometry(group);
            break;

        case RenderMode::SOLID_WIRE: {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            glColor3f(0.45f, 0.55f, 0.65f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            renderGroupGeometry(group);
            glDisable(GL_POLYGON_OFFSET_FILL);

            glColor3f(0.15f, 0.85f, 0.55f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderGroupGeometry(group);
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

    renderGroup(scene.root);

    if (g_showAxes)
        drawAxes();
}