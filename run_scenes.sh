#!/usr/bin/env bash
# =============================================================================
# run_scenes.sh — Compila, gera modelos e corre as scenes da pasta scenes/
#
# Uso:
#   ./run_scenes.sh                  → compila + gera modelos + corre todas as scenes
#   ./run_scenes.sh --scene <nome>   → compila + gera modelos + corre só uma scene
#   ./run_scenes.sh --list           → lista as scenes disponíveis
#   ./run_scenes.sh --help           → mostra esta ajuda
# =============================================================================

set -e

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()   { echo -e "${GREEN}[✓]${NC} $*"; }
warn()   { echo -e "${YELLOW}[!]${NC} $*"; }
error()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }
header() { echo -e "\n${BOLD}${CYAN}$*${NC}"; echo "────────────────────────────────────────"; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="$SCRIPT_DIR/build"
MODELS_DIR="$SCRIPT_DIR/models"
SCENES_DIR="$SCRIPT_DIR/scenes"
GENERATOR="$BUILD_DIR/generator/generator"
ENGINE="$BUILD_DIR/engine/engine"

SCENES=(
    "box"
    "cone"
    "cylinder"
    "plane"
    "sphere"
    "torus"
    "test_all"
)

MODE="all"
SCENE_NAME=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --scene) shift; SCENE_NAME="$1"; MODE="one" ;;
        --list)  MODE="list" ;;
        --help|-h)
            echo "Uso:"
            echo "  ./run_scenes.sh                  → compila + gera + corre todas as scenes"
            echo "  ./run_scenes.sh --scene <nome>   → compila + gera + corre só uma (ex: cone)"
            echo "  ./run_scenes.sh --list           → lista as scenes disponíveis"
            exit 0
            ;;
        *) error "Argumento desconhecido: $1  (usa --help)" ;;
    esac
    shift
done

# ── 1. COMPILAÇÃO ────────────────────────────────────────────────────────────
header "COMPILAÇÃO"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

info "cmake configure ..."
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -E "(error:|warning:|--)" || true

JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
info "cmake build (-j$JOBS) ..."
cmake --build . -- -j"$JOBS"

[[ -x "$GENERATOR" ]] || error "generator não foi criado: $GENERATOR"
[[ -x "$ENGINE"    ]] || error "engine não foi criado: $ENGINE"
info "Compilação OK."

cd "$SCRIPT_DIR"

# ── 2. GERAÇÃO DE MODELOS ────────────────────────────────────────────────────
header "GERAÇÃO DE MODELOS"

gen() {
    info "generator $*"
    "$GENERATOR" "$@"
}

mkdir -p "$MODELS_DIR"
gen plane    1   3         "$MODELS_DIR/plane.3d"
gen box      2   3         "$MODELS_DIR/box.3d"
gen sphere   1   10  10    "$MODELS_DIR/sphere.3d"
gen cone     1   2   4   3 "$MODELS_DIR/cone.3d"
gen cylinder 1   2   16  4 "$MODELS_DIR/cylinder.3d"
gen torus    1   0.3 16 32 "$MODELS_DIR/torus.3d"

info "Modelos gerados em $MODELS_DIR"

# ── 3. SCENES ────────────────────────────────────────────────────────────────
if [[ "$MODE" == "list" ]]; then
    header "SCENES DISPONÍVEIS"
    for s in "${SCENES[@]}"; do
        xml="$SCENES_DIR/${s}.xml"
        if [[ -f "$xml" ]]; then
            info "$s"
        else
            warn "$s  (ficheiro não encontrado)"
        fi
    done
    exit 0
fi

run_scene() {
    local name="$1"
    local xml="$SCENES_DIR/${name}.xml"
    [[ -f "$xml" ]] || { warn "Scene não encontrada: $xml — a saltar"; return 1; }
    echo ""
    echo -e "${BOLD}  Scene:${NC} ${name}.xml"
    echo -e "  ${CYAN}Fecha a janela para continuar.${NC}"
    echo ""
    "$ENGINE" "$xml"
}

if [[ "$MODE" == "one" ]]; then
    [[ -z "$SCENE_NAME" ]] && error "--scene requer um nome (ex: cone)"
    header "A CORRER SCENE: $SCENE_NAME"
    run_scene "$SCENE_NAME"
else
    header "A CORRER TODAS AS SCENES"
    echo -e "  ${YELLOW}Fecha cada janela para avançar para a próxima.${NC}"
    PASS=0; FAIL=0
    for s in "${SCENES[@]}"; do
        echo -e "\n${BOLD}── Scene: ${s} ──${NC}"
        if run_scene "$s"; then
            info "$s: OK"; (( PASS++ )) || true
        else
            warn "$s: falhou"; (( FAIL++ )) || true
        fi
    done
    header "RESULTADO"
    echo -e "  ${GREEN}Passaram: $PASS${NC}   ${RED}Falharam: $FAIL${NC}   Total: ${#SCENES[@]}"
fi

echo ""
info "run_scenes.sh concluído."