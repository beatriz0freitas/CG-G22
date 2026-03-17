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
static void drawAxes() {
    glLineWidth(1.0f);

    glBegin(GL_LINES);
        glColor3f(0, 1, 0); glVertex3f(0, -100, 0); glVertex3f(0,  100, 0);
        glColor3f(1, 0, 0); glVertex3f(-100, 0, 0); glVertex3f( 100, 0, 0);
        glColor3f(0, 0, 1); glVertex3f(0, 0, -100); glVertex3f(0, 0,  100);
    glEnd();

    glDisable(GL_LINE_STIPPLE);
}

// Desenha todos os triângulos de um Group
static void renderGroupGeometry(const Group& group) {
    for (const auto& mesh : group.meshes) {
        glBegin(GL_TRIANGLES);
        for (const auto& v : mesh.verts)
            glVertex3f(v.x, v.y, v.z);
        glEnd();
    }
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