// arvore.hpp
// Interface comum a todas as árvores do projeto. Permite escolher a estrutura
// em tempo de execução (AVL ou Rubro-Negra) e rodar exatamente os mesmos
// testes/benchmarks nas duas.
#pragma once

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// Contadores usados na análise comparativa (seção 3 do enunciado).
struct Estatisticas {
    unsigned long long rotacoes = 0;      // cada rotação simples conta 1 (dupla = 2)
    unsigned long long recoloracoes = 0;  // trocas de cor (apenas Rubro-Negra)
    unsigned long long comparacoes = 0;   // nós visitados durante busca/inserção/remoção
};

class Arvore {
public:
    virtual ~Arvore() = default;

    virtual std::string nome() const = 0;

    // Retorna false se a chave já existe (duplicatas são rejeitadas).
    virtual bool inserir(int chave) = 0;
    // Retorna false se a chave não existe (árvore permanece inalterada).
    virtual bool remover(int chave) = 0;
    virtual bool buscar(int chave) const = 0;

    // Elementos em ordem crescente.
    virtual std::vector<int> emOrdem() const = 0;
    // Desenha a árvore "deitada" (subárvore direita em cima).
    virtual void imprimir(std::ostream& os) const = 0;

    // Altura em NÓS do maior caminho raiz -> folha. Árvore vazia = 0.
    virtual int altura() const = 0;
    virtual std::size_t tamanho() const = 0;

    // Confere TODAS as propriedades da estrutura (ordem BST, balanceamento /
    // regras da Rubro-Negra, ponteiros, contagem de nós). Custa O(n).
    virtual bool validar(std::string* erro = nullptr) const = 0;

    virtual void limpar() = 0;

    virtual Estatisticas estatisticas() const = 0;
    virtual void zerarEstatisticas() = 0;
};
