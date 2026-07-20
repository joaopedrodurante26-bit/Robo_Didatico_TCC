#ifndef MOTORES_H
#define MOTORES_H

// Inicialização
void initMotores();

// Atualização
void atualizarMotores();

// Controle diferencial (-255 a 255)
void setVelocidade(int velEsq, int velDir);

// Movimentos
void moverFrente(int velocidade);
void moverTras(int velocidade);
void virarEsquerda(int velocidade);
void virarDireita(int velocidade);
void pararMotores();

#endif