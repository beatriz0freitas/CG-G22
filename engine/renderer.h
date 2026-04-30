#pragma once
#include "scene.h"

// Responsabilidade única: desenhar a scene com OpenGL.

// Modos de renderização togglávéis com a tecla 'M'
enum class RenderMode {
    WIREFRAME,      // GL_LINE — modo atual 
    SOLID,          // GL_FILL sem eixos
    SOLID_WIRE,     // GL_FILL + contornos (dois passes)
};

// Estado partilhado para o main.cpp poder ler
extern RenderMode g_renderMode;
extern bool       g_showAxes;
extern float      g_time;   // segundos desde o início, actualizado pelo main

// Cicla para o próximo modo de renderização
void toggleRenderMode();

// Faz upload de todos os VBOs da scene para a GPU (chamar depois de glutCreateWindow)
void buildVBOs(Group& g);

// Desenha um frame completo da cena
void renderScene(const Scene& scene);