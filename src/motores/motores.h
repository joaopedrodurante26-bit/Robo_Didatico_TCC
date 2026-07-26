#ifndef MOTORES_H
#define MOTORES_H

// Inicialização
void initMotores();

// Atualização
void atualizarMotores();

// Estado da trava de segurança
bool motoresSegurancaAtiva();

// Marca o início/fim de um comando de movimento atual
void motores_iniciarComando();
void motores_finalizarComando();

// Controle diferencial (-255 a 255)
void setVelocidade(int velEsq, int velDir);

// Movimentos
void moverFrente(int velocidade);
void moverTras(int velocidade);
void virarEsquerda(int velocidade);
void virarDireita(int velocidade);
void pararMotores();

#endif