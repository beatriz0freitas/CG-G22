#!/usr/bin/env bash
# Compila o projeto, gera os modelos e corre os testes.
#
# Uso:
#   ./run.sh                  - compila + gera + testes fase 1, fase 2 e fase 3
#   ./run.sh --scenes         - corre todas as scenes da pasta scenes/
#   ./run.sh --test1 <n>      - corre apenas o teste N da fase 1
#   ./run.sh --test2 <n>      - corre apenas o teste N da fase 2
#   ./run.sh --test3 <n>      - corre apenas o teste N da fase 3
#   ./run.sh --test4 <n>      - corre apenas o teste N da fase 4
#   ./run.sh --scene <nome>   - corre uma scene especifica (ex: cone)
#   ./run.sh --build-only     - so compila, nao gera nem corre nada
#   ./run.sh --clean          - apaga build/ e todos os .3d gerados

set -e

# Muda para o diretorio raiz do projeto (um nivel acima de uteis/)
cd "$(dirname "$0")"/.. || exit 1

# abre o engine com um xml; devolve 1 se o ficheiro nao existir
run_engine() {
    if [[ ! -f "$1" ]]; then
        echo "  Ficheiro nao encontrado: $1 — a saltar"
        return 1
    fi
    echo "  A abrir: $1 (fecha a janela para continuar)"
    build/engine/engine "$1"
}

gen() {
    echo "  generator $*"
    build/generator/generator "$@"
}

if [[ "${1:-}" == "--clean" ]]; then
    echo "A limpar..."
    rm -rf build/ models/
    find test_files/test_files_phase_1 test_files/test_files_phase_2 test_files/test_files_phase_3 test_files/test_files_phase_4 -name "*.3d" -delete 2>/dev/null || true
    echo "Pronto."
    exit 0
fi

# Compilacao (comum a todos os modos restantes)
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=1 > /dev/null 2>&1

JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
cmake --build . -- -j"$JOBS"

[[ -x "generator/generator" ]] || { echo "Erro: generator nao foi criado."; exit 1; }
[[ -x "engine/engine"       ]] || { echo "Erro: engine nao foi criado."; exit 1; }

cd ..


if [[ "${1:-}" == "--build-only" ]]; then
    exit 0
fi

# Geracao de modelos (comum a todos os modos restantes)
mkdir -p models

gen plane    1   3         models/plane.3d
gen box      2   3         models/box.3d
gen sphere   1   10  10    models/sphere.3d
gen cone     1   2   4  3  models/cone.3d
gen cylinder 1   2   16  4 models/cylinder.3d
gen torus    1   0.3 16 32 models/torus.3d
gen bezier   uteis/teapot.patch 10 models/bezier_10.3d

# os modelos de teste ficam ao lado dos XMLs porque o engine resolve caminhos relativos ao ficheiro XML
if [[ -d "test_files/test_files_phase_1" ]]; then
    gen cone   1 2 4  3  test_files/test_files_phase_1/cone_1_2_4_3.3d
    gen sphere 1 10 10   test_files/test_files_phase_1/sphere_1_10_10.3d
    gen box    2 3       test_files/test_files_phase_1/box_2_3.3d
    gen plane  2 3       test_files/test_files_phase_1/plane_2_3.3d
fi

if [[ -d "test_files/test_files_phase_2" ]]; then
    gen sphere 1 8  8    test_files/test_files_phase_2/sphere_1_8_8.3d
    gen box    2 3       test_files/test_files_phase_2/box_2_3.3d
    gen cone   1 2 4 3   test_files/test_files_phase_2/cone_1_2_4_3.3d
fi

if [[ -d "test_files/test_files_phase_3" ]]; then
    gen bezier uteis/teapot.patch 10 test_files/test_files_phase_3/bezier_10.3d
fi

if [[ -d "test_files/test_files_phase_4" ]]; then
    gen plane  2 3       test_files/test_files_phase_4/plane_2_3.3d
    gen box    2 3       test_files/test_files_phase_4/box_2_3.3d
    gen cone   1 2 4 3   test_files/test_files_phase_4/cone_1_2_4_3.3d
    gen sphere 1 8 8     test_files/test_files_phase_4/sphere_1_8_8.3d
    gen bezier uteis/teapot.patch 10 test_files/test_files_phase_4/bezier_10.3d
fi

if [[ "${1:-}" == "--test1" ]]; then
    [[ -n "$2" ]] || { echo "Indica o numero do teste (1-5)."; exit 1; }
    run_engine "test_files/test_files_phase_1/test_1_${2}.xml"
    exit 0
fi

if [[ "${1:-}" == "--test2" ]]; then
    [[ -n "$2" ]] || { echo "Indica o numero do teste (1-4)."; exit 1; }
    run_engine "test_files/test_files_phase_2/test_2_${2}.xml"
    exit 0
fi

if [[ "${1:-}" == "--test3" ]]; then
    [[ -n "$2" ]] || { echo "Indica o numero do teste (1-2)."; exit 1; }
    run_engine "test_files/test_files_phase_3/test_3_${2}.xml"
    exit 0
fi

if [[ "${1:-}" == "--test4" ]]; then
    [[ -n "$2" ]] || { echo "Indica o numero do teste (1-6)."; exit 1; }
    run_engine "test_files/test_files_phase_4/test_4_${2}.xml"
    exit 0
fi

if [[ "${1:-}" == "--scene" ]]; then
    [[ -n "$2" ]] || { echo "Indica o nome da scene (ex: cone)."; exit 1; }
    run_engine "scenes/${2}.xml"
    exit 0
fi

if [[ "${1:-}" == "--scenes" ]]; then
    echo ""
    echo "=== Scenes ==="
    for xml in scenes/*.xml; do
        run_engine "$xml" || true
    done
    echo ""
    echo "Concluido."
    exit 0
fi

# modo padrao: testes fase 1 + fase 2 + fase 3
for n in 1 2 3 4 5; do
    run_engine "test_files/test_files_phase_1/test_1_${n}.xml" && echo "  Teste 1.$n OK" || echo "  Teste 1.$n falhou"
done

for n in 1 2 3 4; do
    run_engine "test_files/test_files_phase_2/test_2_${n}.xml" && echo "  Teste 2.$n OK" || echo "  Teste 2.$n falhou"
done

for n in 1 2; do
    run_engine "test_files/test_files_phase_3/test_3_${n}.xml" && echo "  Teste 3.$n OK" || echo "  Teste 3.$n falhou"
done
