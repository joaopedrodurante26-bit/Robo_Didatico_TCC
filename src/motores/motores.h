#ifndef MOTORES_H
#define MOTORES_H

void initMotores();
void atualizarMotores();

// Controle direto
void setVelocidade(int velEsq, int velDir);

// Comandos de alto nível
void moverFrente(int velocidade);
void moverTras(int velocidade);
void virarEsquerda(int velocidade);
void virarDireita(int velocidade);
void pararMotores();

#endif