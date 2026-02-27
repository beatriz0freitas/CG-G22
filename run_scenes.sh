#!/usr/bin/env bash
# =============================================================================
# run_scenes.sh — Corre as scenes da pasta scenes/
#
# Uso:
#   ./run_scenes.sh                  → corre todas as scenes em sequência
#   ./run_scenes.sh --scene <nome>   → corre só uma scene (ex: cone, sphere, ...)
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
SCENES_DIR="$SCRIPT_DIR/scenes"
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
            echo "  ./run_scenes.sh                  → corre todas as scenes"
            echo "  ./run_scenes.sh --scene <nome>   → corre só uma (ex: cone)"
            echo "  ./run_scenes.sh --list           → lista as scenes disponíveis"
            exit 0
            ;;
        *) error "Argumento desconhecido: $1  (usa --help)" ;;
    esac
    shift
done

# Verifica se o engine existe
[[ -x "$ENGINE" ]] || error "Engine não encontrado: $ENGINE\nCompila primeiro com ./run.sh --build-only"

# Verifica se a pasta scenes existe
[[ -d "$SCENES_DIR" ]] || error "Pasta scenes não encontrada: $SCENES_DIR"

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

elif [[ "$MODE" == "one" ]]; then
    [[ -z "$SCENE_NAME" ]] && error "--scene requer um nome (ex: cone)"
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