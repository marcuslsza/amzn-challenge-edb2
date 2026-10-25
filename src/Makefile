CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

HEADERS = arvore.hpp avl.hpp rubronegra.hpp bst.hpp dados.hpp

all: arvores testes benchmark

arvores: main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

testes: testes.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ testes.cpp

benchmark: benchmark.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ benchmark.cpp

# make run   -> sistema interativo
run: arvores
	./arvores

# make test  -> testes de correção (código de saída != 0 se algum falhar)
test: testes
	./testes

# make bench -> comparação experimental (gera resultados_benchmark.csv)
bench: benchmark
	./benchmark

clean:
	rm -f arvores testes benchmark testes_asan resultados_benchmark.csv

.PHONY: all run test bench clean
