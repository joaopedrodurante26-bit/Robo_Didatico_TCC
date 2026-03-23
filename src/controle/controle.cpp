#include "controle.h"

Comando comandoAtual = PARAR;

void initControle() {
    bool sistemaAtivo = true;
    comandoAtual = PARAR;
}

void setComando(Comando cmd) {
    comandoAtual = cmd;
}

Comando getComando() {
    return comandoAtual;
}

void atualizarControle() {
    if (comandoAtual == FRENTE) {
    // mover frente
    } else if (comandoAtual == TRAS) {
        // mover tras
    } else if (comandoAtual == ESQUERDA) {
        // mover esquerda
    } else if (comandoAtual == DIREITA) {
        // mover direita
    } else if (comandoAtual == PARAR) {
        // parar movimento
    }
}