# Amazon Engineering Challenge: Árvores Balanceadas (AVL e Rubro-Negra)

Protótipo em C++17 de um registro de identificadores de encomendas usando duas
estruturas implementadas do zero: **árvore AVL** e **árvore Rubro-Negra**.
Nenhuma biblioteca de árvore balanceada é usada (`std::set`/`std::map` aparecem
apenas nos testes, como modelo de referência para conferir os resultados).

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `arvore.hpp` | Interface comum (`inserir`, `remover`, `buscar`, `emOrdem`, `imprimir`, `altura`, `validar`, estatísticas) |
| `avl.hpp` | Árvore AVL |
| `rubronegra.hpp` | Árvore Rubro-Negra |
| `bst.hpp` | BST sem balanceamento, **só** como linha de base no benchmark |
| `dados.hpp` | Geração determinística dos conjuntos de dados (semente fixa) |
| `main.cpp` | Sistema interativo (seção 2 do enunciado) |
| `testes.cpp` | Testes automatizados (seção 3) |
| `benchmark.cpp` | Comparação experimental AVL x Rubro-Negra (seção 3, análise) |
| `Makefile` | Compilação |

## Como compilar e executar

```bash
make            # compila tudo
make run        # sistema interativo (escolha AVL ou Rubro-Negra no início)
make test       # testes de correção (retorna código != 0 se algum falhar)
make bench      # benchmark; também grava resultados_benchmark.csv
```
## Como gerar os gráficos dos testes
```bash
pip install matplot lib           # instala a biblioteca necessária
python graficos.py                # gera as imagens .png 
```

Sem `make` (por exemplo, no Windows com MinGW):

```bash
g++ -std=c++17 -O2 -o arvores   main.cpp
g++ -std=c++17 -O2 -o testes    testes.cpp
g++ -std=c++17 -O2 -o benchmark benchmark.cpp
```

- Terminal do Windows: rode `chcp 65001` antes, para exibir os acentos.
- Se as cores ANSI não aparecerem direito, use `./arvores --sem-cor`. Os nós
  continuam identificados pelo rótulo `(V)` (vermelho) e `(P)` (preto).
- Opções do benchmark: `--max-n 100000` (pula o N de 1 milhão), `--rep 10`
  (repetições), `--csv arquivo.csv`.

## Decisões adotadas

| Situação | Decisão |
|---|---|
| **Inserir valor repetido** | Rejeitado: a árvore não muda, `inserir` retorna `false` e o menu avisa o usuário. Cada identificador de encomenda é único. |
| **Remover valor inexistente** | Nada é alterado, `remover` retorna `false` e o menu informa que o valor não foi encontrado. |
| **Buscar valor inexistente** | Retorna `false` sem modificar a árvore. |
| **Remoção de nó com dois filhos** | O nó é substituído pelo **sucessor em ordem** (menor chave da subárvore direita). |
| **Árvore vazia** | Todas as operações são seguras: busca/remoção retornam `false`, percurso em ordem é vazio, altura é 0 e a impressão mostra `(árvore vazia)`. |
| **Tipo da chave** | `int` (qualquer valor, inclusive negativos e `INT_MIN`/`INT_MAX`). |
| **Altura** | Medida em **nós** no maior caminho raiz → folha: árvore vazia = 0, um nó = 1. |
| **Rotações** | Cada rotação simples conta 1; uma rotação dupla conta 2. |
| **Recolorações** | Cada mudança efetiva de cor em um nó conta 1 (nós novos, que nascem vermelhos, não contam). Só existe na Rubro-Negra. |
| **Comparações** | Cada nó visitado em busca/inserção/remoção conta 1 (comparação de três vias). |
| **Rubro-Negra** | Nó sentinela no lugar de `nullptr` e ponteiro para o pai (esquema clássico do Cormen). |
| **AVL** | Recursiva, com altura armazenada em cada nó; rebalanceia ao voltar da recursão, na inserção e na remoção. |

### Visualização

A árvore é desenhada "deitada" (subárvore direita em cima, esquerda embaixo, raiz à esquerda):

```
        /-- 80 (P)
    /-- 70 (V)
    |   |   /-- 65 (V)
    |   \-- 60 (P)
--- 50 (P)
    |   /-- 40 (V)
    \-- 30 (P)
```

Na AVL cada nó mostra o fator de balanceamento: `20 (fb=+1)`.

## Testes (`make test`)

Os **mesmos** testes rodam para as duas estruturas, e depois uma comparação
cruzada aplica a mesma sequência de operações nas duas.

| Exigência do enunciado | Teste |
|---|---|
| Inserção crescente, decrescente e não ordenada | `inserção crescente/decrescente/aleatória/em lotes` (valida a estrutura a cada inserção, N=300, e de novo com N=20.000) |
| Busca de existentes e inexistentes | `busca de existentes e inexistentes` |
| Remoção com diferentes configurações de filhos | `remoção com diferentes configurações de filhos` (folha, 1 filho esquerdo, 1 filho direito, 2 filhos, raiz, sucessor com filho) e `remoção exaustiva` (9 combinações de ordem de inserção x ordem de remoção, validando a cada remoção) |
| Remoção de inexistentes | `remoção de elementos inexistentes` |
| Árvores vazias e com um elemento | `árvore vazia e com um elemento` |
| Propriedades de balanceamento após inserções e remoções | `validar()` é chamado após cada operação nos testes acima |
| (extra) | duplicatas, valores extremos de `int`, 60 mil operações aleatórias comparadas com `std::set`, 100 mil elementos, comparação AVL x Rubro-Negra |

`validar()` confere: ordem de busca; contagem de nós; **AVL**: fator de
balanceamento em todos os nós e alturas armazenadas; **Rubro-Negra**: raiz
preta, nenhum vermelho com filho vermelho, mesma altura negra em todos os
caminhos e consistência dos ponteiros para o pai.

Os testes foram verificados com AddressSanitizer/UBSan (sem erros nem vazamentos)
e com *mutation testing*: ao quebrar de propósito a rotação dupla da AVL, o
recolorimento da inserção ou a correção da remoção da Rubro-Negra, os testes
falham.

## Benchmark (`make bench`)

Para N ∈ {1.000; 10.000; 100.000; 1.000.000} e quatro cenários de entrada
(`crescente`, `decrescente`, `aleatoria`, `lotes` = parcialmente ordenado),
AVL, Rubro-Negra e BST recebem a **mesma** sequência de inserções, buscas
(existentes e inexistentes) e remoções (50% das chaves). A BST fica de fora
nos cenários ordenados com N > 20.000, porque vira uma lista e o teste levaria
minutos.

Como cada coluna alimenta um tópico da análise exigida:

| Tópico da análise | Onde olhar |
|---|---|
| Diferenças entre as estratégias de balanceamento | `rot.*` e `recol.*` (AVL só rotaciona; Rubro-Negra rotaciona e recolore) |
| Altura final após as mesmas operações | `alt.ins` e `alt.rem`, comparadas com a altura mínima teórica impressa no cabeçalho |
| Rotações e ajustes realizados | `rot.ins`, `rot.rem`, `recol.ins`, `recol.rem` |
| Diferenças de comportamento | tempos de inserção, busca e remoção por cenário |
| Vantagens, limitações e aplicações | conjunto dos resultados, incluindo a BST como contraste |

`alturas`, `rotações`, `recolorações` e `comparações` são **determinísticas**
(mesmos valores em qualquer máquina). Os **tempos** variam com o hardware, então
cite a máquina usada e rode com mais repetições (`--rep 10`) para o relatório.

Exemplo (N = 100.000, valores determinísticos):

| Cenário | Estrutura | Altura após inserir | Rotações (ins.) | Recolorações (ins.) |
|---|---|---|---|---|
| crescente | AVL | 17 | 99.983 | – |
| crescente | Rubro-Negra | 31 | 99.969 | 499.846 |
| aleatória | AVL | 20 | 69.589 | – |
| aleatória | Rubro-Negra | 20 | 58.569 | 231.932 |

Isso já mostra o contraste teórico: a AVL é mais rígida e mantém a árvore mais
baixa (17 contra 31 com entrada crescente), enquanto a Rubro-Negra tolera uma
árvore mais alta e compensa fazendo menos rotações em entradas aleatórias.
