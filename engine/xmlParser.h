#pragma once
#include "scene.h"
#include <string>

// xmlParser.h -  ler um ficheiro XML e preencher uma Scene.
// window, camera, group/models/model
bool parseXML(const std::string& path, Scene& scene);