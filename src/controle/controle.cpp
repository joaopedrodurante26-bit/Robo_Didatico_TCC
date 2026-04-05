// =====================================================
// MÓDULO DE CONTROLE - IMPLEMENTAÇÃO
// =====================================================
// Responsável por:
// - Armazenar o comando atual do robô
// - Receber comandos externos (Wi-Fi, joystick)
// - Fornecer o comando para outros módulos (motores)
//
// Este módulo NÃO executa ações diretamente.
// Ele apenas define "o que deve ser feito".
// =====================================================

#include "controle.h"

// =====================================================
// VARIÁVEIS GLOBAIS (ESTADO DO SISTEMA)
// =====================================================

// Armazena o comando atual do robô
static Comando comandoAtual = PARAR;

// (Futuro) Estado geral do sistema
// static EstadoSistema estadoAtual = MANUAL;

// =====================================================
// INICIALIZAÇÃO
// =====================================================

void initControle() {
    // Define estado inicial do robô
    comandoAtual = PARAR;
    // IMPORTANTE:
    // A variável abaixo estava sendo criada localmente e não era usada.
    // Se quisermos um estado global, ela deve ser declarada fora da função.
    //
    // bool sistemaAtivo = true;  ← REMOVIDO (não funcional)
}

// =====================================================
// SETTERS (ENTRADA DE COMANDOS)
// =====================================================

void setComando(Comando cmd) {
    // Atualiza o comando atual
    comandoAtual = cmd;
}

// =====================================================
// GETTERS (SAÍDA PARA OUTROS MÓDULOS)
// =====================================================

Comando getComando() {
    return comandoAtual;
}

// =====================================================
// LÓGICA DE CONTROLE
// =====================================================
// Esta função NÃO executa motores diretamente.
// Ela serve como ponto central para lógica futura:
//
// - filtros de comando
// - prioridade entre modos
// - integração com sensores
// - modo automático
//
// Atualmente, funciona como estrutura base.
//

void atualizarControle() {
    switch (comandoAtual) {

        case FRENTE:
            // Futuro: lógica para movimento frontal
            break;

        case TRAS:
            // Futuro: lógica para movimento reverso
            break;

        case ESQUERDA:
            // Futuro: lógica para rotação à esquerda
            break;

        case DIREITA:
            // Futuro: lógica para rotação à direita
            break;

        case PARAR:
        default:
            // Futuro: lógica para parada segura
            break;
    }
}
