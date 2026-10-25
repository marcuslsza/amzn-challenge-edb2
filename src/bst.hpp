// bst.hpp
// Árvore binária de busca SEM balanceamento. Não faz parte do menu do sistema:
// existe apenas como linha de base no benchmark, para mostrar o que acontece
// com uma BST comum quando os dados chegam ordenados (o problema descrito no
// contexto do enunciado).
//
// Todas as operações são iterativas, porque em entrada ordenada a árvore vira
// uma lista encadeada e uma recursão estouraria a pilha.
#pragma once

#include "arvore.hpp"

#include <limits>
#include <string>

class ArvoreBST : public Arvore {
public:
    ArvoreBST() = default;
    ~ArvoreBST() override { limpar(); }
    ArvoreBST(const ArvoreBST&) = delete;
    ArvoreBST& operator=(const ArvoreBST&) = delete;

    std::string nome() const override { return "BST (sem balanceamento)"; }

    bool inserir(int chave) override {
        No** p = &raiz_;
        while (*p) {
            ++st_.comparacoes;
            if (chave == (*p)->chave) return false;
            p = (chave < (*p)->chave) ? &(*p)->esq : &(*p)->dir;
        }
        *p = new No{chave, nullptr, nullptr};
        ++n_;
        return true;
    }

    bool remover(int chave) override {
        No* pai = nullptr;
        No* x = raiz_;
        while (x && x->chave != chave) {
            ++st_.comparacoes;
            pai = x;
            x = (chave < x->chave) ? x->esq : x->dir;
        }
        if (!x) return false;
        ++st_.comparacoes;

        if (x->esq && x->dir) {  // 2 filhos: copia o sucessor e remove o sucessor
            No* paiSuc = x;
            No* suc = x->dir;
            while (suc->esq) {
                paiSuc = suc;
                suc = suc->esq;
            }
            x->chave = suc->chave;
            if (paiSuc->esq == suc)
                paiSuc->esq = suc->dir;
            else
                paiSuc->dir = suc->dir;
            delete suc;
        } else {                 // 0 ou 1 filho
            No* filho = x->esq ? x->esq : x->dir;
            if (!pai)
                raiz_ = filho;
            else if (pai->esq == x)
                pai->esq = filho;
            else
                pai->dir = filho;
            delete x;
        }
        --n_;
        return true;
    }

    bool buscar(int chave) const override {
        const No* x = raiz_;
        while (x) {
            ++st_.comparacoes;
            if (chave == x->chave) return true;
            x = (chave < x->chave) ? x->esq : x->dir;
        }
        return false;
    }

    std::vector<int> emOrdem() const override {
        std::vector<int> v;
        v.reserve(n_);
        std::vector<const No*> pilha;
        const No* x = raiz_;
        while (x || !pilha.empty()) {
            while (x) {
                pilha.push_back(x);
                x = x->esq;
            }
            x = pilha.back();
            pilha.pop_back();
            v.push_back(x->chave);
            x = x->dir;
        }
        return v;
    }

    void imprimir(std::ostream& os) const override {
        os << "(impressão não disponível para a BST de referência)\n";
    }

    // Altura por varredura em níveis (iterativa).
    int altura() const override {
        if (!raiz_) return 0;
        int h = 0;
        std::vector<const No*> nivel{raiz_}, prox;
        while (!nivel.empty()) {
            ++h;
            prox.clear();
            for (const No* n : nivel) {
                if (n->esq) prox.push_back(n->esq);
                if (n->dir) prox.push_back(n->dir);
            }
            nivel.swap(prox);
        }
        return h;
    }

    std::size_t tamanho() const override { return n_; }

    bool validar(std::string* erro = nullptr) const override {
        const std::vector<int> v = emOrdem();
        if (v.size() != n_) {
            if (erro) *erro = "contador de nós difere da quantidade real";
            return false;
        }
        for (std::size_t i = 1; i < v.size(); ++i) {
            if (v[i - 1] >= v[i]) {
                if (erro) *erro = "percurso em ordem não está estritamente crescente";
                return false;
            }
        }
        return true;
    }

    void limpar() override {
        std::vector<No*> pilha;
        if (raiz_) pilha.push_back(raiz_);
        while (!pilha.empty()) {
            No* n = pilha.back();
            pilha.pop_back();
            if (n->esq) pilha.push_back(n->esq);
            if (n->dir) pilha.push_back(n->dir);
            delete n;
        }
        raiz_ = nullptr;
        n_ = 0;
    }

    Estatisticas estatisticas() const override { return st_; }
    void zerarEstatisticas() override { st_ = Estatisticas{}; }

private:
    struct No {
        int chave;
        No* esq;
        No* dir;
    };
    No* raiz_ = nullptr;
    std::size_t n_ = 0;
    mutable Estatisticas st_;
};
