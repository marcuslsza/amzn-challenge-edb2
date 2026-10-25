// avl.hpp
// Árvore AVL implementada do zero (sem bibliotecas de árvore balanceada).
//
// Invariante: para todo nó, |altura(esq) - altura(dir)| <= 1.
// Rebalanceamento por rotações simples (LL, RR) e duplas (LR, RL), aplicado
// no retorno da recursão tanto na inserção quanto na remoção.
#pragma once

#include "arvore.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string>

class ArvoreAVL : public Arvore {
public:
    ArvoreAVL() = default;
    ~ArvoreAVL() override { destruir(raiz_); }
    ArvoreAVL(const ArvoreAVL&) = delete;
    ArvoreAVL& operator=(const ArvoreAVL&) = delete;

    std::string nome() const override { return "AVL"; }

    bool inserir(int chave) override {
        bool inseriu = false;
        raiz_ = inserirRec(raiz_, chave, inseriu);
        if (inseriu) ++n_;
        return inseriu;
    }

    bool remover(int chave) override {
        bool removeu = false;
        raiz_ = removerRec(raiz_, chave, removeu);
        if (removeu) --n_;
        return removeu;
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
        coletar(raiz_, v);
        return v;
    }

    void imprimir(std::ostream& os) const override {
        if (!raiz_) {
            os << "(árvore vazia)\n";
            return;
        }
        if (raiz_->dir) desenhar(os, raiz_->dir, "    ", true);
        os << "--- " << rotulo(raiz_) << "\n";
        if (raiz_->esq) desenhar(os, raiz_->esq, "    ", false);
    }

    int altura() const override { return alt(raiz_); }
    std::size_t tamanho() const override { return n_; }

    bool validar(std::string* erro = nullptr) const override {
        std::string msg;
        std::size_t contados = 0;
        if (verificar(raiz_, std::numeric_limits<long long>::min(),
                      std::numeric_limits<long long>::max(), msg, contados) < 0) {
            if (erro) *erro = msg;
            return false;
        }
        if (contados != n_) {
            if (erro) {
                *erro = "contador de nós (" + std::to_string(n_) +
                        ") difere da quantidade real (" + std::to_string(contados) + ")";
            }
            return false;
        }
        return true;
    }

    void limpar() override {
        destruir(raiz_);
        raiz_ = nullptr;
        n_ = 0;
    }

    Estatisticas estatisticas() const override { return st_; }
    void zerarEstatisticas() override { st_ = Estatisticas{}; }

private:
    struct No {
        int chave;
        int altura;  // altura da subárvore com raiz neste nó (folha = 1)
        No* esq;
        No* dir;
        explicit No(int k) : chave(k), altura(1), esq(nullptr), dir(nullptr) {}
    };

    No* raiz_ = nullptr;
    std::size_t n_ = 0;
    mutable Estatisticas st_;

    // ---------- utilidades ----------
    static int alt(const No* n) { return n ? n->altura : 0; }
    static int fatorBalanceamento(const No* n) { return n ? alt(n->esq) - alt(n->dir) : 0; }
    static void atualizarAltura(No* n) { n->altura = 1 + std::max(alt(n->esq), alt(n->dir)); }

    static void destruir(No* n) {
        if (!n) return;
        destruir(n->esq);
        destruir(n->dir);
        delete n;
    }

    static void coletar(const No* n, std::vector<int>& v) {
        if (!n) return;
        coletar(n->esq, v);
        v.push_back(n->chave);
        coletar(n->dir, v);
    }

    // ---------- rotações ----------
    //        y                x
    //       / \              / \.
    //      x   C    ==>     A   y
    //     / \                  / \.
    //    A   B                B   C
    No* rotacaoDireita(No* y) {
        No* x = y->esq;
        y->esq = x->dir;
        x->dir = y;
        atualizarAltura(y);
        atualizarAltura(x);
        ++st_.rotacoes;
        return x;
    }

    No* rotacaoEsquerda(No* x) {
        No* y = x->dir;
        x->dir = y->esq;
        y->esq = x;
        atualizarAltura(x);
        atualizarAltura(y);
        ++st_.rotacoes;
        return y;
    }

    // Restaura o invariante AVL no nó n (cujos filhos já são AVL).
    No* balancear(No* n) {
        atualizarAltura(n);
        const int fb = fatorBalanceamento(n);
        if (fb > 1) {                                   // pesado à esquerda
            if (fatorBalanceamento(n->esq) < 0)         // caso Esquerda-Direita
                n->esq = rotacaoEsquerda(n->esq);
            return rotacaoDireita(n);                   // caso Esquerda-Esquerda
        }
        if (fb < -1) {                                  // pesado à direita
            if (fatorBalanceamento(n->dir) > 0)         // caso Direita-Esquerda
                n->dir = rotacaoDireita(n->dir);
            return rotacaoEsquerda(n);                  // caso Direita-Direita
        }
        return n;
    }

    // ---------- inserção / remoção ----------
    No* inserirRec(No* n, int k, bool& inseriu) {
        if (!n) {
            inseriu = true;
            return new No(k);
        }
        ++st_.comparacoes;
        if (k < n->chave) {
            n->esq = inserirRec(n->esq, k, inseriu);
        } else if (k > n->chave) {
            n->dir = inserirRec(n->dir, k, inseriu);
        } else {
            inseriu = false;  // duplicata: nada muda
            return n;
        }
        return inseriu ? balancear(n) : n;
    }

    No* removerRec(No* n, int k, bool& removeu) {
        if (!n) {
            removeu = false;
            return nullptr;
        }
        ++st_.comparacoes;
        if (k < n->chave) {
            n->esq = removerRec(n->esq, k, removeu);
        } else if (k > n->chave) {
            n->dir = removerRec(n->dir, k, removeu);
        } else {
            removeu = true;
            if (!n->esq || !n->dir) {  // 0 ou 1 filho: o filho (ou null) ocupa o lugar
                No* filho = n->esq ? n->esq : n->dir;
                delete n;
                return filho;
            }
            // 2 filhos: substitui pela chave do sucessor em ordem (menor da direita)
            No* suc = n->dir;
            while (suc->esq) suc = suc->esq;
            n->chave = suc->chave;
            bool ignorado = false;
            n->dir = removerRec(n->dir, suc->chave, ignorado);
        }
        return removeu ? balancear(n) : n;
    }

    // ---------- verificação (usada por validar) ----------
    // Retorna a altura real da subárvore, ou -1 se achou alguma violação.
    int verificar(const No* n, long long lo, long long hi, std::string& erro,
                  std::size_t& contados) const {
        if (!n) return 0;
        ++contados;
        if (n->chave <= lo || n->chave >= hi) {
            erro = "violação da ordem de busca no nó " + std::to_string(n->chave);
            return -1;
        }
        const int he = verificar(n->esq, lo, n->chave, erro, contados);
        if (he < 0) return -1;
        const int hd = verificar(n->dir, n->chave, hi, erro, contados);
        if (hd < 0) return -1;
        if (std::abs(he - hd) > 1) {
            erro = "nó " + std::to_string(n->chave) + " desbalanceado (altura esq=" +
                   std::to_string(he) + ", dir=" + std::to_string(hd) + ")";
            return -1;
        }
        const int real = 1 + std::max(he, hd);
        if (n->altura != real) {
            erro = "altura armazenada incorreta no nó " + std::to_string(n->chave);
            return -1;
        }
        return real;
    }

    // ---------- impressão ----------
    static std::string rotulo(const No* n) {
        const int fb = fatorBalanceamento(n);
        return std::to_string(n->chave) + " (fb=" + (fb > 0 ? "+" : "") + std::to_string(fb) + ")";
    }

    void desenhar(std::ostream& os, const No* n, const std::string& pref, bool ehDireita) const {
        if (n->dir) desenhar(os, n->dir, pref + (ehDireita ? "    " : "|   "), true);
        os << pref << (ehDireita ? "/-- " : "\\-- ") << rotulo(n) << "\n";
        if (n->esq) desenhar(os, n->esq, pref + (ehDireita ? "|   " : "    "), false);
    }
};
