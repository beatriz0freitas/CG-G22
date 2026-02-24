/*
 * Controlos:
 *   W / S            – avança / recua ao longo de D (FPS, lookAt acompanha)
 *   A / D            – órbita horizontal (Explorer Mode)
 *   Q / E            – órbita vertical   (Explorer Mode)
 *   R                – reset câmara para a posição definida no XML
 *   M                – cicla modo de renderização (wireframe → solid → solid+wire)
 *   X                – toggle eixos XYZ
 *   Scroll           – zoom in / out (altera radius)
 *   Rato (esq.+drag) – órbita livre
 *   ESC              – sair
 *
 * Modelo de câmara:
 *   Explorer Mode — alpha/beta/radius → coordenadas cartesianas via esféricas
 *   FPS forward   — P' = P + k×D,  lookAt' = lookAt + k×D  (slide 6)
 */

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstring>

#include "scene.h"
#include "xmlParser.h"
#include "renderer.h"

// ── Estado global ──
static Scene g_scene;
static Camera g_initCamera;  // posição original do XML para o reset com R

// Câmara orbital
static float g_alpha  = 0.0f;
static float g_beta   = 0.3f;
static float g_radius = 5.0f;

// Rato
static bool g_drag = false;
static int  g_mx = 0, g_my = 0;

// FPS
static int   g_frames   = 0;
static float g_fps      = 0.0f;
static int   g_lastTime = 0;

// ── Câmara orbital ──
static void applyOrbit() {
    Camera& c = g_scene.camera;
    c.position.x = c.lookAt.x + g_radius * cosf(g_beta) * sinf(g_alpha);
    c.position.y = c.lookAt.y + g_radius * sinf(g_beta);
    c.position.z = c.lookAt.z + g_radius * cosf(g_beta) * cosf(g_alpha);
}

// Calcula alpha/beta/radius a partir de uma posição de câmara
static void orbitFromCamera(const Camera& c) {
    float dx = c.position.x - c.lookAt.x;
    float dy = c.position.y - c.lookAt.y;
    float dz = c.position.z - c.lookAt.z;
    g_radius = sqrtf(dx*dx + dy*dy + dz*dz);
    if (g_radius < 0.001f) g_radius = 1.0f;
    g_beta   = asinf(dy / g_radius);
    g_alpha  = atan2f(dx, dz);
}

static void initOrbit() {
    orbitFromCamera(g_initCamera);
    g_scene.camera.lookAt = g_initCamera.lookAt;  // restaura lookAt também
}

// Atualiza o título da janela com FPS e modo de renderização
static void updateTitle() {
    const char* modeStr = "";
    switch (g_renderMode) {
        case RenderMode::WIREFRAME:  modeStr = "Wireframe";       break;
        case RenderMode::SOLID:      modeStr = "Solid";           break;
        case RenderMode::SOLID_WIRE: modeStr = "Solid+Wireframe"; break;
    }
    char title[128];
    snprintf(title, sizeof(title),
             "CG Engine  |  %.1f FPS  |  %s  |  Eixos: %s  |  r=%.2f",
             g_fps, modeStr, g_showAxes ? "on" : "off", g_radius);
    glutSetWindowTitle(title);
}

// ── Callbacks GLUT ──
static void display() {
    renderScene(g_scene);
    glutSwapBuffers();

    // Contagem de FPS
    ++g_frames;
    int now = glutGet(GLUT_ELAPSED_TIME);
    int dt  = now - g_lastTime;
    if (dt >= 500) {                            // atualiza a cada 500 ms
        g_fps      = g_frames * 1000.0f / dt;
        g_frames   = 0;
        g_lastTime = now;
        updateTitle();
    }
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
    Camera& c = g_scene.camera;

    switch (key) {
        // ── FPS forward/backward: P' = P + k*D,  lookAt' = lookAt + k*D  ──
        // D é unitário por construção: ||D||² = cos²β·sin²α + sin²β + cos²β·cos²α = 1
        case 'w': case 'W': {
            float k  =  g_radius * 0.05f;
            float dx = cosf(g_beta) * sinf(g_alpha);
            float dy = sinf(g_beta);
            float dz = cosf(g_beta) * cosf(g_alpha);
            c.lookAt.x += k * dx;  c.lookAt.y += k * dy;  c.lookAt.z += k * dz;
            break;
        }
        case 's': case 'S': {
            float k  =  g_radius * 0.05f;
            float dx = cosf(g_beta) * sinf(g_alpha);
            float dy = sinf(g_beta);
            float dz = cosf(g_beta) * cosf(g_alpha);
            c.lookAt.x -= k * dx;  c.lookAt.y -= k * dy;  c.lookAt.z -= k * dz;
            break;
        }
        // ── Explorer Mode orbit ──
        case 'a': case 'A': g_alpha -= 0.05f; break;
        case 'd': case 'D': g_alpha += 0.05f; break;
        case 'q': case 'Q': g_beta  += 0.05f; if (g_beta >  1.5f) g_beta =  1.5f; break;
        case 'e': case 'E': g_beta  -= 0.05f; if (g_beta < -1.5f) g_beta = -1.5f; break;

        case 'r': case 'R': initOrbit(); break;
        case 'm': case 'M': toggleRenderMode(); break;
        case 'x': case 'X': g_showAxes = !g_showAxes; break;
        case 27:  exit(0);
    }
    applyOrbit();
    updateTitle();
    glutPostRedisplay();
}

static void mouseButton(int btn, int state, int x, int y) {
    if (btn == GLUT_LEFT_BUTTON) {
        g_drag = (state == GLUT_DOWN);
        g_mx = x; g_my = y;
    }
    // Scroll do rato: zoom
    if (btn == 3) { g_radius *= 0.95f; if (g_radius < 0.1f) g_radius = 0.1f; applyOrbit(); glutPostRedisplay(); }
    if (btn == 4) { g_radius *= 1.05f; applyOrbit(); glutPostRedisplay(); }
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
    g_initCamera = g_scene.camera;  // guarda estado original para reset com R

    // Conta o total de triângulos carregados
    size_t totalTris = 0;
    for (const auto& mesh : g_scene.root.meshes)
        totalTris += mesh.verts.size() / 3;
    printf("Pronto: %zu modelo(s), %zu triangulo(s) total.\n",
           g_scene.root.meshes.size(), totalTris);

    initOrbit();
    applyOrbit();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(g_scene.winW, g_scene.winH);
    glutCreateWindow("CG Engine");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);   // fundo azul-escuro (melhor contraste)

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

    g_lastTime = glutGet(GLUT_ELAPSED_TIME);
    updateTitle();

    glutMainLoop();
    return 0;
}