# CG Engine — Fase 1

Motor 3D minimalista desenvolvido no âmbito da UC de Computação Gráfica. Lê uma cena descrita em XML e renderiza os modelos em wireframe com navegação orbital interativa.

---

## Dependências

### Ferramentas base

Estas ferramentas são necessárias para compilar o projeto e devem ser instaladas manualmente caso ainda não estejam disponíveis no sistema.

| Ferramenta   | Versão mínima | Instalação                                      |
| ------------ | ------------- | ----------------------------------------------- |
| GCC ou Clang | 7+            | [gcc.gnu.org](https://gcc.gnu.org)              |
| CMake        | 3.10+         | [cmake.org/install](https://cmake.org/install/) |

### Bibliotecas específicas do projeto

**Linux (Ubuntu/Debian):**

```bash
sudo apt install freeglut3-dev libexpat1-dev
```

**macOS:**

```bash
brew install expat
```

> O GLUT está incluído no macOS via `GLUT.framework` (parte do Xcode Command Line Tools).
> Instala com: `xcode-select --install`

| Biblioteca | Utilização                                        |
| ---------- | ------------------------------------------------- |
| freeGLUT   | Janela OpenGL, callbacks de input, loop principal |
| expat      | Parsing do ficheiro XML de configuração da cena   |

---

## Compilação

Na raiz do projeto:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Os executáveis ficam em `build/generator/generator` e `build/engine/engine`.

---

## Utilização

### 1. Gerar os modelos

A partir da pasta `build/`:

```bash
mkdir -p ../models

./generator/generator plane  1 3      ../models/plane.3d
./generator/generator box    2 3      ../models/box.3d
./generator/generator sphere 1 10 10  ../models/sphere.3d
./generator/generator cone   1 2 4 3  ../models/cone.3d
./generator/generator cylinder 1 2 16 4  ../models/cylinder.3d
./generator/generator torus    1 0.3 16 32 ../models/torus.3d
```

Sintaxe completa:

```
generator plane  <length> <divisions> <output.3d>
generator box    <size> <divisions> <output.3d>
generator sphere <radius> <slices> <stacks> <output.3d>
generator cone   <radius> <height> <slices> <stacks> <output.3d>
generator cylinder <radius> <height> <slices> <stacks> <output.3d>
generator torus    <outerRadius> <innerRadius> <sides> <rings> <output.3d>
```

### 2. Correr o engine

O engine recebe um ficheiro XML de configuração. Os caminhos dos modelos no XML são relativos à localização do próprio ficheiro XML.

```bash
./engine/engine ../scenes/plane.xml
./engine/engine ../scenes/sphere.xml
./engine/engine ../scenes/cylinder.xml
./engine/engine ../scenes/torus.xml
./engine/engine ../scenes/test_all.xml
```

---

## Controlos

| Tecla / Ação                 | Efeito                                          |
| ---------------------------- | ----------------------------------------------- |
| `W`/`S`                      | Avança / recua ao longo de D (lookAt acompanha) |
| `A`/`D`                      | Órbita horizontal                               |
| `Q`/`E`                      | Órbita vertical                                 |
| `R`                          | Reset câmara para a posição definida no XML     |
| `M`                          | Cicla modo: Wireframe → Solid → Solid+Wireframe |
| `X`                          | Toggle eixos XYZ                                |
| Scroll do rato               | Zoom in / out                                   |
| Rato (botão esq. + arrastar) | Órbita livre                                    |
| `ESC`                        | Fechar                                          |

O título da janela mostra em tempo real o FPS, o modo de renderização ativo e o estado dos eixos

---

## Formato dos ficheiros `.3d`

Formato de texto definido pelo grupo, desenhado para suportar as 4 fases sem alterações à estrutura:

```
<número de vértices>
x y z nx ny nz u v
x y z nx ny nz u v
...
```

Cada 3 vértices consecutivos formam um triângulo. Os campos têm o seguinte significado:

| Campo      | Descrição              | Fase em que é preenchido |
| ---------- | ---------------------- | ------------------------ |
| `x y z`    | Posição do vértice     | Fase 1                   |
| `nx ny nz` | Normal do vértice      | Fase 4                   |
| `u v`      | Coordenadas de textura | Fase 4                   |

## Script de Automação (`run.sh`)

O projeto inclui um script que automatiza:

- Compilação com CMake
- Geração automática dos modelos `.3d`
- Execução dos testes da Fase 1
- Limpeza da build e ficheiros gerados

---

### Dar permissões de execução

```bash
chmod +x run.sh
```

| Comando                           | Efeito                                                        |
| --------------------------------- | ------------------------------------------------------------- |
| `./run.sh`                        | Compila + gera todos os modelos + corre os 5 testes da Fase 1 |
| `./run.sh --build-only`           | Apenas compila                                                |
| `./run.sh --no-tests`             | Compila + gera modelos (sem abrir o engine)                   |
| `./run.sh --test <n>`             | Corre apenas o teste N (1–5)                                  |
| `./run.sh --scene <ficheiro.xml>` | Abre uma scene arbitrária                                     |
| `./run.sh --clean`                | Remove `build/` e modelos gerados                             |
| `./run.sh --help`                 | Mostra a ajuda                                                |
