#!/usr/bin/env python3
"""Gera gráficos de colunas a partir do CSV produzido por ./benchmark.

Uso:
    python graficos.py                              # lê resultados_benchmark.csv
    python graficos.py --csv outro.csv --saida graficos
    python graficos.py --n 100000                   # só um tamanho de N
    python graficos.py --com-bst                    # inclui a BST (linha de base)
    python graficos.py --mostrar                    # também abre as janelas

Requer:  pip install matplotlib
Saída:   uma imagem PNG por valor de N, com 6 painéis (altura, rotações e tempos).
"""
import argparse
import csv
import os
import sys
from collections import defaultdict

CENARIOS = ["crescente", "decrescente", "aleatoria", "lotes"]
ROTULO_CENARIO = {
    "crescente": "crescente",
    "decrescente": "decrescente",
    "aleatoria": "aleatória",
    "lotes": "em lotes",
}
# (coluna do CSV, título do painel, rótulo do eixo y)
PAINEIS = [
    ("altura_apos_insercao", "Altura após a inserção", "altura (nós)"),
    ("altura_apos_remocao", "Altura após remover 50%", "altura (nós)"),
    ("rotacoes_insercao", "Rotações na inserção", "rotações"),
    ("t_insercao_ms", "Tempo de inserção", "ms"),
    ("t_busca_ms", "Tempo de busca", "ms"),
    ("t_remocao_ms", "Tempo de remoção", "ms"),
]


def formatar(valor):
    """Inteiros com separador de milhar (99.983); decimais com 2 casas (12,72)."""
    if valor >= 1000 or valor == int(valor):
        return f"{valor:,.0f}".replace(",", ".")
    return f"{valor:.2f}".replace(".", ",")
CORES = {"AVL": "#2a7fbf", "Rubro-Negra": "#d1495b", "BST": "#8d8d8d"}


def nome_curto(estrutura):
    return "BST" if estrutura.startswith("BST") else estrutura


def ler_csv(caminho):
    """Retorna dados[n][estrutura][cenario] = linha (dict)."""
    dados = defaultdict(lambda: defaultdict(dict))
    with open(caminho, newline="", encoding="utf-8") as f:
        for linha in csv.DictReader(f):
            dados[int(linha["n"])][nome_curto(linha["estrutura"])][linha["cenario"]] = linha
    return dados


def desenhar_painel(ax, dados_n, estruturas, coluna, titulo, ylabel):
    largura = 0.8 / len(estruturas)
    for i, est in enumerate(estruturas):
        xs, ys = [], []
        for j, cen in enumerate(CENARIOS):
            linha = dados_n.get(est, {}).get(cen)
            if linha is None:  # ex.: BST omitida em cenários ordenados com N grande
                continue
            xs.append(j + (i - (len(estruturas) - 1) / 2) * largura)
            ys.append(float(linha[coluna]))
        barras = ax.bar(xs, ys, width=largura, label=est, color=CORES.get(est))
        rotulos = [formatar(y) for y in ys]
        girar = 90 if max(len(r) for r in rotulos) > 5 else 0
        ax.bar_label(barras, labels=rotulos, fontsize=7, padding=2, rotation=girar)
    ax.set_xticks(range(len(CENARIOS)))
    ax.set_xticklabels([ROTULO_CENARIO[c] for c in CENARIOS])
    ax.set_title(titulo, fontsize=11)
    ax.set_ylabel(ylabel)
    ax.margins(y=0.18)
    ax.grid(axis="y", alpha=0.3)
    ax.set_axisbelow(True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--csv", default="resultados_benchmark.csv", help="arquivo gerado pelo benchmark")
    ap.add_argument("--saida", default=".", help="pasta onde salvar os PNGs")
    ap.add_argument("--n", type=int, help="gera só o gráfico deste N")
    ap.add_argument("--com-bst", action="store_true", help="inclui a BST sem balanceamento")
    ap.add_argument("--mostrar", action="store_true", help="abre os gráficos em janelas")
    args = ap.parse_args()

    import matplotlib

    if not args.mostrar:
        matplotlib.use("Agg")  # sem interface gráfica: só salva os arquivos
    import matplotlib.pyplot as plt

    if not os.path.exists(args.csv):
        sys.exit(f"Arquivo não encontrado: {args.csv}\nRode antes:  ./benchmark")
    dados = ler_csv(args.csv)
    os.makedirs(args.saida, exist_ok=True)

    tamanhos = [args.n] if args.n else sorted(dados)
    for n in tamanhos:
        if n not in dados:
            sys.exit(f"N={n} não está no CSV (disponíveis: {sorted(dados)})")
        estruturas = [e for e in ("AVL", "Rubro-Negra", "BST") if e in dados[n] and (e != "BST" or args.com_bst)]

        fig, eixos = plt.subplots(2, 3, figsize=(16, 8.5))
        for ax, (coluna, titulo, ylabel) in zip(eixos.flat, PAINEIS):
            desenhar_painel(ax, dados[n], estruturas, coluna, titulo, ylabel)
        handles, rotulos = eixos[0][0].get_legend_handles_labels()
        fig.suptitle(f"AVL x Rubro-Negra — N = {n:,}".replace(",", "."), fontsize=14, fontweight="bold")
        fig.legend(handles, rotulos, loc="upper right", bbox_to_anchor=(0.985, 0.995), ncol=len(rotulos), fontsize=10, frameon=False)
        fig.tight_layout(rect=(0, 0, 1, 0.96))

        destino = os.path.join(args.saida, f"grafico_N{n}.png")
        fig.savefig(destino, dpi=150)
        print("Gerado:", destino)
        if not args.mostrar:
            plt.close(fig)

    if args.mostrar:
        plt.show()


if __name__ == "__main__":
    main()
