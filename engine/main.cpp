/*
 * Controlos:
 *   W / S            – zoom in / out
 *   A / D            – órbita horizontal
 *   Q / E            – órbita vertical
 *   R                – reset câmara para a posição do XML
 *   ESC              – sair
 *   Rato (esq.+drag) – órbita livre
 */

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>

#include "scene.h"
#include "xmlParser.h"
#include "renderer.h"

// Estado global
static Scene g_scene;

// Câmara orbital: o olho orbita em torno do lookAt
static float g_alpha  = 0.0f;  // ângulo horizontal
static float g_beta   = 0.3f;  // ângulo vertical
static float g_radius = 5.0f;  // distância ao lookAt

// Rato
static bool g_drag = false;
static int  g_mx = 0, g_my = 0;

// Câmara orbital
static void applyOrbit() {
    Camera& c = g_scene.camera;
    c.position.x = c.lookAt.x + g_radius * cosf(g_beta) * sinf(g_alpha);
    c.position.y = c.lookAt.y + g_radius * sinf(g_beta);
    c.position.z = c.lookAt.z + g_radius * cosf(g_beta) * cosf(g_alpha);
}

static void initOrbit() {
    Camera& c = g_scene.camera;
    float dx = c.position.x - c.lookAt.x;
    float dy = c.position.y - c.lookAt.y;
    float dz = c.position.z - c.lookAt.z;
    g_radius = sqrtf(dx*dx + dy*dy + dz*dz);
    g_beta   = asinf(dy / g_radius);
    g_alpha  = atan2f(dx, dz);
}

// Callbacks GLUT
static void display() {
    renderScene(g_scene);
    glutSwapBuffers();
}

static void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const Camera& c = g_scene.camera;
    gluPerspective(c.fov, (double)w / h, c.nearP, c.farP);
    glMatrixMode(GL_MODELVIEW);
}

static void keyboard(unsigned char key, int, int) {
    float step = g_radius * 0.05f;
    switch (key) {
        case 'w': case 'W': g_radius -= step; if (g_radius < 0.1f) g_radius = 0.1f; break;
        case 's': case 'S': g_radius += step; break;
        case 'a': case 'A': g_alpha  -= 0.05f; break;
        case 'd': case 'D': g_alpha  += 0.05f; break;
        case 'q': case 'Q': g_beta   += 0.05f; if (g_beta >  1.5f) g_beta =  1.5f; break;
        case 'e': case 'E': g_beta   -= 0.05f; if (g_beta < -1.5f) g_beta = -1.5f; break;
        case 'r': case 'R': initOrbit(); break;
        case 27:  exit(0);
    }
    applyOrbit();
    glutPostRedisplay();
}

static void mouseButton(int btn, int state, int x, int y) {
    if (btn == GLUT_LEFT_BUTTON) {
        g_drag = (state == GLUT_DOWN);
        g_mx = x; g_my = y;
    }
}

static void mouseMove(int x, int y) {
    if (!g_drag) return;
    g_alpha += (x - g_mx) * 0.01f;
    g_beta  -= (y - g_my) * 0.01f;
    if (g_beta >  1.5f) g_beta =  1.5f;
    if (g_beta < -1.5f) g_beta = -1.5f;
    g_mx = x; g_my = y;
    applyOrbit();
    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: engine <config.xml>\n");
        return 1;
    }

    printf("A carregar: %s\n", argv[1]);
    if (!parseXML(argv[1], g_scene)) 
        return 1;
    printf("Pronto: %zu modelo(s) carregado(s).\n",g_scene.root.meshes.size());

    initOrbit();
    applyOrbit();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(g_scene.winW, g_scene.winH);
    glutCreateWindow("CG Engine");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);       
    glCullFace(GL_BACK);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_scene.camera.fov,
                   (double)g_scene.winW / g_scene.winH,
                   g_scene.camera.nearP,
                   g_scene.camera.farP);
    glMatrixMode(GL_MODELVIEW);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);

    //rendererInit(g_scene);
    glutMainLoop();
    return 0;
}