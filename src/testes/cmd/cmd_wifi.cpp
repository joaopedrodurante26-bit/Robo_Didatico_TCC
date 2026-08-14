#include "cmd_wifi.h"

#include <WiFi.h>
#include <WiFiClient.h>

#include "../../wifi/wifi_manager.h"

static uint16_t parsePortOrDefault(const String& value, uint16_t defaultPort) {
    String text = value;
    text.trim();
    if (text.length() == 0) {
        return defaultPort;
    }

    long parsed = text.toInt();
    if (parsed <= 0 || parsed > 65535) {
        return defaultPort;
    }

    return (uint16_t)parsed;
}

static String securityToString(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        case WIFI_AUTH_WAPI_PSK: return "WAPI";
        default: return "UNKNOWN";
    }
}

static void cmd_wifi_status(String args) {
    console_println("WIFI STATUS");
    console_println("");
    console_println("SSID.............." + WiFi.softAPSSID());
    console_println("IP................" + WiFi.softAPIP().toString());
    console_println("Clientes.........." + String(WiFi.softAPgetStationNum()));
    console_println("Modo.............." + String((int)WiFi.getMode()));
    console_println("Saude............." + wifiGetHealthLabel());
    console_println("AP saudavel......." + String(wifiIsApHealthy() ? "SIM" : "NAO"));
}

static void cmd_wifi_clients(String args) {
    console_println("Clientes conectados: " + String(WiFi.softAPgetStationNum()));
}

static void cmd_wifi_ip(String args) {
    console_println("IP do AP: " + WiFi.softAPIP().toString());
}

static void cmd_wifi_ssid(String args) {
    console_println("SSID atual: " + WiFi.softAPSSID());
}

static void cmd_wifi_diag(String args) {
    console_println("WIFI DIAGNOSTICO (JSON)");
    console_println(wifiGetDiagnosticsJson());
}

static void cmd_wifi_scan(String args) {
    String text = args;
    text.trim();

    int maxResults = 10;
    if (text.length() > 0) {
        long parsed = text.toInt();
        if (parsed > 0) {
            maxResults = (int)parsed;
            if (maxResults > 30) {
                maxResults = 30;
            }
        }
    }

    wifi_mode_t previousMode = WiFi.getMode();
    bool changedMode = false;
    if (previousMode == WIFI_MODE_AP) {
        WiFi.mode(WIFI_MODE_APSTA);
        delay(80);
        changedMode = true;
    }

    console_println("WIFI SCAN em andamento...");
    int found = WiFi.scanNetworks();

    if (changedMode) {
        WiFi.mode(previousMode);
        delay(40);
    }

    if (found < 0) {
        console_println("[FAIL] Nao foi possivel concluir o scan.");
        WiFi.scanDelete();
        return;
    }

    console_println("Redes encontradas: " + String(found));
    int limit = found < maxResults ? found : maxResults;

    for (int i = 0; i < limit; ++i) {
        String line = "[" + String(i + 1) + "] ";
        line += WiFi.SSID(i);
        line += " | RSSI " + String(WiFi.RSSI(i)) + " dBm";
        line += " | CH " + String(WiFi.channel(i));
        line += " | " + securityToString((wifi_auth_mode_t)WiFi.encryptionType(i));
        console_println(line);
    }

    if (found > limit) {
        console_println("(mostrando " + String(limit) + " de " + String(found) + ")");
    }

    WiFi.scanDelete();
}

static void cmd_wifi_ping(String args) {
    String input = args;
    input.trim();

    String host = WiFi.softAPIP().toString();
    uint16_t port = 80;

    if (input.length() > 0) {
        int sep = input.indexOf(' ');
        if (sep < 0) {
            host = input;
        } else {
            host = input.substring(0, sep);
            String portText = input.substring(sep + 1);
            portText.trim();
            port = parsePortOrDefault(portText, 80);
        }
    }

    host.trim();
    if (host.length() == 0) {
        console_println("Uso: PING [host|ip] [porta]");
        return;
    }

    WiFiClient client;
    client.setTimeout(1200);

    unsigned long startMs = millis();
    bool ok = client.connect(host.c_str(), port);
    unsigned long elapsedMs = millis() - startMs;

    if (ok) {
        console_println("[OK] Alcance de " + host + ":" + String(port) + " em " + String(elapsedMs) + " ms");
        client.stop();
        return;
    }

    console_println("[FAIL] Sem resposta de " + host + ":" + String(port) + " em " + String(elapsedMs) + " ms");
}

static void cmd_wifi_recover(String args) {
    console_println("Tentando recuperar AP...");
    if (wifiRecoverNow()) {
        console_println("[OK] AP recuperado.");
    } else {
        console_println("[FAIL] Nao foi possivel recuperar o AP.");
    }

    cmd_wifi_status("");
}

static void cmd_wifi_reset_diag(String args) {
    wifiResetDiagnostics();
    console_println("[OK] Contadores de diagnostico WIFI reiniciados.");
}

static void cmd_wifi_help(String args) {
    if (!console_printTextFile("/help/wifi_menu.txt")) {
        console_println("Ajuda indisponivel no momento.");
    }
}

static void cmd_wifi_exit(String args) {
    console_setState(STATE_MAIN);
    console_println("[OK] WIFI EXIT");
}

static Command comandosWifi[] = {
    {"STATUS", cmd_wifi_status, "Resumo do AP e saude"},
    {"INFO", cmd_wifi_status, "Resumo do AP e saude (alias)"},
    {"SCAN", cmd_wifi_scan, "Varre redes proximas"},
    {"PING", cmd_wifi_ping, "Teste de alcance TCP host/ip:porta"},
    {"CLIENTS", cmd_wifi_clients, "Numero de clientes conectados"},
    {"IP", cmd_wifi_ip, "Mostra IP do Access Point"},
    {"SSID", cmd_wifi_ssid, "Mostra SSID atual"},
    {"DIAG", cmd_wifi_diag, "Mostra diagnostico detalhado em JSON"},
    {"RECOVER", cmd_wifi_recover, "Forca recuperacao do AP"},
    {"RESTART", cmd_wifi_recover, "Forca recuperacao do AP (alias)"},
    {"RESET DIAG", cmd_wifi_reset_diag, "Zera contadores de diagnostico"},
    {"HELP", cmd_wifi_help, "Ajuda do menu WIFI"},
    {"EXIT", cmd_wifi_exit, "Volta ao menu principal"},
};

Command* getWifiCommands(size_t &count) {
    count = sizeof(comandosWifi) / sizeof(Command);
    return comandosWifi;
}
