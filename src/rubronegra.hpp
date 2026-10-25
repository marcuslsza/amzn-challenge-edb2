// rubronegra.hpp
// Árvore Rubro-Negra implementada do zero (sem bibliotecas de árvore balanceada).
//
// Propriedades (todas verificadas por validar()):
//   1. Todo nó é vermelho ou preto.
//   2. A raiz é preta.
//   3. Os "nós nulos" (folhas) são pretos.
//   4. Nenhum nó vermelho tem filho vermelho.
//   5. Todo caminho de um nó até suas folhas nulas tem o mesmo número de nós pretos.
//
// Usa um nó sentinela (nil_) no lugar de nullptr, e ponteiro para o pai, seguindo
// o esquema clássico (Cormen et al., "Algoritmos") para inserção e remoção.
#pragma once

#include "arvore.hpp"

#include <algorithm>
#include <limits>
#include <string>

class ArvoreRubroNegra : public Arvore {
public:
    // Se true, nós vermelhos são impressos em vermelho (códigos ANSI), além do
    // rótulo (V)/(P). Desative com --sem-cor em terminais que não suportam ANSI.
    static inline bool usarCor = true;

    ArvoreRubroNegra() {
        nil_ = new No{0, PRETO, nullptr, nullptr, nullptr};
        nil_->esq = nil_->dir = nil_->pai = nil_;
        raiz_ = nil_;
    }
    ~ArvoreRubroNegra() override {
        destruir(raiz_);
        delete nil_;
    }
    ArvoreRubroNegra(const ArvoreRubroNegra&) = delete;
    ArvoreRubroNegra& operator=(const ArvoreRubroNegra&) = delete;

    std::string nome() const override { return "Rubro-Negra"; }

    bool inserir(int chave) override {
        No* pai = nil_;
        No* x = raiz_;
        while (x != nil_) {
            pai = x;
            ++st_.comparacoes;
            if (chave == x->chave) return false;  // duplicata: nada muda
            x = (chave < x->chave) ? x->esq : x->dir;
        }
        No* z = new No{chave, VERMELHO, nil_, nil_, pai};  // todo nó novo nasce vermelho
        if (pai == nil_)
            raiz_ = z;
        else if (chave < pai->chave)
            pai->esq = z;
        else
            pai->dir = z;
        ++n_;
        corrigirInsercao(z);
        return true;
    }

    bool remover(int chave) override {
        No* z = localizar(chave);
        if (z == nil_) return false;

        No* y = z;                 // nó que sai (ou se move) da posição original
        Cor corOriginalY = y->cor;
        No* x;                     // nó que ocupa a posição "vaga"
        if (z->esq == nil_) {
            x = z->dir;
            transplantar(z, z->dir);
        } else if (z->dir == nil_) {
            x = z->esq;
            transplantar(z, z->esq);
        } else {
            y = minimo(z->dir);    // sucessor em ordem
            corOriginalY = y->cor;
            x = y->dir;
            if (y->pai == z) {
                x->pai = y;
            } else {
                transplantar(y, y->dir);
                y->dir = z->dir;
                y->dir->pai = y;
            }
            transplantar(z, y);
            y->esq = z->esq;
            y->esq->pai = y;
            pintar(y, z->cor);     // y herda a cor de z
        }
        delete z;
        --n_;
        if (corOriginalY == PRETO) corrigirRemocao(x);
        return true;
    }

    bool buscar(int chave) const override {
        const No* x = raiz_;
        while (x != nil_) {
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
        if (raiz_ == nil_) {
            os << "(árvore vazia)\n";
            return;
        }
        if (raiz_->dir != nil_) desenhar(os, raiz_->dir, "    ", true);
        os << "--- " << rotulo(raiz_) << "\n";
        if (raiz_->esq != nil_) desenhar(os, raiz_->esq, "    ", false);
    }

    int altura() const override { return alt(raiz_); }
    std::size_t tamanho() const override { return n_; }

    bool validar(std::string* erro = nullptr) const override {
        auto falha = [&](const std::string& m) {
            if (erro) *erro = m;
            return false;
        };
        if (nil_->cor != PRETO) return falha("o nó sentinela (folhas nulas) não é preto");
        if (raiz_ != nil_) {
            if (raiz_->cor != PRETO) return falha("a raiz é vermelha");
            if (raiz_->pai != nil_) return falha("o pai da raiz não é nulo");
        }
        std::string msg;
        std::size_t contados = 0;
        if (verificar(raiz_, std::numeric_limits<long long>::min(),
                      std::numeric_limits<long long>::max(), msg, contados) < 0)
            return falha(msg);
        if (contados != n_)
            return falha("contador de nós (" + std::to_string(n_) +
                         ") difere da quantidade real (" + std::to_string(contados) + ")");
        return true;
    }

    void limpar() override {
        destruir(raiz_);
        raiz_ = nil_;
        nil_->pai = nil_;
        n_ = 0;
    }

    Estatisticas estatisticas() const override { return st_; }
    void zerarEstatisticas() override { st_ = Estatisticas{}; }

private:
    enum Cor { VERMELHO, PRETO };

    struct No {
        int chave;
        Cor cor;
        No* esq;
        No* dir;
        No* pai;
    };

    No* nil_;
    No* raiz_;
    std::size_t n_ = 0;
    mutable Estatisticas st_;

    // ---------- utilidades ----------
    int alt(const No* n) const { return n == nil_ ? 0 : 1 + std::max(alt(n->esq), alt(n->dir)); }

    void destruir(No* n) {
        if (n == nil_) return;
        destruir(n->esq);
        destruir(n->dir);
        delete n;
    }

    void coletar(const No* n, std::vector<int>& v) const {
        if (n == nil_) return;
        coletar(n->esq, v);
        v.push_back(n->chave);
        coletar(n->dir, v);
    }

    No* minimo(No* x) const {
        while (x->esq != nil_) x = x->esq;
        return x;
    }

    No* localizar(int chave) const {
        No* x = raiz_;
        while (x != nil_) {
            ++st_.comparacoes;
            if (chave == x->chave) return x;
            x = (chave < x->chave) ? x->esq : x->dir;
        }
        return nil_;
    }

    // Troca de cor contabilizada (só conta se a cor realmente mudar).
    void pintar(No* n, Cor c) {
        if (n->cor != c) {
            n->cor = c;
            ++st_.recoloracoes;
        }
    }

    // Substitui a subárvore com raiz u pela subárvore com raiz v.
    void transplantar(No* u, No* v) {
        if (u->pai == nil_)
            raiz_ = v;
        else if (u == u->pai->esq)
            u->pai->esq = v;
        else
            u->pai->dir = v;
        v->pai = u->pai;  // vale mesmo quando v == nil_ (a correção da remoção usa isso)
    }

    // ---------- rotações ----------
    void rotacaoEsquerda(No* x) {
        No* y = x->dir;
        x->dir = y->esq;
        if (y->esq != nil_) y->esq->pai = x;
        y->pai = x->pai;
        if (x->pai == nil_)
            raiz_ = y;
        else if (x == x->pai->esq)
            x->pai->esq = y;
        else
            x->pai->dir = y;
        y->esq = x;
        x->pai = y;
        ++st_.rotacoes;
    }

    void rotacaoDireita(No* x) {
        No* y = x->esq;
        x->esq = y->dir;
        if (y->dir != nil_) y->dir->pai = x;
        y->pai = x->pai;
        if (x->pai == nil_)
            raiz_ = y;
        else if (x == x->pai->dir)
            x->pai->dir = y;
        else
            x->pai->esq = y;
        y->dir = x;
        x->pai = y;
        ++st_.rotacoes;
    }

    // ---------- correção após inserção ----------
    // Só há problema se o pai de z também é vermelho (viola a propriedade 4).
    void corrigirInsercao(No* z) {
        while (z->pai->cor == VERMELHO) {
            No* avo = z->pai->pai;
            if (z->pai == avo->esq) {
                No* tio = avo->dir;
                if (tio->cor == VERMELHO) {            // caso 1: tio vermelho -> recolore
                    pintar(z->pai, PRETO);
                    pintar(tio, PRETO);
                    pintar(avo, VERMELHO);
                    z = avo;
                } else {
                    if (z == z->pai->dir) {            // caso 2: "joelho" -> vira caso 3
                        z = z->pai;
                        rotacaoEsquerda(z);
                    }
                    pintar(z->pai, PRETO);             // caso 3: linha reta -> rotação
                    pintar(z->pai->pai, VERMELHO);
                    rotacaoDireita(z->pai->pai);
                }
            } else {                                   // espelho
                No* tio = avo->esq;
                if (tio->cor == VERMELHO) {
                    pintar(z->pai, PRETO);
                    pintar(tio, PRETO);
                    pintar(avo, VERMELHO);
                    z = avo;
                } else {
                    if (z == z->pai->esq) {
                        z = z->pai;
                        rotacaoDireita(z);
                    }
                    pintar(z->pai, PRETO);
                    pintar(z->pai->pai, VERMELHO);
                    rotacaoEsquerda(z->pai->pai);
                }
            }
        }
        pintar(raiz_, PRETO);
    }

    // ---------- correção após remoção ----------
    // x carrega um "preto extra" (duplo-preto) que precisa ser absorvido.
    void corrigirRemocao(No* x) {
        while (x != raiz_ && x->cor == PRETO) {
            if (x == x->pai->esq) {
                No* w = x->pai->dir;                   // irmão
                if (w->cor == VERMELHO) {              // caso 1: irmão vermelho
                    pintar(w, PRETO);
                    pintar(x->pai, VERMELHO);
                    rotacaoEsquerda(x->pai);
                    w = x->pai->dir;
                }
                if (w->esq->cor == PRETO && w->dir->cor == PRETO) {  // caso 2: sobrinhos pretos
                    pintar(w, VERMELHO);
                    x = x->pai;
                } else {
                    if (w->dir->cor == PRETO) {        // caso 3: sobrinho distante preto
                        pintar(w->esq, PRETO);
                        pintar(w, VERMELHO);
                        rotacaoDireita(w);
                        w = x->pai->dir;
                    }
                    pintar(w, x->pai->cor);            // caso 4: sobrinho distante vermelho
                    pintar(x->pai, PRETO);
                    pintar(w->dir, PRETO);
                    rotacaoEsquerda(x->pai);
                    x = raiz_;
                }
            } else {                                   // espelho
                No* w = x->pai->esq;
                if (w->cor == VERMELHO) {
                    pintar(w, PRETO);
                    pintar(x->pai, VERMELHO);
                    rotacaoDireita(x->pai);
                    w = x->pai->esq;
                }
                if (w->dir->cor == PRETO && w->esq->cor == PRETO) {
                    pintar(w, VERMELHO);
                    x = x->pai;
                } else {
                    if (w->esq->cor == PRETO) {
                        pintar(w->dir, PRETO);
                        pintar(w, VERMELHO);
                        rotacaoEsquerda(w);
                        w = x->pai->esq;
                    }
                    pintar(w, x->pai->cor);
                    pintar(x->pai, PRETO);
                    pintar(w->esq, PRETO);
                    rotacaoDireita(x->pai);
                    x = raiz_;
                }
            }
        }
        pintar(x, PRETO);
    }

    // ---------- verificação (usada por validar) ----------
    // Retorna a "altura negra" da subárvore (contando a folha nula como 1),
    // ou -1 se achou alguma violação.
    int verificar(const No* n, long long lo, long long hi, std::string& erro,
                  std::size_t& contados) const {
        if (n == nil_) return 1;
        ++contados;
        const std::string id = std::to_string(n->chave);
        if (n->chave <= lo || n->chave >= hi) {
            erro = "violação da ordem de busca no nó " + id;
            return -1;
        }
        if (n->cor == VERMELHO && (n->esq->cor == VERMELHO || n->dir->cor == VERMELHO)) {
            erro = "nó vermelho " + id + " com filho vermelho";
            return -1;
        }
        if ((n->esq != nil_ && n->esq->pai != n) || (n->dir != nil_ && n->dir->pai != n)) {
            erro = "ponteiro para o pai inconsistente abaixo do nó " + id;
            return -1;
        }
        const int be = verificar(n->esq, lo, n->chave, erro, contados);
        if (be < 0) return -1;
        const int bd = verificar(n->dir, n->chave, hi, erro, contados);
        if (bd < 0) return -1;
        if (be != bd) {
            erro = "altura negra diferente entre as subárvores do nó " + id;
            return -1;
        }
        return be + (n->cor == PRETO ? 1 : 0);
    }

    // ---------- impressão ----------
    static std::string rotulo(const No* n) {
        const bool vermelho = (n->cor == VERMELHO);
        std::string s = std::to_string(n->chave) + (vermelho ? " (V)" : " (P)");
        if (usarCor && vermelho) s = "\033[31m" + s + "\033[0m";
        return s;
    }

    void desenhar(std::ostream& os, const No* n, const std::string& pref, bool ehDireita) const {
        if (n->dir != nil_) desenhar(os, n->dir, pref + (ehDireita ? "    " : "|   "), true);
        os << pref << (ehDireita ? "/-- " : "\\-- ") << rotulo(n) << "\n";
        if (n->esq != nil_) desenhar(os, n->esq, pref + (ehDireita ? "|   " : "    "), false);
    }
};
