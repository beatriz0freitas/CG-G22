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

Cada primitiva geométrica tem o seu próprio par `.h` / `.cpp` (`plane`, `box`, `sphere`, `cone`), em vez de um único ficheiro com tudo.

Isto significa que para encontrar ou corrigir a geração de uma esfera, por exemplo, vais diretamente a `sphere.cpp` sem navegar por centenas de linhas de código não relacionado. Quando a Fase 3 introduzir superfícies de Bézier, basta adicionar `bezier.h` + `bezier.cpp` sem tocar em nenhum dos ficheiros existentes.

### Pasta `primitives/`

Os ficheiros de cada primitiva estão agrupados em `generator/primitives/`. O `filewriter` fica fora desta pasta, ao lado do `main.cpp`, porque não é uma primitiva — é uma utilidade de suporte partilhada por todas. A separação torna a distinção de responsabilidades visualmente óbvia.

### `filewriter` partilhado

A escrita para ficheiro e a definição de `Vertex` e `Model` estão num único sítio (`filewriter.h` / `filewriter.cpp`). Todas as primitivas incluem este ficheiro. Se o formato `.3d` precisar de mudar (por exemplo, para incluir normais na Fase 4), a alteração é feita
num único sítio.

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

### `main.cpp` com responsabilidade única

O `main.cpp` trata apenas de três coisas: inicialização do GLUT, câmara orbital interativa, e orquestração das outras partes. Não contém parsing nem lógica de desenho.

O resultado é que este ficheiro raramente precisa de ser alterado nas fases seguintes — está essencialmente completo desde a Fase 1.

### Câmara orbital com alpha, beta, radius

A câmara é controlada por três parâmetros esféricos: `alpha` (ângulo horizontal), `beta` (ângulo vertical) e `radius` (distância ao lookAt). A posição cartesiana é recalculada a partir destes sempre que há input.

Esta abordagem foi escolhida por alinhar com as orientações da UC, que especificam coordenadas esféricas como base do modelo de câmara. Permite implementar órbita (A/D/Q/E e rato) e movimento forward/backward (W/S)
de forma matematicamente consistente.

O vetor de direção D é calculado a partir de alpha e beta e é normalizado por construção (||D||² = cos²β + sin²β = 1), sem necessidade de normalização explícita.

### Look at point fixo vs. movimento forward/backward

O enunciado especifica que o look at point é fixo em (0,0,0) para o modo orbital. As orientações da UC especificam ainda um modo de movimento forward/backward onde o look at acompanha a câmara ao longo de D, mantendo a orientação constante. Estes são dois comportamentos distintos para dois modos de interação diferentes, não uma contradição:

* **Órbita** (A/D/Q/E, rato): look at fixo, câmara move-se à volta dele.
* **Forward/backward** (W/S): look at acompanha, câmara translada ao longo de D.

### Face culling — decisão de não ativar na Fase 1

O face culling (`glEnable(GL_CULL_FACE)`) não está ativo na Fase 1. A razão é que o rendering é feito em wireframe com `GL_FRONT_AND_BACK`, e com culling ativo metade das arestas seria removida, degradando
a visualização.

O culling será ativado no `rendererInit` a partir da Fase 4, quando o rendering passar a modo sólido com iluminação — contexto onde o culling tem impacto visual correto e benefício de performance real.

O winding order (CCW) está definido corretamente em todas as primitivas desde já, pelo que ativar o culling na Fase 4 não exigirá alterações na geometria gerada.

### `up` vector fixo em (0, 1, 0)

O `gluLookAt` usa sempre `up = (0, 1, 0)` hardcoded no renderer, conforme especificado nas orientações da UC. Isto é seguro porque `|beta| < 1.5` rad garante que a câmara nunca fica paralela ao eixo Y, evitando o gimbal lock.

O campo `up` continua a existir na estrutura `Camera` por ser lido do XML (onde é definido por convenção), mas não é passado ao `gluLookAt`.

### `glutPostRedisplay` sem idle function

A cena só é redesenhada quando o teclado ou o rato provocam uma alteração, chamando `glutPostRedisplay()`. Não há `glutIdleFunc` registada.

Para cenas estáticas como a Fase 1, uma idle function estaria a redesenhar continuamente sem necessidade, desperdiçando CPU. Esta abordagem está alinhada com as orientações da UC e é suficiente para todas as fases, incluindo a Fase 3 com animações — nesse caso basta registar uma idle function que chame `glutPostRedisplay()` apenas quando o estado da animação mudar.

---

## O que vai além do mínimo do enunciado (extras)

O enunciado da Fase 1 exige apenas que o engine leia o XML e exiba os modelos. As seguintes funcionalidades foram adicionadas além do mínimo, sendo positivas para a avaliação:

* Controlos de câmara interativos com teclado (W/S/A/D/Q/E/R) e rato
* Eixos XYZ a cores para orientação visual durante o desenvolvimento
* Formato `.3d` já preparado com normais e UV para as Fases 3 e 4
* Estrutura de código com separação de responsabilidades preparada para as 4 fases

---
