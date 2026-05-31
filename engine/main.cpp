/*
 * Controlos (Explorer Mode):
 *   W / S            – órbita vertical
 *   A / D            – órbita horizontal
 *   Seta Cima/Baixo  – avança / recua (move position no plano XZ)
 *   Seta Esq./Dir.   – strafe lateral  (move position no plano XZ)
 *   Z / X            – sobe / desce    (move position em Y)
 *   Q / E            – zoom in / out (altera radius)
 *   Scroll           – zoom in / out (altera radius)
 *   Rato (esq.+drag) – órbita livre
 *
 * Controlos (Third Person Mode):
 *   W / A / S / D    – move character (target) no plano XZ
 *   Seta Cima/Baixo  – sobe / desce character em Y
 *   Seta Esq./Dir.   – roda câmara ao redor do character
 *   Q / E            – zoom in / out (altera distance)
 *   Scroll           – zoom in / out (altera distance)
 *   Rato (esq.+drag) – roda câmara ao redor do character
 *
 * Global:
 *   V                – muda entre Explorer Mode e Third Person Mode
 *   R                – reset câmara para a posição definida no XML
 *   M                – cicla modo de renderização (wireframe → solid → solid+wire)
 *   B                – toggle eixos XYZ
 *   C                – mostrar / esconder a linha Catmull-Rom
 *   ESC              – sair
 *
 * Modelo de câmara:
 *   Explorer Mode — alpha/beta/radius → coordenadas cartesianas via esféricas
 *   Third Person  — camera segue target mantendo distance e angle
 */

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstring>

#include "scene.h"
#include "xmlParser.h"
#include "renderer.h"

// ── Estado global ──
static Scene g_scene;
static Camera g_initCamera; // posição original do XML para o reset com R

// Câmara orbital (Explorer Mode)
static float g_alpha = 0.0f;
static float g_beta = 0.3f;
static float g_radius = 5.0f;

// Third Person Mode
enum class CameraMode { EXPLORER, THIRD_PERSON };
static CameraMode g_cameraMode = CameraMode::EXPLORER;
static Vec3 g_targetPos = {0.0f, 0.0f, 0.0f}; // posição do character
static float g_thirdPersonAlpha = 0.0f;  // ângulo horizontal ao redor do character
static float g_thirdPersonDistance = 3.0f; // distância da câmara ao character
static float g_thirdPersonHeight = 1.0f;   // altura relativa ao character

// Rato
static bool g_drag = false;
static int g_mx = 0, g_my = 0;

// FPS
static int g_frames = 0;
static float g_fps = 0.0f;
static int g_lastTime = 0;

// ── Câmara orbital ──
static void applyOrbit() {
    Camera &c = g_scene.camera;
    c.position.x = c.lookAt.x + g_radius * cosf(g_beta) * sinf(g_alpha);
    c.position.y = c.lookAt.y + g_radius * sinf(g_beta);
    c.position.z = c.lookAt.z + g_radius * cosf(g_beta) * cosf(g_alpha);
}

// ── Third Person Camera ──
static void applyThirdPerson() {
    Camera &c = g_scene.camera;
    // Câmara posicionada atrás e acima do character (target)
    c.lookAt = g_targetPos;
    c.lookAt.y += g_thirdPersonHeight;
    
    c.position.x = g_targetPos.x + g_thirdPersonDistance * sinf(g_thirdPersonAlpha);
    c.position.y = g_targetPos.y + g_thirdPersonHeight + g_thirdPersonDistance * 0.3f; // um pouco acima
    c.position.z = g_targetPos.z + g_thirdPersonDistance * cosf(g_thirdPersonAlpha);
}

// Calcula alpha/beta/radius a partir de uma posição de câmara
static void orbitFromCamera(const Camera &c) {
    float dx = c.position.x - c.lookAt.x;
    float dy = c.position.y - c.lookAt.y;
    float dz = c.position.z - c.lookAt.z;
    g_radius = sqrtf(dx * dx + dy * dy + dz * dz);
    if (g_radius < 0.001f)
        g_radius = 1.0f;
    g_beta = asinf(dy / g_radius);
    g_alpha = atan2f(dx, dz);
}

static void initOrbit() {
    orbitFromCamera(g_initCamera);
    g_scene.camera.lookAt = g_initCamera.lookAt; // restaura lookAt também
}

static void countGroup(const Group& group, size_t& models, size_t& tris) {
    models += group.meshes.size();
    for (const auto& mesh : group.meshes)
        tris += mesh.verts.size() / 3;
    for (const auto& child : group.children)
        countGroup(child, models, tris);
}

// Atualiza o título da janela com FPS e modo de renderização
static void updateTitle() {
    const char *modeStr = "";
    switch (g_renderMode) {
        case RenderMode::WIREFRAME:
            modeStr = "Wireframe";
            break;
        case RenderMode::SOLID:
            modeStr = "Solid";
            break;
        case RenderMode::SOLID_WIRE:
            modeStr = "Solid+Wireframe";
            break;
    }
    const char *camStr = (g_cameraMode == CameraMode::EXPLORER) ? "Explorer" : "Third Person";
    char title[128];
    snprintf(title, sizeof(title),
             "CG Engine  |  %.1f FPS  |  %s  |  Câmara: %s  |  Eixos: %s  |  Catmull: %s",
             g_fps, modeStr, camStr, g_showAxes ? "on" : "off",
             g_showCatmullCurve ? "on" : "off");
    glutSetWindowTitle(title);
}

// ── Callbacks GLUT ──
static void display() {
    // Actualiza o tempo global (segundos desde o início)
    g_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
 
    renderScene(g_scene);
    glutSwapBuffers();
    glutPostRedisplay();   // animação contínua
 
    ++g_frames;
    int now = glutGet(GLUT_ELAPSED_TIME);
    int dt  = now - g_lastTime;
    if (dt >= 500) {
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


// Move a posição da câmara (e o lookAt o mesmo delta, para preservar a direção)
static void translateCamera(Camera &c, float ox, float oy, float oz) {
    c.position.x += ox;
    c.position.y += oy;
    c.position.z += oz;
    c.lookAt.x += ox;
    c.lookAt.y += oy;
    c.lookAt.z += oz;
}

static void keyboard(unsigned char key, int, int) {
    Camera &c = g_scene.camera;

    switch (key) {
    // ── Mode switch ──
        case 'v':
        case 'V':
            g_cameraMode = (g_cameraMode == CameraMode::EXPLORER) 
                         ? CameraMode::THIRD_PERSON 
                         : CameraMode::EXPLORER;
            if (g_cameraMode == CameraMode::EXPLORER) {
                applyOrbit();
            } else {
                applyThirdPerson();
            }
            updateTitle();
            break;
    
    // ── Explorer Mode orbit vertical ──
        case 'w':
        case 'W':
            if (g_cameraMode == CameraMode::EXPLORER) {
                g_beta += 0.05f;
                if (g_beta > 1.5f)
                    g_beta = 1.5f;
                applyOrbit();
            } else {
                // Third Person: move character forward
                g_targetPos.x += 0.1f * sinf(g_thirdPersonAlpha);
                g_targetPos.z += 0.1f * cosf(g_thirdPersonAlpha);
                applyThirdPerson();
            }
            break;
        case 's':
        case 'S':
            if (g_cameraMode == CameraMode::EXPLORER) {
                g_beta -= 0.05f;
                if (g_beta < -1.5f)
                    g_beta = -1.5f;
                applyOrbit();
            } else {
                // Third Person: move character backward
                g_targetPos.x -= 0.1f * sinf(g_thirdPersonAlpha);
                g_targetPos.z -= 0.1f * cosf(g_thirdPersonAlpha);
                applyThirdPerson();
            }
            break;
        // ── Explorer Mode orbit horizontal ──
        case 'a':
        case 'A':
            if (g_cameraMode == CameraMode::EXPLORER) {
                g_alpha -= 0.05f;
                applyOrbit();
            } else {
                // Third Person: move character left
                float leftX = cosf(g_thirdPersonAlpha);
                float leftZ = -sinf(g_thirdPersonAlpha);
                g_targetPos.x -= 0.1f * leftX;
                g_targetPos.z -= 0.1f * leftZ;
                applyThirdPerson();
            }
            break;
        case 'd':
        case 'D':
            if (g_cameraMode == CameraMode::EXPLORER) {
                g_alpha += 0.05f;
                applyOrbit();
            } else {
                // Third Person: move character right
                float rightX = cosf(g_thirdPersonAlpha);
                float rightZ = -sinf(g_thirdPersonAlpha);
                g_targetPos.x += 0.1f * rightX;
                g_targetPos.z += 0.1f * rightZ;
                applyThirdPerson();
            }
            break;
        // ── Zoom ──
        case 'q':
        case 'Q':
            if (g_cameraMode == CameraMode::EXPLORER) {
                g_radius *= 0.95f;
                if (g_radius < 0.1f)
                    g_radius = 0.1f;
                applyOrbit();
            } else {
                g_thirdPersonDistance *= 0.95f;
                if (g_thirdPersonDistance < 0.5f)
                    g_thirdPersonDistance = 0.5f;
                applyThirdPerson();
            }
            break;
        case 'e':
        case 'E':
            if (g_cameraMode == CameraMode::EXPLORER) {
                g_radius *= 1.05f;
                applyOrbit();
            } else {
                g_thirdPersonDistance *= 1.05f;
                applyThirdPerson();
            }
            break;
        // ── Sobe / desce ──
        case 'z':
        case 'Z': {
            if (g_cameraMode == CameraMode::EXPLORER) {
                float k = g_radius * 0.05f;
                translateCamera(c, 0.0f, k, 0.0f);
                orbitFromCamera(c);
                applyOrbit();
            } else {
                g_targetPos.y += 0.1f;
                applyThirdPerson();
            }
            break;
        }
        case 'x':
        case 'X': {
            if (g_cameraMode == CameraMode::EXPLORER) {
                float k = g_radius * 0.05f;
                translateCamera(c, 0.0f, -k, 0.0f);
                orbitFromCamera(c);
                applyOrbit();
            } else {
                g_targetPos.y -= 0.1f;
                applyThirdPerson();
            }
            break;
        }
        case 'r':
        case 'R':
            g_cameraMode = CameraMode::EXPLORER;
            initOrbit();
            g_targetPos = {0.0f, 0.0f, 0.0f};
            applyOrbit();
            break;
        case 'm':
        case 'M':
            toggleRenderMode();
            break;
        case 'b':
        case 'B':
            g_showAxes = !g_showAxes;
            break;
        case 'c':
        case 'C':
            toggleCatmullCurve();
            break;
        case 27:
            exit(0);
    }
    updateTitle();
    glutPostRedisplay();
}

static void specialKey(int key, int, int) {
    Camera &c = g_scene.camera;
    float k = g_radius * 0.05f;
    
    if (g_cameraMode == CameraMode::EXPLORER) {
        // Vetor frente D (normalizado, no plano XZ — ignora componente vertical para pan horizontal)
        float fdx = -sinf(g_alpha);
        float fdz = -cosf(g_alpha);
        // Vetor direita (perpendicular a D no plano XZ)
        float rx = cosf(g_alpha);
        float rz = -sinf(g_alpha);
        switch (key) {
            case GLUT_KEY_UP:
                translateCamera(c, k * fdx, 0.0f, k * fdz);
                break;
            case GLUT_KEY_DOWN:
                translateCamera(c, -k * fdx, 0.0f, -k * fdz);
                break;
            case GLUT_KEY_RIGHT:
                translateCamera(c, k * rx, 0.0f, k * rz);
                break;
            case GLUT_KEY_LEFT:
                translateCamera(c, -k * rx, 0.0f, -k * rz);
                break;
        }
        // Recalcula alpha/beta/radius a partir da nova posição
        orbitFromCamera(c);
        applyOrbit();
    } else {
        // Third Person Mode: setas rotacionam câmara e sobem/descem character
        switch (key) {
            case GLUT_KEY_UP:
                g_targetPos.y += 0.1f;
                break;
            case GLUT_KEY_DOWN:
                g_targetPos.y -= 0.1f;
                break;
            case GLUT_KEY_RIGHT:
                g_thirdPersonAlpha += 0.05f;
                break;
            case GLUT_KEY_LEFT:
                g_thirdPersonAlpha -= 0.05f;
                break;
        }
        applyThirdPerson();
    }
    updateTitle();
    glutPostRedisplay();
}

static void mouseButton(int btn, int state, int x, int y) {
    if (btn == GLUT_LEFT_BUTTON) {
        g_drag = (state == GLUT_DOWN);
        g_mx = x;
        g_my = y;
    }
    // Scroll do rato: zoom
    if (btn == 3) {
        if (g_cameraMode == CameraMode::EXPLORER) {
            g_radius *= 0.95f;
            if (g_radius < 0.1f)
                g_radius = 0.1f;
            applyOrbit();
        } else {
            g_thirdPersonDistance *= 0.95f;
            if (g_thirdPersonDistance < 0.5f)
                g_thirdPersonDistance = 0.5f;
            applyThirdPerson();
        }
        glutPostRedisplay();
    }
    if (btn == 4) {
        if (g_cameraMode == CameraMode::EXPLORER) {
            g_radius *= 1.05f;
            applyOrbit();
        } else {
            g_thirdPersonDistance *= 1.05f;
            applyThirdPerson();
        }
        glutPostRedisplay();
    }
}

static void mouseMove(int x, int y) {
    if (!g_drag) return;
    
    if (g_cameraMode == CameraMode::EXPLORER) {
        g_alpha += (x - g_mx) * 0.01f;
        g_beta  -= (y - g_my) * 0.01f;
        if (g_beta >  1.5f) 
            g_beta =  1.5f;
        if (g_beta < -1.5f) 
            g_beta = -1.5f;
        applyOrbit();
    } else {
        g_thirdPersonAlpha += (x - g_mx) * 0.01f;
        g_targetPos.y += (g_my - y) * 0.02f;
        applyThirdPerson();
    }
    g_mx = x; 
    g_my = y;
    glutPostRedisplay();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: engine <config.xml>\n");
        return 1;
    }

    printf("A carregar: %s\n", argv[1]);
    if (!parseXML(argv[1], g_scene))
        return 1;
    g_initCamera = g_scene.camera; // guarda estado original para reset com R

    // Conta o total de triângulos carregados
    size_t totalModels = 0;
    size_t totalTris = 0;
    countGroup(g_scene.root, totalModels, totalTris);
    printf("Pronto: %zu modelo(s), %zu triangulo(s) total.\n",
           totalModels, totalTris);

    initOrbit();
    applyOrbit();
 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(g_scene.winW, g_scene.winH);
    glutCreateWindow("CG Engine");
 
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_NORMALIZE);
#ifdef GL_RESCALE_NORMAL
    glEnable(GL_RESCALE_NORMAL);
#endif
    glShadeModel(GL_SMOOTH);
    {
        float amb[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Fase 3: upload de todos os modelos para a GPU via VBOs
    buildVBOs(g_scene.root);
 
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
    glutSpecialFunc(specialKey);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMove);
 
    g_lastTime = glutGet(GLUT_ELAPSED_TIME);
    updateTitle();
    glutMainLoop();
    return 0;
}
