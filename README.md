# CG Engine

Motor 3D desenvolvido no âmbito da UC de Computação Gráfica. Lê cenas descritas em XML e renderiza os modelos com suporte a hierarquia de grupos, transformações geométricas e navegação interativa.

---

## Dependências

**Linux:**
```bash
sudo apt install cmake freeglut3-dev libexpat1-dev libjpeg-dev
```

**macOS:**
```bash
brew install cmake expat jpeg
# GLUT já está incluído no macOS via Xcode Command Line Tools
xcode-select --install
```

**Windows:** usa o vcpkg com o `vcpkg.json` incluído no projeto.

---

## Compilação

O script [`run.sh`](#runsh) trata de tudo: compilação, geração de modelos e testes:

```bash
chmod +x run.sh
./run.sh
```
Para compilar manualmente usar:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Os executáveis ficam em `build/generator/generator` e `build/engine/engine`.

---

## Generator

Gera um ficheiro `.3d` para cada primitiva. Corre a partir de `build/`:

```bash
./generator/generator plane    <length> <divisions>                  <out.3d>
./generator/generator box      <size> <divisions>                    <out.3d>
./generator/generator sphere   <radius> <slices> <stacks>            <out.3d>
./generator/generator cone     <radius> <height> <slices> <stacks>   <out.3d>
./generator/generator cylinder <radius> <height> <slices> <stacks>   <out.3d>
./generator/generator torus    <outerRadius> <innerRadius> <sides> <rings>  <out.3d>
./generator/generator bezier   <patch.patch> <tessellation>          <out.3d>
```

Exemplos:
```bash
./generator/generator sphere 1 10 10  ../models/sphere.3d
./generator/generator torus  1 0.3 16 32  ../models/torus.3d
```

---

## Engine

Recebe um ficheiro XML de configuração da scene. Os caminhos dos modelos no XML são relativos à localização do próprio XML.

```bash
./engine/engine ../scenes/sphere.xml
./engine/engine ../test_files/test_files_phase_2/test_2_4.xml
```

---

## Controlos

### Explorer Mode (Padrão)
| Tecla / Ação | Efeito |
|---|---|
| `W` / `S` | Órbita vertical |
| `A` / `D` | Órbita horizontal |
| `Q` / `E` | Zoom in / out |
| Setas ↑ ↓ | Avanço / recuo no plano XZ |
| Setas ← → | Deslocamento lateral |
| `Z` / `X` | Subir / descer (eixo Y) |
| Rato (botão esq. + arrastar) | Órbita livre |
| Scroll | Zoom in / out |

### Third Person Mode (Ativado com `V`)
| Tecla / Ação | Efeito |
|---|---|
| `W` / `S` | Avança / recua o character |
| `A` / `D` | Deslocamento lateral do character |
| `Z` / `X` | Subir / descer o character |
| Setas ↑ ↓ | Subir / descer o character |
| Setas ← → | Roda a câmara ao redor do character |
| `Q` / `E` | Zoom in / out |
| Rato (botão esq. + arrastar) | Roda câmara (horizontal) + move character (vertical) |
| Scroll | Zoom in / out |

### Global
| Tecla / Ação | Efeito |
|---|---|
| `V` | Alterna entre Explorer Mode e Third Person Mode |
| `M` | Cicla modo: Wireframe → Solid → Solid+Wireframe |
| `C` | Mostra / esconde a curva Catmull-Rom |
| `B` | Toggle eixos XYZ |
| `R` | Reset para Explorer Mode na posição original do XML |
| `ESC` | Fechar |

---
<a name="runsh"></a>

## Script `run.sh`

Automatiza compilação, geração de modelos e execução de testes.

```bash
./run.sh                  # compila + gera + testes fase 1, fase 2 e fase 3
./run.sh --build-only     # só compila
./run.sh --test1 <n>      # corre apenas o teste N da fase 1 (1-5)
./run.sh --test2 <n>      # corre apenas o teste N da fase 2 (1-4)
./run.sh --test3 <n>      # corre apenas o teste N da fase 3 (1-2)
./run.sh --test4 <n>      # corre apenas o teste N da fase 4 (1-7)
./run.sh --scenes         # corre todas as scenes da pasta scenes/
./run.sh --scene <nome>   # corre uma scene específica (ex: cone)
./run.sh --clean          # apaga build/ e todos os .3d gerados
```

---
**Universidade do Minho | Licenciatura em Engenharia Informática**
