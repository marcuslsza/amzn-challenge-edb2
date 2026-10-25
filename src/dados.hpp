// dados.hpp
// Geração determinística dos conjuntos de dados usados nos testes e no
// benchmark. Como tudo é derivado de uma semente fixa, as duas árvores recebem
// exatamente as mesmas sequências e os resultados são reproduzíveis.
#pragma once

#include <algorithm>
#include <numeric>
#include <random>
#include <string>
#include <vector>

// Fisher-Yates com mt19937. Evita std::shuffle de propósito: o algoritmo dele
// varia entre compiladores, o que quebraria a reprodutibilidade entre máquinas.
inline void embaralhar(std::vector<int>& v, unsigned semente) {
    std::mt19937 rng(semente);
    for (std::size_t i = v.size(); i > 1; --i) std::swap(v[i - 1], v[rng() % i]);
}

// Permutação de 1..n em uma das ordens:
//   "crescente"   : 1, 2, 3, ..., n
//   "decrescente" : n, ..., 2, 1
//   "aleatoria"   : permutação pseudoaleatória (semente fixa)
//   "lotes"       : parcialmente ordenada - 20 blocos crescentes, em ordem de
//                   blocos embaralhada (simula lotes de encomendas chegando)
inline std::vector<int> gerarSequencia(int n, const std::string& tipo, unsigned semente = 42) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 1);
    if (tipo == "decrescente") {
        std::reverse(v.begin(), v.end());
    } else if (tipo == "aleatoria") {
        embaralhar(v, semente);
    } else if (tipo == "lotes") {
        const int tam = std::max(1, n / 20);
        const int nBlocos = (n + tam - 1) / tam;
        std::vector<int> ordem(nBlocos);
        std::iota(ordem.begin(), ordem.end(), 0);
        embaralhar(ordem, semente);
        v.clear();
        for (int b : ordem) {
            const int ini = b * tam + 1;
            const int fim = std::min(n, (b + 1) * tam);
            for (int x = ini; x <= fim; ++x) v.push_back(x);
        }
    }
    return v;
}
