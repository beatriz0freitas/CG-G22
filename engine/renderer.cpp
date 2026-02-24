#include "renderer.h"

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

// ── Estado global do renderer ──
RenderMode g_renderMode = RenderMode::WIREFRAME;
bool       g_showAxes   = true;

void toggleRenderMode() {
    switch (g_renderMode) {
        case RenderMode::WIREFRAME:  g_renderMode = RenderMode::SOLID;      break;
        case RenderMode::SOLID:      g_renderMode = RenderMode::SOLID_WIRE; break;
        case RenderMode::SOLID_WIRE: g_renderMode = RenderMode::WIREFRAME;  break;
    }
}

// Desenha eixos XYZ
static void drawAxes(float len = 2.0f) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        // X — vermelho
        glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(len,0,0);
        // Y — verde
        glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,len,0);
        // Z — azul
        glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,len);
    glEnd();
    glLineWidth(1.0f);
}

// Desenha todos os triângulos de um Group (e filhos, recursivamente)
static void renderGroupGeometry(const Group& group) {
    for (const auto& mesh : group.meshes) {
        glBegin(GL_TRIANGLES);
        for (const auto& v : mesh.verts)
            glVertex3f(v.x, v.y, v.z);
        glEnd();
    }
    for (const auto& child : group.children)
        renderGroupGeometry(child);
}

static void renderGroup(const Group& group) {
    switch (g_renderMode) {

        case RenderMode::WIREFRAME:
            glColor3f(0.0f, 0.0f, 0.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderGroupGeometry(group);
            break;

        case RenderMode::SOLID:
            // Cor cinzento claro, modo sólido
            glColor3f(0.75f, 0.75f, 0.75f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            renderGroupGeometry(group);
            break;

        case RenderMode::SOLID_WIRE: {
            // Passe 1 — fill com offset para evitar z-fighting
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            glColor3f(0.45f, 0.55f, 0.65f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            renderGroupGeometry(group);
            glDisable(GL_POLYGON_OFFSET_FILL);

            // Passe 2 — wireframe por cima
            glColor3f(0.15f, 0.85f, 0.55f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderGroupGeometry(group);
            break;
        }
    }
    // Repor para fill (para os eixos e outros elementos 2D)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
        drawAxes(2.0f);
}