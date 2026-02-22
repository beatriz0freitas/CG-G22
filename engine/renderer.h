#pragma once
#include "scene.h"

// ─────────────────────────────────────────────────────────────────────
// renderer.h
//
// Responsabilidade única: desenhar a cena com OpenGL.
// Separado do main.cpp para que a transição para VBOs na Fase 3
// seja feita aqui sem tocar nos callbacks GLUT.
//
// Fase 1: glBegin/glEnd (modo imediato)
// Fase 3: substituir por VBOs — só este ficheiro muda
// Fase 4: ativar lighting, texturas, normais — só este ficheiro muda
// ─────────────────────────────────────────────────────────────────────

// Inicialização one-time (chamada uma vez após glutCreateWindow)
void rendererInit(const Scene& scene);

// Desenha um frame completo da cena
void renderScene(const Scene& scene);