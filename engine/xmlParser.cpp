#include "xmlParser.h"
#include <expat.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct ParseState {
    Scene*               scene;
    std::vector<Group*>  groupStack; 
    std::string          dir;
};

static const char* attr(const XML_Char** atts, const char* key) {
    for (int i = 0; atts[i]; i += 2)
        if (strcmp(atts[i], key) == 0) return atts[i + 1];
    return nullptr;
}

static void XMLCALL onStart(void* ud, const XML_Char* name, const XML_Char** atts) {
    auto* ps  = static_cast<ParseState*>(ud);
    Camera& c = ps->scene->camera;

    // Câmara
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

    // grupo
    } else if (strcmp(name, "group") == 0) {
        // Cria um novo grupo filho do grupo atual e coloca-o na pilha
        Group* parent = ps->groupStack.empty()
                        ? &ps->scene->root
                        : ps->groupStack.back();
        parent->children.emplace_back();          // adiciona filho vazio
        ps->groupStack.push_back(&parent->children.back());

    // transforms 
    } else if (strcmp(name, "translate") == 0) {
        if (ps->groupStack.empty()) return;
        TransformOp op;
        op.type = TransformType::TRANSLATE;
        if (auto v = attr(atts, "x")) op.a = atof(v);
        if (auto v = attr(atts, "y")) op.b = atof(v);
        if (auto v = attr(atts, "z")) op.c = atof(v);
        ps->groupStack.back()->transforms.push_back(op);

    } else if (strcmp(name, "rotate") == 0) {
        if (ps->groupStack.empty()) return;
        TransformOp op;
        op.type = TransformType::ROTATE;
        if (auto v = attr(atts, "angle")) op.a = atof(v);
        if (auto v = attr(atts, "x"))     op.b = atof(v);
        if (auto v = attr(atts, "y"))     op.c = atof(v);
        if (auto v = attr(atts, "z"))     op.d = atof(v);
        ps->groupStack.back()->transforms.push_back(op);

    } else if (strcmp(name, "scale") == 0) {
        if (ps->groupStack.empty()) return;
        TransformOp op;
        op.type = TransformType::SCALE;
        if (auto v = attr(atts, "x")) op.a = atof(v);
        if (auto v = attr(atts, "y")) op.b = atof(v);
        if (auto v = attr(atts, "z")) op.c = atof(v);
        ps->groupStack.back()->transforms.push_back(op);

    // Modelo
    } else if (strcmp(name, "model") == 0) {
        if (auto f = attr(atts, "file")) {
            // Se ainda não há nenhum grupo na pilha, os modelos vão para root
            Group* g = ps->groupStack.empty()
                       ? &ps->scene->root
                       : ps->groupStack.back();

            Mesh mesh;
            mesh.filename = ps->dir + f;
            FILE* mf = fopen(mesh.filename.c_str(), "r");
            if (mf) {
                int n; fscanf(mf, "%d", &n);
                mesh.verts.resize(n);
                for (int i = 0; i < n; ++i) {
                    auto& v = mesh.verts[i];
                    fscanf(mf, "%f %f %f %f %f %f %f %f",
                           &v.x, &v.y, &v.z,
                           &v.nx, &v.ny, &v.nz,
                           &v.u,  &v.v);
                }
                fclose(mf);
                printf("  Modelo: '%s' (%d triangulos)\n",
                       mesh.filename.c_str(), n / 3);
                g->meshes.push_back(std::move(mesh));
            } else {
                fprintf(stderr, "Aviso: nao foi possivel abrir '%s'\n",
                        mesh.filename.c_str());
            }
        }
    }
}

static void XMLCALL onEnd(void* ud, const XML_Char* name) {
    auto* ps = static_cast<ParseState*>(ud);
    // ao fechar um <group>, retira da pilha
    if (strcmp(name, "group") == 0 && !ps->groupStack.empty())
        ps->groupStack.pop_back();
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
    return ok;
}