// testes.cpp
// Testes automatizados das duas árvores (seção 3 do enunciado). Os MESMOS testes
// rodam para a AVL e para a Rubro-Negra, sempre sobre a interface Arvore.
//
// Cobertura exigida:
//   - inserção crescente, decrescente e não ordenada
//   - busca de elementos existentes e inexistentes
//   - remoção de nós com diferentes configurações de filhos
//   - remoção de elementos inexistentes
//   - árvores vazias e com apenas um elemento
//   - verificação das propriedades de balanceamento após inserções e remoções
//
// Além disso: duplicatas, valores extremos de int, comparação com std::set como
// modelo de referência (teste diferencial) e comparação AVL x Rubro-Negra.
//
// Compilar/rodar: make test   (retorna código != 0 se algum teste falhar)
#include <climits>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "arvore.hpp"
#include "avl.hpp"
#include "dados.hpp"
#include "rubronegra.hpp"

using Fabrica = std::function<std::unique_ptr<Arvore>()>;

static int g_verificacoes = 0;
static int g_falhas = 0;

// msg pode usar operador <<, ex.: VERIFICAR(x == 1, "x=" << x)
#define VERIFICAR(cond, msg)                                                         \
    do {                                                                             \
        ++g_verificacoes;                                                            \
        if (!(cond)) {                                                               \
            ++g_falhas;                                                              \
            std::cerr << "    [FALHA] " << msg << "  (testes.cpp:" << __LINE__ << ")\n"; \
        }                                                                            \
    } while (0)

// ---------------------------------------------------------------- auxiliares

// Confere as propriedades da estrutura (AVL: fator de balanceamento; RN: cores).
static bool estruturaValida(const Arvore& t, const std::string& contexto) {
    std::string erro;
    const bool ok = t.validar(&erro);
    ++g_verificacoes;
    if (!ok) {
        ++g_falhas;
        std::cerr << "    [FALHA] " << t.nome() << " inválida (" << contexto << "): " << erro << "\n";
    }
    return ok;
}

// Altura máxima aceitável para uma árvore balanceada: 2*log2(n+1) vale para a
// Rubro-Negra (teorema clássico) e, a fortiori, para a AVL (~1,44*log2(n)).
static bool alturaLogaritmica(const Arvore& t) {
    const double limite = 2.0 * std::log2(static_cast<double>(t.tamanho()) + 1.0) + 1e-9;
    return t.altura() <= limite;
}

// Compara o conteúdo da árvore com o modelo de referência (std::set).
static bool conferirModelo(const Arvore& t, const std::set<int>& modelo, const std::string& contexto) {
    const std::vector<int> esperado(modelo.begin(), modelo.end());
    const bool tamanhoOk = (t.tamanho() == modelo.size());
    const bool conteudoOk = (t.emOrdem() == esperado);
    g_verificacoes += 2;
    if (!tamanhoOk || !conteudoOk) {
        ++g_falhas;
        std::cerr << "    [FALHA] " << t.nome() << ": conteúdo difere do modelo (" << contexto
                  << "); tamanho=" << t.tamanho() << " esperado=" << modelo.size() << "\n";
        return false;
    }
    return true;
}

static std::unique_ptr<Arvore> montar(const Fabrica& f, const std::vector<int>& chaves) {
    auto t = f();
    for (int k : chaves) t->inserir(k);
    return t;
}

// ---------------------------------------------------------------- testes

// Inserção crescente / decrescente / aleatória / em lotes, validando a cada passo.
static void testeInsercaoOrdens(const Fabrica& f) {
    for (const std::string tipo : {"crescente", "decrescente", "aleatoria", "lotes"}) {
        auto t = f();
        std::set<int> modelo;
        bool seguir = true;
        for (int x : gerarSequencia(300, tipo)) {
            VERIFICAR(t->inserir(x), "inserir(" << x << ") deveria retornar true [" << tipo << "]");
            modelo.insert(x);
            if (!estruturaValida(*t, "após inserir " + std::to_string(x) + ", ordem " + tipo)) {
                seguir = false;
                break;
            }
        }
        if (!seguir) continue;
        conferirModelo(*t, modelo, "inserção " + tipo);
        VERIFICAR(alturaLogaritmica(*t), "altura " << t->altura() << " grande demais [" << tipo << "]");

        // volume maior: valida só no final
        auto grande = f();
        for (int x : gerarSequencia(20000, tipo)) grande->inserir(x);
        estruturaValida(*grande, "20000 inserções " + tipo);
        VERIFICAR(grande->tamanho() == 20000, "tamanho após 20000 inserções [" << tipo << "]");
        VERIFICAR(alturaLogaritmica(*grande), "altura " << grande->altura() << " grande demais com N=20000 [" << tipo << "]");
    }
}

// Busca de existentes e inexistentes.
static void testeBusca(const Fabrica& f) {
    auto t = f();
    for (int x : gerarSequencia(1000, "aleatoria")) t->inserir(x * 2);  // só pares: 2..2000
    for (int x = 2; x <= 2000; x += 2) VERIFICAR(t->buscar(x), "buscar(" << x << ") deveria achar");
    for (int x = 1; x <= 2001; x += 2) VERIFICAR(!t->buscar(x), "buscar(" << x << ") não deveria achar");
    VERIFICAR(!t->buscar(0), "buscar(0)");
    VERIFICAR(!t->buscar(-5), "buscar(-5)");
    VERIFICAR(!t->buscar(INT_MAX), "buscar(INT_MAX)");
    VERIFICAR(!t->buscar(INT_MIN), "buscar(INT_MIN)");
    estruturaValida(*t, "após buscas (buscar não pode alterar a árvore)");
    VERIFICAR(t->tamanho() == 1000, "buscar não pode mudar o tamanho");
}

// Remoção de nós com diferentes configurações de filhos, em árvores de formato conhecido.
static void testeRemocaoConfiguracoes(const Fabrica& f) {
    struct Caso {
        const char* descricao;
        std::vector<int> insercao;
        std::vector<int> remocoes;
    };
    // Inserir 50,30,70,20,40,60,80 gera uma árvore PERFEITA nas duas estruturas:
    //          50
    //        /    \.
    //      30      70
    //     /  \    /  \.
    //    20  40  60  80
    const std::vector<int> base = {50, 30, 70, 20, 40, 60, 80};
    std::vector<int> baseMais65 = base;
    baseMais65.push_back(65);  // 60 passa a ter filho direito (65)

    const std::vector<Caso> casos = {
        {"folha", base, {20}},
        {"folha (outra posição)", base, {80}},
        {"nó com 1 filho direito (remove 20, depois 30)", base, {20, 30}},
        {"nó com 1 filho esquerdo (remove 40, depois 30)", base, {40, 30}},
        {"nó com 2 filhos, não raiz", base, {70}},
        {"raiz com 2 filhos", base, {50}},
        {"2 filhos cujo sucessor tem filho direito (remove raiz)", baseMais65, {50}},
        {"esvaziando a árvore pela raiz", base, {50, 60, 70, 80, 30, 40, 20}},
    };

    for (const Caso& c : casos) {
        auto t = montar(f, c.insercao);
        std::set<int> modelo(c.insercao.begin(), c.insercao.end());
        VERIFICAR(t->altura() == (c.insercao.size() == 7 ? 3 : t->altura()),
                  "árvore-base deveria ser perfeita (altura 3) [" << c.descricao << "]");
        for (int k : c.remocoes) {
            VERIFICAR(t->remover(k), "remover(" << k << ") deveria retornar true [" << c.descricao << "]");
            modelo.erase(k);
            VERIFICAR(!t->buscar(k), "chave " << k << " ainda encontrada após remoção [" << c.descricao << "]");
            if (!estruturaValida(*t, std::string("remoção: ") + c.descricao)) break;
            if (!conferirModelo(*t, modelo, std::string("remoção: ") + c.descricao)) break;
        }
    }
}

// Remove TODAS as chaves, para várias combinações de ordem de inserção x ordem de remoção.
// Isso exercita todos os casos de rebalanceamento da remoção.
static void testeRemocaoExaustiva(const Fabrica& f) {
    const int N = 150;
    for (const std::string ins : {"crescente", "decrescente", "aleatoria"}) {
        for (const std::string rem : {"crescente", "decrescente", "aleatoria"}) {
            auto t = f();
            for (int x : gerarSequencia(N, ins)) t->inserir(x);
            const std::vector<int> ordem = gerarSequencia(N, rem, 99);
            std::set<int> modelo(ordem.begin(), ordem.end());
            const std::string ctx = "inserção " + ins + ", remoção " + rem;
            bool ok = true;
            for (int x : ordem) {
                VERIFICAR(t->remover(x), "remover(" << x << ") [" << ctx << "]");
                modelo.erase(x);
                if (!estruturaValida(*t, ctx) || !conferirModelo(*t, modelo, ctx)) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                VERIFICAR(t->tamanho() == 0, "árvore deveria ficar vazia [" << ctx << "]");
                VERIFICAR(t->altura() == 0, "altura da árvore vazia [" << ctx << "]");
            }
        }
    }
}

// Remoção de elementos inexistentes.
static void testeRemocaoInexistente(const Fabrica& f) {
    auto t = f();
    VERIFICAR(!t->remover(10), "remover em árvore vazia deveria retornar false");

    for (int x = 2; x <= 200; x += 2) t->inserir(x);
    const std::vector<int> antes = t->emOrdem();
    for (int x : {1, 3, 99, 201, 0, -7, INT_MAX, INT_MIN})
        VERIFICAR(!t->remover(x), "remover(" << x << ") inexistente deveria retornar false");
    VERIFICAR(t->emOrdem() == antes, "remoção de inexistentes não pode alterar a árvore");
    VERIFICAR(t->tamanho() == antes.size(), "tamanho após remoções inexistentes");
    estruturaValida(*t, "após remover inexistentes");

    VERIFICAR(t->remover(100), "remover(100) existente");
    VERIFICAR(!t->remover(100), "remover(100) pela segunda vez deveria retornar false");
    estruturaValida(*t, "após remover o mesmo valor duas vezes");
}

// Árvores vazias e com um único elemento.
static void testeVaziaEUmElemento(const Fabrica& f) {
    ArvoreRubroNegra::usarCor = false;
    auto t = f();
    VERIFICAR(t->tamanho() == 0, "tamanho da árvore vazia");
    VERIFICAR(t->altura() == 0, "altura da árvore vazia");
    VERIFICAR(!t->buscar(10), "buscar em árvore vazia");
    VERIFICAR(!t->remover(10), "remover em árvore vazia");
    VERIFICAR(t->emOrdem().empty(), "percurso em ordem da árvore vazia");
    estruturaValida(*t, "árvore vazia");
    {
        std::ostringstream os;
        t->imprimir(os);
        VERIFICAR(!os.str().empty(), "imprimir árvore vazia deve produzir alguma mensagem");
    }

    VERIFICAR(t->inserir(42), "inserir 42 na árvore vazia");
    VERIFICAR(t->tamanho() == 1, "tamanho com 1 elemento");
    VERIFICAR(t->altura() == 1, "altura com 1 elemento");
    VERIFICAR(t->buscar(42), "buscar 42");
    VERIFICAR(!t->buscar(41), "buscar 41");
    VERIFICAR(t->emOrdem() == std::vector<int>{42}, "percurso em ordem com 1 elemento");
    estruturaValida(*t, "1 elemento");
    {
        std::ostringstream os;
        t->imprimir(os);
        VERIFICAR(os.str().find("42") != std::string::npos, "impressão deve conter o 42");
        if (t->nome() == "Rubro-Negra")
            VERIFICAR(os.str().find("(P)") != std::string::npos, "raiz deve aparecer como preta (P)");
    }
    VERIFICAR(!t->inserir(42), "inserir 42 duplicado");
    VERIFICAR(!t->remover(41), "remover inexistente com 1 elemento");
    VERIFICAR(t->tamanho() == 1, "tamanho inalterado");

    VERIFICAR(t->remover(42), "remover o único elemento");
    VERIFICAR(t->tamanho() == 0 && t->altura() == 0, "árvore deve voltar a ficar vazia");
    estruturaValida(*t, "esvaziada");
    VERIFICAR(t->inserir(7), "reinserir após esvaziar");
    estruturaValida(*t, "reinserção após esvaziar");

    t->limpar();
    VERIFICAR(t->tamanho() == 0 && t->altura() == 0, "limpar()");
    VERIFICAR(t->inserir(1) && t->inserir(2) && t->inserir(3), "usar a árvore após limpar()");
    estruturaValida(*t, "após limpar() e reinserir");
}

// Tentativas de inserir valores repetidos.
static void testeDuplicatas(const Fabrica& f) {
    auto t = f();
    for (int x : gerarSequencia(50, "aleatoria")) VERIFICAR(t->inserir(x), "primeira inserção de " << x);
    const std::vector<int> antes = t->emOrdem();
    for (int x : gerarSequencia(50, "aleatoria", 5)) VERIFICAR(!t->inserir(x), "duplicata " << x << " deveria ser rejeitada");
    VERIFICAR(t->tamanho() == 50, "duplicatas não podem aumentar o tamanho");
    VERIFICAR(t->emOrdem() == antes, "duplicatas não podem alterar o conteúdo");
    estruturaValida(*t, "após duplicatas");
}

// Valores extremos e negativos.
static void testeValoresExtremos(const Fabrica& f) {
    auto t = f();
    const std::vector<int> chaves = {0, -1, 1, INT_MIN, INT_MAX, INT_MIN + 1, INT_MAX - 1, -1000, 1000};
    std::set<int> modelo;
    for (int k : chaves) {
        VERIFICAR(t->inserir(k), "inserir " << k);
        modelo.insert(k);
        if (!estruturaValida(*t, "valores extremos")) return;
    }
    conferirModelo(*t, modelo, "valores extremos");
    for (int k : chaves) VERIFICAR(t->buscar(k), "buscar " << k);
    for (int k : chaves) {
        VERIFICAR(t->remover(k), "remover " << k);
        modelo.erase(k);
        if (!estruturaValida(*t, "remoção de extremos") || !conferirModelo(*t, modelo, "remoção de extremos")) return;
    }
}

// Teste diferencial: sequência aleatória de operações comparada com std::set.
static void testeModeloAleatorio(const Fabrica& f) {
    for (unsigned semente : {1u, 2u, 3u}) {
        std::mt19937 rng(semente);
        auto t = f();
        std::set<int> modelo;
        const int antes = g_falhas;
        for (int i = 0; i < 20000; ++i) {
            const int k = static_cast<int>(rng() % 300);
            const int op = static_cast<int>(rng() % 100);
            if (op < 50) {
                const bool esperado = modelo.insert(k).second;
                VERIFICAR(t->inserir(k) == esperado, "inserir(" << k << ") divergiu do modelo, op " << i);
            } else if (op < 85) {
                const bool esperado = modelo.erase(k) > 0;
                VERIFICAR(t->remover(k) == esperado, "remover(" << k << ") divergiu do modelo, op " << i);
            } else {
                const bool esperado = modelo.count(k) > 0;
                VERIFICAR(t->buscar(k) == esperado, "buscar(" << k << ") divergiu do modelo, op " << i);
            }
            if (g_falhas != antes || !estruturaValida(*t, "operação aleatória " + std::to_string(i))) return;
        }
        conferirModelo(*t, modelo, "operações aleatórias, semente " + std::to_string(semente));
    }
}

// Grande volume: 100 mil inserções, remoção de metade, e inserção ordenada.
static void testeGrandeVolume(const Fabrica& f) {
    const int N = 100000;
    {
        auto t = f();
        for (int x : gerarSequencia(N, "aleatoria")) t->inserir(x);
        estruturaValida(*t, "100000 inserções aleatórias");
        VERIFICAR(t->tamanho() == static_cast<std::size_t>(N), "tamanho após 100000 inserções");
        VERIFICAR(alturaLogaritmica(*t), "altura " << t->altura());

        const std::vector<int> ordem = gerarSequencia(N, "aleatoria", 7);
        int removidos = 0;
        for (int i = 0; i < N / 2; ++i) removidos += t->remover(ordem[i]) ? 1 : 0;
        VERIFICAR(removidos == N / 2, "deveria remover " << N / 2 << ", removeu " << removidos);
        estruturaValida(*t, "após remover 50000");
        std::vector<int> restante(ordem.begin() + N / 2, ordem.end());
        std::sort(restante.begin(), restante.end());
        VERIFICAR(t->emOrdem() == restante, "conteúdo após remover metade");
        VERIFICAR(alturaLogaritmica(*t), "altura após remoções: " << t->altura());
    }
    {
        // pior caso de uma BST comum: entrada já ordenada
        auto t = f();
        for (int x = 1; x <= N; ++x) t->inserir(x);
        estruturaValida(*t, "100000 inserções crescentes");
        VERIFICAR(alturaLogaritmica(*t), "altura " << t->altura() << " com entrada crescente");
        for (int x = N; x >= N / 2; --x) t->remover(x);
        estruturaValida(*t, "remoções em ordem decrescente");
    }
}

// Mesma sequência de operações nas DUAS árvores: resultados e conteúdo devem coincidir.
static void testeAVLContraRubroNegra() {
    ArvoreAVL avl;
    ArvoreRubroNegra rn;
    std::mt19937 rng(2024);
    const int antes = g_falhas;
    for (int i = 0; i < 30000; ++i) {
        const int k = static_cast<int>(rng() % 1000);
        const int op = static_cast<int>(rng() % 100);
        if (op < 50) {
            VERIFICAR(avl.inserir(k) == rn.inserir(k), "inserir(" << k << ") divergiu entre AVL e RN");
        } else if (op < 85) {
            VERIFICAR(avl.remover(k) == rn.remover(k), "remover(" << k << ") divergiu entre AVL e RN");
        } else {
            VERIFICAR(avl.buscar(k) == rn.buscar(k), "buscar(" << k << ") divergiu entre AVL e RN");
        }
        if (g_falhas != antes) return;
    }
    VERIFICAR(avl.emOrdem() == rn.emOrdem(), "AVL e Rubro-Negra devem guardar os mesmos elementos");
    VERIFICAR(avl.tamanho() == rn.tamanho(), "tamanhos devem coincidir");
    estruturaValida(avl, "comparação cruzada");
    estruturaValida(rn, "comparação cruzada");
}

// ---------------------------------------------------------------- execução

int main() {
    ArvoreRubroNegra::usarCor = false;

    struct Teste {
        const char* nome;
        void (*fn)(const Fabrica&);
    };
    const std::vector<Teste> testes = {
        {"inserção crescente/decrescente/aleatória/em lotes", testeInsercaoOrdens},
        {"busca de existentes e inexistentes", testeBusca},
        {"remoção com diferentes configurações de filhos", testeRemocaoConfiguracoes},
        {"remoção exaustiva (9 combinações de ordens)", testeRemocaoExaustiva},
        {"remoção de elementos inexistentes", testeRemocaoInexistente},
        {"árvore vazia e com um elemento", testeVaziaEUmElemento},
        {"inserção de valores repetidos", testeDuplicatas},
        {"valores extremos de int", testeValoresExtremos},
        {"operações aleatórias x std::set (modelo)", testeModeloAleatorio},
        {"grande volume (100 mil elementos)", testeGrandeVolume},
    };

    const std::vector<std::pair<std::string, Fabrica>> estruturas = {
        {"AVL", [] { return std::make_unique<ArvoreAVL>(); }},
        {"Rubro-Negra", [] { return std::make_unique<ArvoreRubroNegra>(); }},
    };

    for (const auto& [nomeEstrutura, fabrica] : estruturas) {
        std::cout << "\n=== " << nomeEstrutura << " ===\n";
        for (const Teste& teste : testes) {
            const int falhasAntes = g_falhas;
            teste.fn(fabrica);
            std::cout << (g_falhas == falhasAntes ? "  [OK]     " : "  [FALHOU] ") << teste.nome << "\n";
        }
    }

    std::cout << "\n=== Comparação entre as estruturas ===\n";
    {
        const int falhasAntes = g_falhas;
        testeAVLContraRubroNegra();
        std::cout << (g_falhas == falhasAntes ? "  [OK]     " : "  [FALHOU] ")
                  << "mesma sequência de operações em AVL e Rubro-Negra\n";
    }

    std::cout << "\n" << g_verificacoes << " verificações, " << g_falhas << " falha(s).\n";
    if (g_falhas == 0) std::cout << "TODOS OS TESTES PASSARAM.\n";
    return g_falhas == 0 ? 0 : 1;
}
