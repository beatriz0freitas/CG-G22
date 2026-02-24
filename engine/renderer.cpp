#include "renderer.h"

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

// Desenha um Group e os seus filhos recursivamente - por agora sem transforms, sem filhos.
static void renderGroup(const Group& group) {

    glColor3f(1.0f, 1.0f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (const auto& mesh : group.meshes) {
        glBegin(GL_TRIANGLES);
        for (const auto& v : mesh.verts)
            glVertex3f(v.x, v.y, v.z);
        glEnd();
    }
}

void renderScene(const Scene& scene) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    const Camera& c = scene.camera;
    // up = (0,1,0) fixo conforme os slides.
    // Seguro porque |beta| < 1.5 rad garante que a câmara nunca fica paralela ao eixo Y.
    gluLookAt(c.position.x, c.position.y, c.position.z,
              c.lookAt.x,   c.lookAt.y,   c.lookAt.z,
              0.0f, 1.0f, 0.0f);

    renderGroup(scene.root);

    // Eixos XYZ — úteis para debug de orientação
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_LINES);
        glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(2,0,0); // X vermelho
        glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,2,0); // Y verde
        glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,2); // Z azul
    glEnd();
}