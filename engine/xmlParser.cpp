#include "xmlParser.h"
#include <expat.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

struct ParseState {
    Scene*      scene;
    Group*      currentGroup;
    std::string dir;
};

static const char* getAttr(const XML_Char** atts, const char* key) {
    for (int i = 0; atts[i]; i += 2)
        if (strcmp(atts[i], key) == 0) return atts[i + 1];
    return nullptr;
}

static void XMLCALL onStart(void* ud, const XML_Char* name, const XML_Char** atts) {
    auto* ps  = static_cast<ParseState*>(ud);
    Camera& c = ps->scene->camera;

    if (strcmp(name, "window") == 0) {
        if (auto v = getAttr(atts, "width"))  ps->scene->winW = atoi(v);
        if (auto v = getAttr(atts, "height")) ps->scene->winH = atoi(v);

    } else if (strcmp(name, "position") == 0) {
        if (auto v = getAttr(atts, "x")) c.position.x = atof(v);
        if (auto v = getAttr(atts, "y")) c.position.y = atof(v);
        if (auto v = getAttr(atts, "z")) c.position.z = atof(v);

    } else if (strcmp(name, "lookAt") == 0) {
        if (auto v = getAttr(atts, "x")) c.lookAt.x = atof(v);
        if (auto v = getAttr(atts, "y")) c.lookAt.y = atof(v);
        if (auto v = getAttr(atts, "z")) c.lookAt.z = atof(v);

    } else if (strcmp(name, "up") == 0) {
        if (auto v = getAttr(atts, "x")) c.up.x = atof(v);
        if (auto v = getAttr(atts, "y")) c.up.y = atof(v);
        if (auto v = getAttr(atts, "z")) c.up.z = atof(v);

    } else if (strcmp(name, "projection") == 0) {
        if (auto v = getAttr(atts, "fov"))  c.fov   = atof(v);
        if (auto v = getAttr(atts, "near")) c.nearP = atof(v);
        if (auto v = getAttr(atts, "far"))  c.farP  = atof(v);

    } else if (strcmp(name, "model") == 0) {
        if (auto f = getAttr(atts, "file")) {
            Mesh mesh;
            mesh.filename = ps->dir + f;

            FILE* mf = fopen(mesh.filename.c_str(), "r");
            if (mf) {
                int n; fscanf(mf, "%d", &n);
                mesh.verts.resize(n);
                for (int i = 0; i < n; ++i) {
                    auto& v = mesh.verts[i];
                    // Lê os 8 campos: posição + normal + UV
                    // Formato: x y z nx ny nz u v
                    fscanf(mf, "%f %f %f %f %f %f %f %f",
                           &v.x,  &v.y,  &v.z,
                           &v.nx, &v.ny, &v.nz,
                           &v.u,  &v.v);
                }
                fclose(mf);
                printf("  Modelo: '%s' (%d triangulos)\n",
                       mesh.filename.c_str(), n / 3);
                ps->currentGroup->meshes.push_back(std::move(mesh));
            } else {
                fprintf(stderr, "Aviso: nao foi possivel abrir '%s'\n",
                        mesh.filename.c_str());
            }
        }

    // ── Fase 2 ────────────────────────────────────────────────────
    // } else if (strcmp(name, "group") == 0) {
    //     ps->currentGroup->children.emplace_back();
    //     ps->currentGroup = &ps->currentGroup->children.back();
    // } else if (strcmp(name, "translate") == 0) { ...
    // } else if (strcmp(name, "rotate") == 0) { ...
    // } else if (strcmp(name, "scale") == 0) { ...

    // ── Fase 4 ────────────────────────────────────────────────────
    // } else if (strcmp(name, "light") == 0) { ...
    // } else if (strcmp(name, "texture") == 0) { ...
    }
}

bool parseXML(const std::string& path, Scene& scene) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", path.c_str());
        return false;
    }

    ParseState ps;
    ps.scene        = &scene;
    ps.currentGroup = &scene.root;
    auto slash = path.rfind('/');
    ps.dir = (slash != std::string::npos) ? path.substr(0, slash + 1) : "";

    XML_Parser p = XML_ParserCreate(nullptr);
    XML_SetUserData(p, &ps);
    XML_SetElementHandler(p, onStart, nullptr);

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
    return ok;
}