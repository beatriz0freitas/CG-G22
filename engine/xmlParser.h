#pragma once
#include "scene.h"
#include <string>

// ─────────────────────────────────────────────────────────────────────
// xmlParser.h
//
// Responsabilidade única: ler um ficheiro XML e preencher uma Scene.
// Este ficheiro nunca é reescrito — só cresce à medida que o formato
// XML evolui nas fases seguintes.
//
// Fase 1: window, camera, group/models/model
// Fase 2: group/transform (translate, rotate, scale), grupos aninhados
// Fase 3: translate com time/align/points, rotate com time
// Fase 4: lights, model/texture, model/color
// ─────────────────────────────────────────────────────────────────────

bool parseXML(const std::string& path, Scene& scene);