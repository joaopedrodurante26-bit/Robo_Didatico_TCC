#include "testes.h"

//======================================================
// Variáveis
//======================================================

String comando = "";

//======================================================
// Inicialização
//======================================================

void testes_iniciar()
{
    Serial.begin(115200);
    delay(1000);

    testes_exibirCabecalho();
    testes_exibirAjuda();
}

//======================================================
// Loop principal
//======================================================

void testes_atualizar()
{
    if (Serial.available())
    {
        comando = Serial.readStringUntil('\n');
        comando.trim();

        if (comando.length() > 0)
        {
            testes_processarComando(comando);
        }

        Serial.println();
        Serial.print("> ");
    }
}

//======================================================
// Processamento dos comandos
//======================================================

void testes_processarComando(String comando)
{
    comando.toUpperCase();

    if (comando == "HELP")
    {
        testes_exibirAjuda();
    }

    else if (comando == "STATUS")
    {
        Serial.println("STATUS:");
        Serial.println("Sistema em funcionamento.");
    }

    else if (comando == "CLS")
    {
        for (int i = 0; i < 40; i++)
            Serial.println();
    }

    else
    {
        Serial.print("ERRO: Comando desconhecido -> ");
        Serial.println(comando);
        Serial.println("Digite HELP para listar os comandos.");
    }
}

//======================================================
// Cabeçalho
//======================================================

void testes_exibirCabecalho()
{
    Serial.println();
    Serial.println("=========================================");
    Serial.println(" ROBÔ EDUCACIONAL");
    Serial.println(" MODO DE TESTES");
    Serial.println("=========================================");
}

//======================================================
// Ajuda
//======================================================

void testes_exibirAjuda()
{
    Serial.println();
    Serial.println("Comandos disponíveis:");
    Serial.println("-----------------------------------------");
    Serial.println("HELP");
    Serial.println("STATUS");
    Serial.println("CLS");
}