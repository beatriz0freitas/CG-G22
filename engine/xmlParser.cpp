#include "xmlParser.h"
#include <expat.h>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <functional>

struct ParseState {
    Scene*               scene;
    std::vector<Group*>  groupStack;
    std::string          dir;
    // Para acumular <point> dentro de um <translate time=...>
    Group*  animGroup = nullptr;
    size_t  animIdx   = 0;
    Mesh*   currentMesh = nullptr;
    Light*  currentLight = nullptr;
};

static bool sameName(const char* a, const char* b) {
    while (*a && *b) {
        char ca = (char)std::tolower((unsigned char)*a);
        char cb = (char)std::tolower((unsigned char)*b);
        if (ca != cb) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

static const char* attr(const XML_Char** atts, const char* key) {
    for (int i = 0; atts[i]; i += 2)
        if (sameName(atts[i], key)) return atts[i + 1];
    return nullptr;
}

static bool attrBool(const XML_Char** atts, const char* key) {
    const char* value = attr(atts, key);
    if (!value) return false;

    std::string s(value);
    for (char& c : s)
        c = (char)std::tolower((unsigned char)c);

    return s == "true" || s == "1" || s == "yes";
}

static std::string lowerStr(const char* value) {
    std::string s = value ? value : "";
    for (char& c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

static float colorComp(const XML_Char** atts, const char* key, float fallback) {
    if (auto v = attr(atts, key)) return (float)atof(v) / 255.0f;
    return fallback;
}

static void parseRGB(const XML_Char** atts, float dst[4]) {
    dst[0] = colorComp(atts, "R", dst[0]);
    dst[1] = colorComp(atts, "G", dst[1]);
    dst[2] = colorComp(atts, "B", dst[2]);
    dst[3] = 1.0f;
}

static void XMLCALL onStart(void* ud, const XML_Char* name, const XML_Char** atts) {
    auto* ps  = static_cast<ParseState*>(ud);
    Camera& c = ps->scene->camera;

    // ── Câmara ──
    if (strcmp(name, "window") == 0) {
        if (auto v = attr(atts, "width"))  ps->scene->winW = atoi(v);
        if (auto v = attr(atts, "height")) ps->scene->winH = atoi(v);

    } else if (strcmp(name, "position") == 0) {
        if (auto v = attr(atts, "x")) c.position.x = atof(v);
        if (auto v = attr(atts, "y")) c.position.y = atof(v);
        if (auto v = attr(atts, "z")) c.position.z = atof(v);

    } else if (strcmp(name, "lookAt") == 0) {
        if (auto v = attr(atts, "x")) c.lookAt.x = atof(v);
        if (auto v = attr(atts, "y")) c.lookAt.y = atof(v);
        if (auto v = attr(atts, "z")) c.lookAt.z = atof(v);

    } else if (strcmp(name, "up") == 0) {
        if (auto v = attr(atts, "x")) c.up.x = atof(v);
        if (auto v = attr(atts, "y")) c.up.y = atof(v);
        if (auto v = attr(atts, "z")) c.up.z = atof(v);

    } else if (strcmp(name, "projection") == 0) {
        if (auto v = attr(atts, "fov"))  c.fov   = atof(v);
        if (auto v = attr(atts, "near")) c.nearP = atof(v);
        if (auto v = attr(atts, "far"))  c.farP  = atof(v);

    // ── Luzes ──
    } else if (strcmp(name, "light") == 0) {
        ps->currentLight = nullptr;
        if (ps->scene->lights.size() >= 8) return;

        Light light;
        std::string type = lowerStr(attr(atts, "type"));
        if (type == "directional") {
            light.type = LightType::DIRECTIONAL;
        } else if (type == "spot" || type == "spotlight") {
            light.type = LightType::SPOT;
            light.cutoff = 45.0f;
        } else {
            light.type = LightType::POINT;
        }

        if (auto v = attr(atts, "posx")) light.position.x = atof(v);
        if (auto v = attr(atts, "posy")) light.position.y = atof(v);
        if (auto v = attr(atts, "posz")) light.position.z = atof(v);
        if (auto v = attr(atts, "dirx")) light.direction.x = atof(v);
        if (auto v = attr(atts, "diry")) light.direction.y = atof(v);
        if (auto v = attr(atts, "dirz")) light.direction.z = atof(v);
        if (auto v = attr(atts, "cutoff")) light.cutoff = atof(v);

        ps->scene->lights.push_back(light);
        ps->currentLight = &ps->scene->lights.back();

    // ── Grupos ──
    } else if (strcmp(name, "group") == 0) {
        Group* parent = ps->groupStack.empty()
                        ? &ps->scene->root
                        : ps->groupStack.back();
        parent->children.emplace_back();
        ps->groupStack.push_back(&parent->children.back());

    // ── Transforms ──
    } else if (strcmp(name, "translate") == 0) {
        if (ps->groupStack.empty()) return;
        TransformOp op;

        if (auto v = attr(atts, "time")) {
            // Animado: Catmull-Rom
            op.type  = TransformType::ANIM_TRANSLATE;
            op.time  = atof(v);
            op.align = attrBool(atts, "align");
            ps->groupStack.back()->transforms.push_back(op);
            // Guarda referência para acumular <point>
            ps->animGroup = ps->groupStack.back();
            ps->animIdx   = ps->animGroup->transforms.size() - 1;
        } else {
            // Estático
            op.type = TransformType::TRANSLATE;
            if (auto v = attr(atts, "x")) op.a = atof(v);
            if (auto v = attr(atts, "y")) op.b = atof(v);
            if (auto v = attr(atts, "z")) op.c = atof(v);
            ps->groupStack.back()->transforms.push_back(op);
        }

    } else if (strcmp(name, "rotate") == 0) {
        if (ps->groupStack.empty()) return;
        TransformOp op;

        if (auto v = attr(atts, "time")) {
            // Rotação contínua
            op.type = TransformType::ANIM_ROTATE;
            op.time = atof(v);
            if (auto v2 = attr(atts, "x")) op.b = atof(v2);
            if (auto v2 = attr(atts, "y")) op.c = atof(v2);
            if (auto v2 = attr(atts, "z")) op.d = atof(v2);
        } else {
            // Rotação estática
            op.type = TransformType::ROTATE;
            if (auto v2 = attr(atts, "angle")) op.a = atof(v2);
            if (auto v2 = attr(atts, "x"))     op.b = atof(v2);
            if (auto v2 = attr(atts, "y"))     op.c = atof(v2);
            if (auto v2 = attr(atts, "z"))     op.d = atof(v2);
        }
        ps->groupStack.back()->transforms.push_back(op);

    } else if (strcmp(name, "scale") == 0) {
        if (ps->groupStack.empty()) return;
        TransformOp op;
        op.type = TransformType::SCALE;
        op.a = 1.0f; op.b = 1.0f; op.c = 1.0f;
        if (auto v = attr(atts, "x")) op.a = atof(v);
        if (auto v = attr(atts, "y")) op.b = atof(v);
        if (auto v = attr(atts, "z")) op.c = atof(v);
        ps->groupStack.back()->transforms.push_back(op);

    // ── Ponto de controlo Catmull-Rom ──
    } else if (strcmp(name, "point") == 0) {
        if (!ps->animGroup) return;
        Vec3 p{0,0,0};
        if (auto v = attr(atts, "x")) p.x = atof(v);
        if (auto v = attr(atts, "y")) p.y = atof(v);
        if (auto v = attr(atts, "z")) p.z = atof(v);
        ps->animGroup->transforms[ps->animIdx].points.push_back(p);

    // ── Modelo ──
    } else if (strcmp(name, "model") == 0) {
        ps->currentMesh = nullptr;
        if (auto f = attr(atts, "file")) {
            Group* g = ps->groupStack.empty()
                       ? &ps->scene->root
                       : ps->groupStack.back();

            Mesh mesh;
            mesh.filename = ps->dir + f;
            FILE* mf = fopen(mesh.filename.c_str(), "r");
            if (mf) {
                int n = 0;
                bool ok = fscanf(mf, "%d", &n) == 1 && n > 0;
                if (ok) {
                    mesh.verts.resize(n);
                    for (int i = 0; i < n; ++i) {
                        auto& v = mesh.verts[i];
                        if (fscanf(mf, "%f %f %f %f %f %f %f %f",
                                   &v.x, &v.y, &v.z,
                                   &v.nx, &v.ny, &v.nz,
                                   &v.u,  &v.v) != 8) {
                            ok = false;
                            break;
                        }
                    }
                }
                fclose(mf);
                if (ok) {
                    printf("  Modelo: '%s' (%d triangulos)\n",
                           mesh.filename.c_str(), n / 3);
                    g->meshes.push_back(std::move(mesh));
                    ps->currentMesh = &g->meshes.back();
                } else {
                    fprintf(stderr, "Aviso: formato invalido em '%s'\n",
                            mesh.filename.c_str());
                }
            } else {
                fprintf(stderr, "Aviso: nao foi possivel abrir '%s'\n",
                        mesh.filename.c_str());
            }
        }

    } else if (strcmp(name, "texture") == 0) {
        if (ps->currentMesh) {
            if (auto f = attr(atts, "file"))
                ps->currentMesh->textureFile = ps->dir + f;
        }

    } else if (strcmp(name, "color") == 0) {
        if (ps->currentLight)
            parseRGB(atts, ps->currentLight->color);

    } else if (strcmp(name, "diffuse") == 0) {
        if (ps->currentMesh) parseRGB(atts, ps->currentMesh->material.diffuse);

    } else if (strcmp(name, "ambient") == 0) {
        if (ps->currentMesh) parseRGB(atts, ps->currentMesh->material.ambient);

    } else if (strcmp(name, "specular") == 0) {
        if (ps->currentMesh) parseRGB(atts, ps->currentMesh->material.specular);

    } else if (strcmp(name, "emissive") == 0) {
        if (ps->currentMesh) parseRGB(atts, ps->currentMesh->material.emissive);

    } else if (strcmp(name, "shininess") == 0) {
        if (ps->currentMesh) {
            if (auto v = attr(atts, "value"))
                ps->currentMesh->material.shininess = atof(v);
        }
    }
}

static void XMLCALL onEnd(void* ud, const XML_Char* name) {
    auto* ps = static_cast<ParseState*>(ud);

    if (strcmp(name, "group") == 0 && !ps->groupStack.empty())
        ps->groupStack.pop_back();

    if (strcmp(name, "translate") == 0)
        ps->animGroup = nullptr;   // fecha o bloco de pontos

    if (strcmp(name, "model") == 0)
        ps->currentMesh = nullptr;

    if (strcmp(name, "light") == 0)
        ps->currentLight = nullptr;
}

bool parseXML(const std::string& path, Scene& scene) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", path.c_str());
        return false;
    }

    ParseState ps;
    ps.scene = &scene;
    auto slash = path.rfind('/');
    ps.dir = (slash != std::string::npos) ? path.substr(0, slash + 1) : "";

    XML_Parser p = XML_ParserCreate(nullptr);
    XML_SetUserData(p, &ps);
    XML_SetElementHandler(p, onStart, onEnd);

    char buf[4096];
    bool ok = true; int done = 0;
    while (!done) {
        size_t len = fread(buf, 1, sizeof(buf), f);
        done = feof(f);
        if (XML_Parse(p, buf, (int)len, done) == XML_STATUS_ERROR) {
            fprintf(stderr, "Erro XML: %s (linha %lu)\n",
                    XML_ErrorString(XML_GetErrorCode(p)),
                    XML_GetCurrentLineNumber(p));
            ok = false; break;
        }
    }
    XML_ParserFree(p);
    fclose(f);
    
    // ── Debug: Contar modelos únicos vs instâncias ──
    if (ok) {
        std::map<std::string, int> modelCount;
        std::function<void(const Group&)> countModels = [&](const Group& g) {
            for (const auto& mesh : g.meshes) {
                modelCount[mesh.filename]++;
            }
            for (const auto& child : g.children)
                countModels(child);
        };
        countModels(scene.root);
        
        int totalModels = 0, uniqueModels = modelCount.size();
        for (const auto& [filename, count] : modelCount) {
            totalModels += count;
        }
        printf("\n=== Cache Analysis ===\n");
        printf("Modelos únicos: %d\n", uniqueModels);
        printf("Instâncias totais: %d\n", totalModels);
        for (const auto& [filename, count] : modelCount) {
            if (count > 1)
                printf("  %s: %d instâncias (cache potential: %d reutilizações)\n",
                       filename.c_str(), count, count - 1);
        }
        printf("\n");
    }
    
    return ok;
}
