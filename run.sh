#!/usr/bin/env bash
# =============================================================================
# run.sh — Compila, gera modelos e corre testes
#
# Uso:
#   ./run.sh                    → compila + gera modelos + corre todos os testes
#   ./run.sh --build-only       → só compila
#   ./run.sh --no-tests         → compila + gera modelos (sem abrir engine)
#   ./run.sh --test <n>         → corre apenas o teste N (1-5)
#   ./run.sh --scene <xml>      → abre uma scene arbitrária
#   ./run.sh --clean            → apaga build/ e modelos gerados
#   ./run.sh --help             → mostra esta ajuda
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
TEST_DIR="$SCRIPT_DIR/test files/test_files_phase_1"

GENERATOR="$BUILD_DIR/generator/generator"
ENGINE="$BUILD_DIR/engine/engine"

# .3d gerados na pasta de testes (para limpar no fim)
TEST_MODELS=(
    "$TEST_DIR/cone_1_2_4_3.3d"
    "$TEST_DIR/sphere_1_10_10.3d"
    "$TEST_DIR/box_2_3.3d"
    "$TEST_DIR/plane_2_3.3d"
)

cleanup_test_models() {
    header "LIMPEZA"
    for f in "${TEST_MODELS[@]}"; do
        if [[ -f "$f" ]]; then
            rm "$f"
            info "Removido: $(basename "$f")"
        fi
    done
}

MODE="all"
TEST_N=""
SCENE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-only)  MODE="build-only" ;;
        --no-tests)    MODE="no-tests" ;;
        --test)        shift; TEST_N="$1"; MODE="test-one" ;;
        --scene)       shift; SCENE="$1"; MODE="custom-scene" ;;
        --clean)
            header "CLEAN"
            rm -rf "$BUILD_DIR"
            rm -rf "$MODELS_DIR"
            cleanup_test_models
            info "Tudo limpo."
            exit 0
            ;;
        --help|-h)
            echo "Uso:"
            echo "  ./run.sh                 → compila + gera + todos os testes"
            echo "  ./run.sh --build-only    → só compila"
            echo "  ./run.sh --no-tests      → compila + gera modelos"
            echo "  ./run.sh --test <n>      → só o teste N (1–5)"
            echo "  ./run.sh --scene <xml>   → abre uma scene à escolha"
            echo "  ./run.sh --clean         → apaga build/ e modelos gerados"
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
info "Compilação OK → generator + engine prontos."

[[ "$MODE" == "build-only" ]] && { info "Modo --build-only: a terminar."; exit 0; }

# ── 2. GERAÇÃO DE MODELOS ────────────────────────────────────────────────────
header "GERAÇÃO DE MODELOS"
cd "$SCRIPT_DIR"

gen() {
    info "generator $*"
    "$GENERATOR" "$@"
}

# Modelos genéricos em models/ (ficam para uso geral)
mkdir -p "$MODELS_DIR"
gen plane    1   3         "$MODELS_DIR/plane.3d"
gen box      2   3         "$MODELS_DIR/box.3d"
gen sphere   1   10  10    "$MODELS_DIR/sphere.3d"
gen cone     1   2   4   3 "$MODELS_DIR/cone.3d"
gen cylinder 1   2   16  4 "$MODELS_DIR/cylinder.3d"
gen torus    1   0.3 16 32 "$MODELS_DIR/torus.3d"

# Modelos para os testes: gerados diretamente na pasta de testes
# (o engine resolve caminhos relativos ao XML, têm de estar ao lado dele)
[[ -d "$TEST_DIR" ]] || error "Pasta de testes não encontrada: $TEST_DIR"

gen cone   1   2   4  3  "$TEST_DIR/cone_1_2_4_3.3d"
gen sphere 1   10  10    "$TEST_DIR/sphere_1_10_10.3d"
gen box    2   3         "$TEST_DIR/box_2_3.3d"
gen plane  2   3         "$TEST_DIR/plane_2_3.3d"

info "Todos os modelos gerados."

[[ "$MODE" == "no-tests" ]] && {
    info "Modo --no-tests: modelos de teste mantidos em: $TEST_DIR"
    exit 0
}

# ── 3. TESTES ────────────────────────────────────────────────────────────────
header "TESTES"

declare -A TEST_DESC
TEST_DESC[1]="Cone  (fov=60)            cone_1_2_4_3.3d"
TEST_DESC[2]="Cone  (fov=20, zoom)      cone_1_2_4_3.3d"
TEST_DESC[3]="Esfera                    sphere_1_10_10.3d"
TEST_DESC[4]="Caixa (far=3.5, clipping) box_2_3.3d"
TEST_DESC[5]="Plano + Esfera            plane_2_3.3d + sphere_1_10_10.3d"

run_test() {
    local n="$1"
    local xml="$TEST_DIR/test_1_${n}.xml"
    [[ -f "$xml" ]] || { warn "Teste $n: XML não encontrado — a saltar"; return 1; }
    echo ""
    echo -e "${BOLD}  Teste $n/5:${NC} ${TEST_DESC[$n]}"
    echo -e "  ${CYAN}Fecha a janela para continuar.${NC}"
    echo ""
    "$ENGINE" "$xml"
}

if [[ "$MODE" == "test-one" ]]; then
    [[ -z "$TEST_N" ]] && error "--test requer um número (1-5)"
    run_test "$TEST_N"

elif [[ "$MODE" == "custom-scene" ]]; then
    [[ -z "$SCENE" ]] && error "--scene requer o caminho para um XML"
    [[ ! "$SCENE" = /* ]] && SCENE="$SCRIPT_DIR/$SCENE"
    [[ -f "$SCENE" ]] || error "Ficheiro não encontrado: $SCENE"
    "$ENGINE" "$SCENE"

else
    echo -e "  ${YELLOW}5 testes em sequência — fecha cada janela para avançar.${NC}"
    PASS=0; FAIL=0
    for n in 1 2 3 4 5; do
        echo -e "\n${BOLD}── Teste $n/5 ──  ${TEST_DESC[$n]}${NC}"
        if run_test "$n"; then
            info "Teste $n: OK"; (( PASS++ )) || true
        else
            warn "Teste $n: falhou";  (( FAIL++ )) || true
        fi
    done
    header "RESULTADO"
    echo -e "  ${GREEN}Passaram: $PASS${NC}   ${RED}Falharam: $FAIL${NC}   Total: 5"
fi

# ── 4. LIMPEZA DOS .3d DE TESTE ──────────────────────────────────────────────
cleanup_test_models

echo ""
info "run.sh concluído."