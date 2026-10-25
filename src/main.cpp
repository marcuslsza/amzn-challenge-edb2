// main.cpp
// Sistema interativo: o usuário escolhe AVL ou Rubro-Negra em tempo de execução
// e usa as operações do enunciado (inserir, remover, buscar, percurso em ordem
// e impressão da estrutura).
//
// Uso: ./arvores [--sem-cor]
#include <cctype>
#include <climits>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "arvore.hpp"
#include "avl.hpp"
#include "dados.hpp"
#include "rubronegra.hpp"

namespace {

// ---------- entrada ----------
bool lerLinha(const std::string& prompt, std::string& linha) {
    std::cout << prompt << std::flush;
    return static_cast<bool>(std::getline(std::cin, linha));
}

// Converte texto em int. Retorna false se não for um inteiro válido.
bool paraInt(const std::string& texto, int& valor) {
    try {
        std::size_t pos = 0;
        const long long v = std::stoll(texto, &pos);
        while (pos < texto.size() && std::isspace(static_cast<unsigned char>(texto[pos]))) ++pos;
        if (pos != texto.size() || v < INT_MIN || v > INT_MAX) return false;
        valor = static_cast<int>(v);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// Repete até receber um inteiro válido. Retorna false somente se a entrada acabou (EOF).
bool lerInteiro(const std::string& prompt, int& valor) {
    std::string linha;
    while (lerLinha(prompt, linha)) {
        if (paraInt(linha, valor)) return true;
        std::cout << "  Entrada inválida: digite um número inteiro.\n";
    }
    return false;
}

// ---------- operações do menu ----------
void inserirUm(Arvore& t) {
    int x;
    if (!lerInteiro("  Valor a inserir: ", x)) return;
    if (t.inserir(x))
        std::cout << "  " << x << " inserido.\n";
    else
        std::cout << "  " << x << " já existe na árvore; inserção ignorada (duplicatas não são permitidas).\n";
}

void inserirVarios(Arvore& t) {
    std::string linha;
    if (!lerLinha("  Valores separados por espaço: ", linha)) return;
    std::istringstream is(linha);
    std::string tok;
    int inseridos = 0;
    std::string duplicados, invalidos;
    while (is >> tok) {
        int x;
        if (!paraInt(tok, x))
            invalidos += " " + tok;
        else if (t.inserir(x))
            ++inseridos;
        else
            duplicados += " " + tok;
    }
    std::cout << "  Inseridos: " << inseridos << "\n";
    if (!duplicados.empty()) std::cout << "  Ignorados (já existiam):" << duplicados << "\n";
    if (!invalidos.empty()) std::cout << "  Ignorados (não são inteiros):" << invalidos << "\n";
}

void removerUm(Arvore& t) {
    int x;
    if (!lerInteiro("  Valor a remover: ", x)) return;
    if (t.remover(x))
        std::cout << "  " << x << " removido.\n";
    else
        std::cout << "  " << x << " não encontrado; nada foi removido.\n";
}

void buscarUm(const Arvore& t) {
    int x;
    if (!lerInteiro("  Valor a buscar: ", x)) return;
    std::cout << "  " << x << (t.buscar(x) ? " ENCONTRADO.\n" : " NÃO encontrado.\n");
}

void percursoEmOrdem(const Arvore& t) {
    const std::vector<int> v = t.emOrdem();
    if (v.empty()) {
        std::cout << "  (árvore vazia)\n";
        return;
    }
    std::cout << "  Em ordem (" << v.size() << " elementos):";
    for (int x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

void imprimirArvore(const Arvore& t) {
    if (t.tamanho() > 100) {
        std::string r;
        if (!lerLinha("  A árvore tem " + std::to_string(t.tamanho()) +
                          " nós e o desenho será grande. Imprimir mesmo assim? (s/N): ",
                      r))
            return;
        if (r.empty() || (r[0] != 's' && r[0] != 'S')) return;
    }
    std::cout << "\n";
    t.imprimir(std::cout);
    std::cout << "\n";
}

void informacoes(const Arvore& t) {
    const std::size_t n = t.tamanho();
    const int minima = n == 0 ? 0 : static_cast<int>(std::ceil(std::log2(static_cast<double>(n) + 1.0)));
    const Estatisticas st = t.estatisticas();
    std::cout << "  Estrutura:           " << t.nome() << "\n"
              << "  Elementos:           " << n << "\n"
              << "  Altura (em nós):     " << t.altura() << "  (mínima possível: " << minima << ")\n"
              << "  Rotações realizadas: " << st.rotacoes << "\n";
    if (dynamic_cast<const ArvoreRubroNegra*>(&t))
        std::cout << "  Recolorações:        " << st.recoloracoes << "\n";
    std::cout << "  Comparações:         " << st.comparacoes << "\n";
}

void validarEstrutura(const Arvore& t) {
    std::string erro;
    if (t.validar(&erro))
        std::cout << "  OK: todas as propriedades da " << t.nome() << " estão preservadas.\n";
    else
        std::cout << "  VIOLAÇÃO: " << erro << "\n";
}

void carregarSequencia(Arvore& t) {
    std::cout << "  Tipo de sequência:\n"
                 "    1) crescente   2) decrescente   3) aleatória   4) em lotes\n";
    int tipo, n;
    if (!lerInteiro("  Escolha: ", tipo)) return;
    if (tipo < 1 || tipo > 4) {
        std::cout << "  Opção inválida.\n";
        return;
    }
    if (!lerInteiro("  Quantidade de valores (1 a 1000000): ", n)) return;
    if (n < 1 || n > 1000000) {
        std::cout << "  Quantidade fora do intervalo.\n";
        return;
    }
    static const char* nomes[] = {"crescente", "decrescente", "aleatoria", "lotes"};
    int inseridos = 0;
    for (int x : gerarSequencia(n, nomes[tipo - 1])) inseridos += t.inserir(x) ? 1 : 0;
    std::cout << "  " << inseridos << " valores inseridos (" << (n - inseridos) << " já existiam). "
              << "Altura = " << t.altura() << ".\n";
}

std::unique_ptr<Arvore> escolherEstrutura() {
    while (true) {
        std::cout << "\nEscolha a estrutura de dados:\n"
                     "  1) Árvore AVL\n"
                     "  2) Árvore Rubro-Negra\n"
                     "  0) Sair\n";
        int op;
        if (!lerInteiro("Opção: ", op)) return nullptr;
        if (op == 1) return std::make_unique<ArvoreAVL>();
        if (op == 2) return std::make_unique<ArvoreRubroNegra>();
        if (op == 0) return nullptr;
        std::cout << "Opção inválida.\n";
    }
}

void mostrarMenu(const Arvore& t) {
    std::cout << "\n=== " << t.nome() << " (" << t.tamanho() << " elementos) ===\n"
                 "  1) Inserir um valor\n"
                 "  2) Inserir vários valores\n"
                 "  3) Remover\n"
                 "  4) Buscar\n"
                 "  5) Percurso em ordem\n"
                 "  6) Imprimir a árvore\n"
                 "  7) Informações (altura, rotações, ...)\n"
                 "  8) Validar propriedades da estrutura\n"
                 "  9) Carregar sequência de teste (crescente/decrescente/aleatória/lotes)\n"
                 " 10) Limpar a árvore\n"
                 " 11) Trocar de estrutura\n"
                 "  0) Sair\n";
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--sem-cor") ArvoreRubroNegra::usarCor = false;
    }

    std::cout << "=== Amazon Engineering Challenge: Árvores Balanceadas ===\n";
    std::unique_ptr<Arvore> arvore = escolherEstrutura();

    while (arvore) {
        mostrarMenu(*arvore);
        int op;
        if (!lerInteiro("Opção: ", op)) break;
        switch (op) {
            case 1: inserirUm(*arvore); break;
            case 2: inserirVarios(*arvore); break;
            case 3: removerUm(*arvore); break;
            case 4: buscarUm(*arvore); break;
            case 5: percursoEmOrdem(*arvore); break;
            case 6: imprimirArvore(*arvore); break;
            case 7: informacoes(*arvore); break;
            case 8: validarEstrutura(*arvore); break;
            case 9: carregarSequencia(*arvore); break;
            case 10:
                arvore->limpar();
                arvore->zerarEstatisticas();
                std::cout << "  Árvore esvaziada.\n";
                break;
            case 11:
                arvore = escolherEstrutura();  // a árvore atual é descartada
                break;
            case 0: arvore.reset(); break;
            default: std::cout << "  Opção inválida.\n";
        }
    }
    std::cout << "Até logo!\n";
    return 0;
}
