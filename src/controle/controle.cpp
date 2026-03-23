#include "controle.h"

String comandoAtual = "parar";

void initControle() {
    comandoAtual = "parar";
}

void setComando(const String& cmd) {
    comandoAtual = cmd;
}

void atualizarControle() {
    if (comandoAtual == "frente") {
    // mover frente
    } else if (comandoAtual == "tras") {
        // mover tras
    } else if (comandoAtual == "esquerda") {
        // mover esquerda
    } else if (comandoAtual == "direita") {
        // mover direita
    } else if (comandoAtual == "parar") {
        // parar movimento
    }