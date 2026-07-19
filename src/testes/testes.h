#ifndef TESTES_H
#define TESTES_H

#include <Arduino.h>

//======================================================
// Inicialização
//======================================================

void testes_iniciar();

//======================================================
// Loop principal do modo de testes
//======================================================

void testes_atualizar();

//======================================================
// Comandos
//======================================================

void testes_processarComando(String comando);

//======================================================
// Mensagens
//======================================================

void testes_exibirCabecalho();
void testes_exibirAjuda();

#endif