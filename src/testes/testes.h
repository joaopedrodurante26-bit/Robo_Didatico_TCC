#ifndef TESTES_H
#define TESTES_H

// Módulo de testes / console de diagnóstico
// - Implementa uma interface via Serial para manutenção
// - Máquina de estados com submenus para Motores, Sensores, WiFi, FS e Diagnóstico
// - Usa os módulos já existentes (motores, sensores, wifi, diagnostico, LittleFS)

// Inicializa o console de testes. Deve ser chamado no setup().
void testes_iniciar();

// Deve ser chamado periodicamente no loop() para processar entrada/streams.
void atualizarTestes();

#endif

