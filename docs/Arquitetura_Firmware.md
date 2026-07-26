# Arquitetura do Firmware

Este documento descreve a arquitetura atual do firmware do robô, com foco em separação de responsabilidades, fluxo de comandos e fluxo de sensores.

## 1. Visão Geral de Camadas

```mermaid
flowchart TD
    UI[Interfaces: Web + Serial] --> CC[Console Core]
    CC --> CP[Command Processor]
    CP --> RB[Robot / RobotMode]

    RB --> M[Motores]
    RB --> SM[SensorManager]
    RB --> WF[WiFi Manager]

    SM --> S[Sensores Físicos]
    SM --> RS[RobotState]
    WF --> RS
    RB --> RS
```

### Resumo
- As interfaces (Web e Serial) apenas transportam comandos e exibem respostas.
- O processamento de comandos é centralizado no Console Core.
- O `Robot` define comportamento (`RobotMode`) e interface ativa (`InterfaceMode`).
- O `SensorManager` encapsula aquisição de sensores e alimenta o `RobotState`.
- O `RobotState` é a fonte única de estado para APIs e telas.

## 2. Fluxo de Comando (Web/Serial)

```mermaid
flowchart TD
    W[Console Web] --> C[Console Core]
    S[Console Serial] --> C

    C --> P[Parser + Dispatcher]
    P --> T[Command Tables: Main/Motor/Sensor]
    T --> R[Robot API / Módulos]
    R --> MT[Motores]
```

### Observações
- Comandos podem ser enviados por qualquer interface e cair no mesmo interpretador.
- Comandos qualificados funcionam em qualquer prompt (ex.: `MOTOR F 120`, `SYSTEM STATUS`).
- A resposta é espelhada para Serial e Web via saída unificada do console.

## 3. Fluxo de Sensores

```mermaid
flowchart TD
    MPU[MPU6050] --> SM[SensorManager]
    U[Ultrassônico] --> SM
    E[Encoders] --> SM

    SM --> RS[RobotState]
    RS --> MON[Web Monitor / API /status /state]
    RS --> CTRL[Controle / Lógica]
```

### Observações
- Módulos acima do `SensorManager` não dependem do hardware específico.
- `RobotState` oferece snapshot consolidado para monitoramento e decisões.

## 4. Scheduler (Loop Principal)

O loop principal utiliza frequências diferentes por responsabilidade:

- Sensores (`SensorManager`): ~200 Hz (5 ms)
- Controle + Motores: ~50 Hz (20 ms)
- WiFi/API Web: ~10 Hz (100 ms)
- Console: processamento contínuo por iteração

Isso melhora previsibilidade e reduz acoplamento temporal entre módulos.

## 5. Estruturas-Chave

### RobotMode (comportamento)
- `MODE_IDLE`
- `MODE_MANUAL`
- `MODE_AUTONOMOUS`
- `MODE_CALIBRATION`

### InterfaceMode (interface ativa)
- `UI_CONSOLE`
- `UI_CONTROL`
- `UI_MONITOR`
- `UI_CONFIGURATION`

### RobotState (estado consolidado)
Inclui, entre outros:
- modo e interface
- PWM esquerdo/direito
- encoders
- distância
- IMU (aceleração e giroscópio)
- conectividade WiFi
- uptime

## 6. Princípios Arquiteturais Atuais

1. Um único núcleo de comandos para todas as interfaces.
2. Separação entre comportamento do robô e tipo de interface.
3. Fonte única de estado para observabilidade e integração.
4. Camada de sensores abstraída para suportar evolução de hardware.
5. Evolução incremental com compatibilidade de comandos legados.

## 7. Próximas Evoluções Recomendadas

1. `RobotConfig` com persistência em LittleFS (`config.json`, `pid.json`, etc.).
2. Event manager para reações orientadas a evento (ex.: obstáculo crítico).
3. Watchdog lógico para falhas de sensores e modos degradados.
4. Diagnóstico expandido com relatório estruturado para apresentação do TCC.
