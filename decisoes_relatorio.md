
# Decisões de Arquitetura — CG Engine

Registo das principais decisões tomadas durante o desenvolvimento, com a justificação de cada uma.

---

## Formato dos ficheiros `.3d`

O formato inclui desde a Fase 1 todos os campos necessários nas 4 fases:

```
<número de vértices>
x y z nx ny nz u v
```

Os campos `nx ny nz` (normal) e `u v` (coordenadas de textura) são escritos a zero pelo generator nas Fases 1 a 3. O engine lê sempre os 8 campos.

A alternativa seria usar apenas `x y z` agora e alterar o formato mais tarde. Essa abordagem foi rejeitada porque implicaria regenerar todos os modelos e atualizar o parser na Fase 4 — trabalho desnecessário. Com o formato completo desde o início, a Fase 4 apenas preenche campos que já existem, sem quebrar compatibilidade com ficheiros anteriores.

---

## Generator

### Um ficheiro por primitiva

Cada primitiva geométrica tem o seu próprio par `.h` / `.cpp` (`plane`, `box`, `sphere`, `cone`, `cylinder`, `torus`), em vez de um único ficheiro com tudo.

Isto significa que para encontrar ou corrigir a geração de uma esfera, por exemplo, vais diretamente a `sphere.cpp` sem navegar por centenas de linhas de código não relacionado. Quando a Fase 3 introduzir superfícies de Bézier, basta adicionar `bezier.h` + `bezier.cpp` sem tocar em nenhum dos ficheiros existentes.

### Pasta `primitives/`

Os ficheiros de cada primitiva estão agrupados em `generator/primitives/`. O `filewriter` fica fora desta pasta, ao lado do `main.cpp`, porque não é uma primitiva — é uma utilidade de suporte partilhada por todas. A separação torna a distinção de responsabilidades visualmente óbvia.

### `filewriter` partilhado

A escrita para ficheiro e a definição de `Vertex` e `Model` estão num único sítio (`filewriter.h` / `filewriter.cpp`). Todas as primitivas incluem este ficheiro. Se o formato `.3d` precisar de mudar (por exemplo, para incluir normais na Fase 4), a alteração é feita num único sítio.

### Primitiva `cylinder` (extra)

O cilindro é parametrizado por raio, altura, slices (divisões angulares) e stacks (divisões verticais).

A superfície lateral é gerada com normal puramente radial `(sin θ, 0, cos θ)` — correta para um cilindro, onde a normal é perpendicular ao eixo e ao raio, sem componente vertical. As tampas usam normais `(0, ±1, 0)`. Esta separação de normais é essencial para o shading correto na Fase 4. A flag `capped` (por omissão `true`) permite gerar cilindros abertos, útil para construir outros objetos compostos.

UV da lateral: `u = θ / 2π`, `v = y / height` (projeção cilíndrica).
UV das tampas: projeção planar centrada `u = 0.5 + sin(θ) * 0.5`, `v = 0.5 + cos(θ) * 0.5`.

### Primitiva `torus` (extra)

O torus é parametrizado por dois ângulos: `φ` (volta em torno do eixo Y, o "anel principal") e `θ` (volta em torno do tubo). A parametrização é:

```
P(φ, θ) = ((R + r·cos θ)·cos φ,  r·sin θ,  (R + r·cos θ)·sin φ)
```

A normal em qualquer ponto é o vetor do centro do tubo para o ponto, que simplifica para:

```
N(φ, θ) = (cos θ·cos φ,  sin θ,  cos θ·sin φ)
```

Este vetor é unitário por construção (`||N||² = cos²θ·cos²φ + sin²θ + cos²θ·sin²φ = cos²θ + sin²θ = 1`), sem necessidade de normalização explícita — análogo ao raciocínio da esfera no projeto.

UV: `u = φ / 2π` (posição no anel), `v = θ / 2π` (posição no tubo), ambos em [0,1], compatíveis com texturas de repetição na Fase 4.

Os parâmetros `outerRadius` (R) e `innerRadius` (r) são expostos diretamente ao utilizador em vez de, por exemplo, `centerRadius` + `tubeRadius`, porque são mais intuitivos: `outerRadius` é o raio máximo do donut (do centro ao ponto mais exterior) e `innerRadius` é o raio do tubo.

---

## Engine

### `scene.h` — estruturas de dados separadas da lógica

Todas as estruturas de dados da cena (`Mesh`, `Group`, `Transform`, `Camera`, `Scene`) estão num único header, sem lógica associada.

A vantagem é que cada fase adiciona campos a estas estruturas sem obrigar a tocar na lógica de parsing ou de rendering.
Os comentários no ficheiro já antecipam o que cada fase vai adicionar, funcionando como um guia de desenvolvimento.

### Parser XML com expat — `xmlParser.h` / `xmlParser.cpp`

O parsing XML está isolado num par de ficheiros dedicado. A biblioteca escolhida foi o  **expat** , um parser SAX (event-driven) que processa o XML linha a linha sem construir uma árvore em memória, o que é mais eficiente e suficiente para o formato que temos.

A alternativa mais comum seria o **tinyxml2** (parser DOM), que constrói uma árvore completa do documento em memória antes de qualquer leitura. Para um formato de configuração de cena, onde percorremos o documento uma única vez do início ao fim, o modelo SAX do expat é mais direto e sem overhead desnecessário. O expat está também disponível como pacote de sistema (`libexpat1-dev`), sem necessidade de copiar ficheiros para o repositório.

O `xmlParser` estar separado do `main.cpp` é a decisão com maior impacto na longevidade do projeto: na Fase 1 o XML tem ~5 tags; na Fase 4 terá transforms animados, luzes, materiais e texturas. Com o parser isolado, esse crescimento acontece num único ficheiro sem nunca tocar nos callbacks GLUT.

### `renderer.h` / `renderer.cpp` — lógica de desenho separada

A lógica de desenho OpenGL está isolada do `main.cpp`.

A razão principal é a Fase 3, que exige a transição de modo imediato (`glBegin` / `glEnd`) para VBOs. Com o renderer separado, essa substituição é feita num único ficheiro. Os callbacks GLUT e a lógica de câmara do `main.cpp` não são tocados.

A função `renderGroup` dentro do renderer já está estruturada de forma recursiva (com os comentários da Fase 2 presentes), tornando a adição de hierarquia de grupos uma alteração mínima e localizada.

### Três modos de renderização togglávéis (extra)

O renderer suporta três modos ciclados com a tecla `M`:

* **Wireframe** — `GL_FRONT_AND_BACK, GL_LINE`. Modo padrão na Fase 1, sem culling.
* **Solid** — `GL_FRONT_AND_BACK, GL_FILL` com cor cinzento. Útil para verificar geometria.
* **Solid+Wireframe** — dois passes: primeiro fill com `glPolygonOffset(1, 1)` para evitar z-fighting, depois wireframe por cima em verde. Permite inspecionar a topologia da malha sem perder a forma do sólido.

O `glPolygonOffset` é necessário no terceiro modo para evitar z-fighting: sem ele, os pixels do wireframe ficam à mesma profundidade que os do fill e o resultado é flickering. O offset de `(1, 1)` é o valor standard para este efeito.

### FPS no título da janela (extra)

O FPS é calculado no callback `display()` e atualizado no título da janela com `glutSetWindowTitle()` a cada 500 ms (para evitar overhead demasiado frequente). O título mostra também o modo de renderização atual e o estado dos eixos, tornando a janela auto-documentada durante o desenvolvimento.

### Scroll do rato para zoom (extra)

Os botões 3 e 4 do rato (scroll up/down) são mapeados para zoom in/out (`g_radius *= 0.95f` / `1.05f`). É mais ergonómico do que W/S para ajustes rápidos de distância. O fator multiplicativo (em vez de aditivo) dá zoom proporcional à distância atual.

### Toggle de eixos com tecla `X` (extra)

A variável `g_showAxes` em `renderer.h` permite ligar/desligar os eixos XYZ com a tecla `X`. Os eixos têm setas no topo para indicar direção positiva. São úteis durante o desenvolvimento mas podem ser escondidos para screenshots limpos.

### `main.cpp` com responsabilidade única

O `main.cpp` trata apenas de três coisas: inicialização do GLUT, câmara orbital interativa, e orquestração das outras partes. Não contém parsing nem lógica de desenho.

O resultado é que este ficheiro raramente precisa de ser alterado nas fases seguintes — está essencialmente completo desde a Fase 1.

### Câmara orbital com alpha, beta, radius

A câmara é controlada por três parâmetros esféricos: `alpha` (ângulo horizontal), `beta` (ângulo vertical) e `radius` (distância ao lookAt). A posição cartesiana é recalculada a partir destes sempre que há input.

Esta abordagem foi escolhida por alinhar com as orientações da UC, que especificam coordenadas esféricas como base do modelo de câmara. Permite implementar órbita (A/D/Q/E e rato) e zoom (W/S e scroll) de forma matematicamente consistente.

O vetor de direção D é calculado a partir de alpha e beta e é normalizado por construção (`||D||² = cos²β + sin²β = 1`), sem necessidade de normalização explícita.

### Look at point fixo vs. movimento forward/backward

Os slides da UC (p03, slide 6) especificam dois comportamentos distintos:

* **Explorer Mode** (A/D/Q/E, rato, scroll): câmara orbita à volta do lookAt com coordenadas esféricas (alpha, beta, radius). O lookAt é fixo durante a órbita.
* **FPS forward/backward** (W/S): a câmara translada ao longo do vetor de direção D, e o lookAt acompanha com o mesmo deslocamento: `P' = P + k×D`, `lookAt' = lookAt + k×D`.

O vetor D é calculado a partir de alpha e beta — é unitário por construção (`||D||² = cos²β·sin²α + sin²β + cos²β·cos²α = 1`), não requer normalização explícita. O zoom puro (alterar apenas radius sem mover o lookAt) fica reservado ao scroll do rato.

Esta separação é fiel ao modelo descrito nos slides e evita a ambiguidade de ter W/S a fazer zoom orbital (o que seria concetualmente inconsistente com um motor de cena).

### `up` vector fixo em (0, 1, 0)

O `gluLookAt` usa sempre `up = (0, 1, 0)` hardcoded no renderer, conforme especificado nas orientações da UC. Isto é seguro porque `|beta| < 1.5` rad garante que a câmara nunca fica paralela ao eixo Y, evitando o gimbal lock.

O campo `up` continua a existir na estrutura `Camera` por ser lido do XML (onde é definido por convenção), mas não é passado ao `gluLookAt`.

### `glutPostRedisplay` sem idle function

A cena só é redesenhada quando o teclado ou o rato provocam uma alteração, chamando `glutPostRedisplay()`. Não há `glutIdleFunc` registada — os slides da UC não a mencionam e, para cenas estáticas, uma idle function redesenharia continuamente sem necessidade, desperdiçando CPU. Esta abordagem é suficiente para todas as fases, incluindo a Fase 3 com animações — nesse caso basta registar uma idle function que chame `glutPostRedisplay()` apenas quando o estado da animação mudar.

---

## O que vai além do mínimo do enunciado (extras)

O enunciado da Fase 1 exige apenas que o engine leia o XML e exiba os modelos. As seguintes funcionalidades foram adicionadas além do mínimo:

**Generator:**

* Primitiva `cylinder` (raio, altura, slices, stacks; tampas incluídas)
* Primitiva `torus` (outerRadius, innerRadius, sides, rings)
* Normais corretas em todas as primitivas desde a Fase 1 (prontas para a Fase 4)
* Coordenadas UV em todas as primitivas desde a Fase 1 (prontas para a Fase 4)

**Engine:**

* Três modos de renderização togglávéis com `M` (wireframe, solid, solid+wireframe)
* Toggle de eixos XYZ com `X`
* Eixos XYZ com setas de direção para melhor legibilidade
* Scroll do rato para zoom
* Contador de FPS no título da janela
* Fundo azul-escuro para melhor contraste visual com geometria branca/colorida
* Formato `.3d` já preparado com normais e UV para as Fases 3 e 4
* Estrutura de código com separação de responsabilidades preparada para as 4 fases
