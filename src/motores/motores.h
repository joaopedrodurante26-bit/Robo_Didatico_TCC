// =====================================================
// MÓDULO DE MOTORES - INTERFACE
// =====================================================
// Responsável por:
// - Controlar os motores do robô
// - Executar comandos vindos do módulo de controle
// - Traduzir velocidade e direção em sinais para a ponte H
//
// Este módulo é responsável apenas pela execução física.
// NÃO deve conter lógica de decisão.
//
// Integração:
// Controle → Motores → Hardware
// =====================================================

#ifndef MOTORES_H
#define MOTORES_H

// =====================================================
// INICIALIZAÇÃO E LOOP
// =====================================================

// -----------------------------------------------------
// Inicializa os motores
// -----------------------------------------------------
// Deve configurar pinos, PWM e estado inicial.
//
void initMotores();

// -----------------------------------------------------
// Atualiza o estado dos motores
// -----------------------------------------------------
// Deve ser chamada no loop()
// Responsável por aplicar o comando atual.
//
void atualizarMotores();

// =====================================================
// CONTROLE DE BAIXO NÍVEL (DIFERENCIAL)
// =====================================================

// -----------------------------------------------------
// Define velocidades dos lados do robô
// -----------------------------------------------------
// velEsq  → velocidade do lado esquerdo (-255 a 255)
// velDir  → velocidade do lado direito (-255 a 255)
//
// Valores positivos → frente
// Valores negativos → trás
//
// Esta é a função mais importante do módulo.
// Todas as outras devem usar essa como base.
//
void setVelocidade(int velEsq, int velDir);

// =====================================================
// CONTROLE DE ALTO NÍVEL
// =====================================================
// Funções auxiliares para facilitar uso e testes.
// Internamente chamam setVelocidade().
// =====================================================

// Movimento para frente
void moverFrente(int velocidade);

// Movimento para trás
void moverTras(int velocidade);

// Rotação no próprio eixo (esquerda)
void virarEsquerda(int velocidade);

// Rotação no próprio eixo (direita)
void virarDireita(int velocidade);

// Parada total dos motores
void pararMotores();

#endif