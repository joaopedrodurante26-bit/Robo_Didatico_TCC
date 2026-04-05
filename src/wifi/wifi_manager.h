// =====================================================
// MÓDULO WIFI MANAGER - INTERFACE
// =====================================================
// Responsável por:
// - Inicializar a rede Wi-Fi (modo Access Point)
// - Gerenciar o servidor web embarcado
// - Receber comandos remotos (ex: joystick)
// - Disponibilizar dados do robô via HTTP
//
// Este módulo atua como interface entre:
// Usuário (web) ↔ Robô (controle + sensores)
//
// Ele NÃO deve conter lógica de controle do robô,
// apenas comunicação.
// =====================================================

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// =====================================================
// INICIALIZAÇÃO E LOOP
// =====================================================

// -----------------------------------------------------
// Inicializa o Wi-Fi e o servidor web
// -----------------------------------------------------
// Deve ser chamado no setup()
// Responsável por:
// - Criar rede (Access Point)
// - Configurar rotas HTTP
// - Iniciar servidor
//
void initWiFi();

// -----------------------------------------------------
// Atualiza o sistema Wi-Fi
// -----------------------------------------------------
// Deve ser chamado no loop()
// Responsável por:
// - Processar requisições HTTP
// - Manter conexão ativa
//
void atualizarWiFi();

#endif