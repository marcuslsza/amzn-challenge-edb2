// benchmark.cpp
// Comparação experimental AVL x Rubro-Negra (seção 3: "A análise deverá discutir").
//
// Para cada tamanho N e cada cenário de entrada, as TRÊS estruturas recebem
// exatamente a mesma sequência de operações:
//   1. inserção de N chaves (na ordem do cenário)
//   2. busca de chaves existentes e inexistentes
//   3. remoção de 50% das chaves, em ordem aleatória
//
// Métricas: altura final, rotações, recolorações, comparações e tempo (média de
// várias repetições). A BST sem balanceamento entra como linha de base.
//
// As chaves guardadas são 2*v (números pares); as buscas "inexistentes" usam
// 2*v+1, que caem em posições variadas da árvore (e não só nas pontas).
//
// Uso: ./benchmark [--max-n N] [--rep R] [--csv arquivo.csv]
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "arvore.hpp"
#include "avl.hpp"
#include "bst.hpp"
#include "dados.hpp"
#include "rubronegra.hpp"

using Fabrica = std::function<std::unique_ptr<Arvore>()>;
using Relogio = std::chrono::steady_clock;

namespace {

struct Sequencias {
    std::vector<int> insercao;       // chaves na ordem do cenário
    std::vector<int> buscasExistem;  // chaves presentes
    std::vector<int> buscasFalham;   // chaves ausentes
    std::vector<int> remocoes;       // 50% das chaves, em ordem aleatória
};

struct Resultado {
    std::string cenario, estrutura;
    int n = 0;
    double tIns = 0, tBusca = 0, tRem = 0;  // ms, média das repetições
    int altIns = 0, altRem = 0;
    Estatisticas sIns, sBusca, sRem;
};

double ms(Relogio::time_point a, Relogio::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

Sequencias preparar(int n, const std::string& cenario) {
    Sequencias s;
    for (int v : gerarSequencia(n, cenario)) s.insercao.push_back(2 * v);

    const int m = std::min(n, 200000);
    const std::vector<int> perm = gerarSequencia(n, "aleatoria", 7);
    for (int i = 0; i < m; ++i) {
        s.buscasExistem.push_back(2 * perm[i]);
        s.buscasFalham.push_back(2 * perm[i] + 1);
    }
    const std::vector<int> permRem = gerarSequencia(n, "aleatoria", 11);
    for (int i = 0; i < n / 2; ++i) s.remocoes.push_back(2 * permRem[i]);
    return s;
}

[[noreturn]] void abortar(const std::string& msg) {
    std::cerr << "ERRO NO BENCHMARK: " << msg << "\n";
    std::exit(1);
}

volatile std::size_t g_sorvedouro = 0;  // impede o compilador de descartar as buscas

Resultado medir(const Fabrica& fab, const std::string& cenario, int n, const Sequencias& seq, int reps) {
    Resultado r;
    r.cenario = cenario;
    r.n = n;
    double somaIns = 0, somaBusca = 0, somaRem = 0;

    for (int rep = 0; rep < reps; ++rep) {
        auto t = fab();
        r.estrutura = t->nome();

        // 1) inserção
        t->zerarEstatisticas();
        auto t0 = Relogio::now();
        for (int k : seq.insercao) t->inserir(k);
        auto t1 = Relogio::now();
        r.sIns = t->estatisticas();
        r.altIns = t->altura();
        if (t->tamanho() != static_cast<std::size_t>(n)) abortar(r.estrutura + ": tamanho errado após inserção");

        // 2) busca
        t->zerarEstatisticas();
        std::size_t achados = 0, falsos = 0;
        auto t2 = Relogio::now();
        for (int k : seq.buscasExistem) achados += t->buscar(k) ? 1 : 0;
        for (int k : seq.buscasFalham) falsos += t->buscar(k) ? 1 : 0;
        auto t3 = Relogio::now();
        g_sorvedouro = achados + falsos;
        r.sBusca = t->estatisticas();
        if (achados != seq.buscasExistem.size() || falsos != 0) abortar(r.estrutura + ": busca retornou resultado errado");

        // 3) remoção
        t->zerarEstatisticas();
        auto t4 = Relogio::now();
        for (int k : seq.remocoes) t->remover(k);
        auto t5 = Relogio::now();
        r.sRem = t->estatisticas();
        r.altRem = t->altura();
        if (t->tamanho() != static_cast<std::size_t>(n - n / 2)) abortar(r.estrutura + ": tamanho errado após remoção");

        // integridade (fora da medição, só na última repetição)
        if (rep == reps - 1) {
            std::string erro;
            if (!t->validar(&erro)) abortar(r.estrutura + " inválida após remoções: " + erro);
        }

        somaIns += ms(t0, t1);
        somaBusca += ms(t2, t3);
        somaRem += ms(t4, t5);
    }
    r.tIns = somaIns / reps;
    r.tBusca = somaBusca / reps;
    r.tRem = somaRem / reps;
    return r;
}

void escreverCsv(const std::string& caminho, const std::vector<Resultado>& rs) {
    std::ofstream f(caminho);
    f << "n,cenario,estrutura,t_insercao_ms,t_busca_ms,t_remocao_ms,altura_apos_insercao,altura_apos_remocao,"
         "rotacoes_insercao,rotacoes_remocao,recoloracoes_insercao,recoloracoes_remocao,"
         "comparacoes_insercao,comparacoes_busca,comparacoes_remocao\n";
    for (const Resultado& r : rs) {
        f << r.n << ',' << r.cenario << ",\"" << r.estrutura << "\"," << r.tIns << ',' << r.tBusca << ',' << r.tRem
          << ',' << r.altIns << ',' << r.altRem << ',' << r.sIns.rotacoes << ',' << r.sRem.rotacoes << ','
          << r.sIns.recoloracoes << ',' << r.sRem.recoloracoes << ',' << r.sIns.comparacoes << ','
          << r.sBusca.comparacoes << ',' << r.sRem.comparacoes << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    int maxN = 1000000, reps = 5;
    std::string csv = "resultados_benchmark.csv";
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--max-n" && i + 1 < argc) maxN = std::atoi(argv[++i]);
        else if (a == "--rep" && i + 1 < argc) reps = std::max(1, std::atoi(argv[++i]));
        else if (a == "--csv" && i + 1 < argc) csv = argv[++i];
        else {
            std::cerr << "Uso: " << argv[0] << " [--max-n N] [--rep R] [--csv arquivo.csv]\n";
            return 1;
        }
    }

    const std::vector<int> tamanhos = {1000, 10000, 100000, 1000000};
    const std::vector<std::string> cenarios = {"crescente", "decrescente", "aleatoria", "lotes"};
    const std::vector<std::pair<std::string, Fabrica>> estruturas = {
        {"AVL", [] { return std::make_unique<ArvoreAVL>(); }},
        {"Rubro-Negra", [] { return std::make_unique<ArvoreRubroNegra>(); }},
        {"BST", [] { return std::make_unique<ArvoreBST>(); }},
    };
    const int limiteBstOrdenada = 20000;  // BST com entrada ordenada é O(n^2): só até este N

    std::vector<Resultado> todos;
    std::cout << "Repetições por medição: " << reps << " (tempos = média)\n";
    std::cout << "Buscas: até 200000 chaves existentes + 200000 inexistentes. Remoção: 50% das chaves.\n";

    for (int n : tamanhos) {
        if (n > maxN) continue;
        const int minima = static_cast<int>(std::ceil(std::log2(n + 1.0)));
        std::printf("\n==== N = %d   (altura mínima teórica = %d nós) ====\n", n, minima);
        std::printf("%-12s %-12s | %7s %7s | %9s %9s | %9s %9s | %9s %9s %9s\n", "cenário", "estrutura",
                    "alt.ins", "alt.rem", "rot.ins", "rot.rem", "recol.ins", "recol.rem", "ins(ms)", "busca(ms)",
                    "rem(ms)");
        for (const std::string& cen : cenarios) {
            const Sequencias seq = preparar(n, cen);
            for (const auto& [nome, fab] : estruturas) {
                if (nome == "BST" && cen != "aleatoria" && n > limiteBstOrdenada) {
                    std::printf("%-12s %-12s | (omitida: O(n^2) com entrada ordenada)\n", cen.c_str(), nome.c_str());
                    continue;
                }
                std::fflush(stdout);
                const Resultado r = medir(fab, cen, n, seq, reps);
                todos.push_back(r);
                std::printf("%-12s %-12s | %7d %7d | %9llu %9llu | %9llu %9llu | %9.2f %9.2f %9.2f\n", cen.c_str(),
                            nome.c_str(), r.altIns, r.altRem, r.sIns.rotacoes, r.sRem.rotacoes,
                            r.sIns.recoloracoes, r.sRem.recoloracoes, r.tIns, r.tBusca, r.tRem);
            }
        }
    }

    escreverCsv(csv, todos);
    std::cout << "\nResultados completos (incluindo comparações) salvos em " << csv << "\n";
    return 0;
}
