#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Configurações do MQTT
const char* mqttServer = "ahbccp4thrap8-ats.iot.us-east-1.amazonaws.com";
const int mqttPort = 8883;
const char* mqttUser = "";
const char* mqttPassword = "";

// Certificados para comunicação segura
const char* rootCA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

const char* certificate = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAM2C+veS4kzyw0/8Q3s2veqyuJHnMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yNDA4MjUxNjM2
MjhaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCxGgblcebA1nONeDjG
BD+ENCKwN34bmg8N+AJKHXw8G+wb1pFvtbFvmD0ExRBsrk4DSVLVJ+2GcJRnmEIE
P+5NJ/82EJcUIqE5J/i6Rm+TukaSoBNsL2Il9IyU/XOqkiZwiPMFdBkpNoQwniA1
+m931dysNNs0ExKYnk00KA9M1j+eTsbm3BCk3a95yN7YBZwgbs+3+2T7cIX3crZU
aImdH0Qf/2k03MjbIcFb94rcGZFVcrSme2wgkd3fhgWEybo4SVl+WN22hn5wX+EG
eb5HRXDD94f3I2n0JfuN2/hONPZaY6zjopZzrIXCgqBtoWlVEy7KCdl2t8wA+lHS
+GS/AgMBAAGjYDBeMB8GA1UdIwQYMBaAFAIrEA6HDSK65G0VdCVx1pDseItkMB0G
A1UdDgQWBBRHmNiLMMHmitzd1ayj2tyIdHQD/DAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAicKxSqI3gq/JT8LjpiVQYmQm
M//JoP6g06cWt/BAvPVY12TwemR566Egi9lr6u0ie7QFjxTu9hwonpIJGWALpbPK
Adrm/X5c3NhGavdwb2LTFtCkNdjOVlxNHf3tnSU6Mlqugd/oG5UtQYwTKt82naTr
oNKNwaSvMCzMpzwvaeCAb3iidoVc1rpaZ8Fr061GAy03LMMcPtV/j/t0DH2MSAsP
NQaAveKj3ke/2UKXYLc1W2/IElR2zbX1RdT37uSrccauOI4dNwOtuaYAvz3xkEn0
gT/Z7J/Bw7Uoqc00h0JwVsbRBWLosR4RIXSkMt9N387Vdi3hkY8SChGxAaqAnA==
-----END CERTIFICATE-----
)EOF";

const char* privateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAsRoG5XHmwNZzjXg4xgQ/hDQisDd+G5oPDfgCSh18PBvsG9aR
b7Wxb5g9BMUQbK5OA0lS1SfthnCUZ5hCBD/uTSf/NhCXFCKhOSf4ukZvk7pGkqAT
bC9iJfSMlP1zqpImcIjzBXQZKTaEMJ4gNfpvd9XcrDTbNBMSmJ5NNCgPTNY/nk7G
5twQpN2vecje2AWcIG7Pt/tk+3CF93K2VGiJnR9EH/9pNNzI2yHBW/eK3BmRVXK0
pntsIJHd34YFhMm6OElZfljdtoZ+cF/hBnm+R0Vww/eH9yNp9CX7jdv4TjT2WmOs
46KWc6yFwoKgbaFpVRMuygnZdrfMAPpR0vhkvwIDAQABAoIBAQCJMD8tV7lHohfq
+7kG8118fKJuXN5MZV/KE1c6sHJ/YaXZvrH0lgu4BXcnDbx2Y+O8ufz3b7GYlfbv
9MsW3assi5IwAFP33geD7gnHyi4+gmqOxH+nK2FdQ33vIBKMjCBIxl1y2QdwnHFz
89nB1piofLsvjtZLFYcvQFlP0MRhYL6t0GinLNO9OiMl58NFK/Vn1sNQT8dZ+29A
J139q6lAxbRPtA3xLKmktYuK3utvf6ePgEdjgptykxt1cL6TZN84sQhERRHsj+Jg
FnueSzGAbnEqRfMjCfShAwiCeDZJDxb3Q5cNOL+op0sNaQ2HE7JNb6cvHtTGg2v4
Xjv5B7+ZAoGBAOlNjyK/ZUjK1TYpEkKO97PswWESAhyTxlps+Xuk6H+61lLAl+Z8
pMeo3J/2Sbzd/0oslY4OtC3o30z0pjqSx9waYnwR9zRpwxi7lBj1QwujjT9iLQIk
QQDmK56bN+adBl1SbXuiH5uDO4luxi7WTM/oaMBB0TAIgMjmbjMJRcmjAoGBAMJU
xGOYYvaoQf0ItbeRAGuLK+vSUX2Q0unaOZSKTq572U2lX86n2OgFh95c+mAtQYeV
bDnut/Q19cY4dEsmHFajY7NtO2RHKW9/yyHrESa7s95Ltr0GkjGSDoF9RQlhZoKw
vcTGQA4J6sMRwKeWTkD17oy70U7sYszOOIgaKiI1AoGBAIQV1SPnGIDN1UiEmEH2
j2bec91xRKDJSVOIvvaxtrAaJ51STK3Bg8lGYSJvXe///7kO5N04leooHcSD/ljL
ITwL9BYqVbrm5f3qtT4sHXlJb36jJrg/rk3EAo4ZctytqhzLvhBxCVQSSHLWtH1v
9qz+989hc/2t0rvrOhjK9yfhAoGAElfB+cymerXDFMk/rcYIDh6i1K2Td5C199Db
YoPzjVCvYD7d3jnFpZDwFDMehvl+l/eosYcw4eI+AqnNgFJmll7xyDvlQT/eT2H2
oIr5oXhSMAdhhlIadodcyygx3gLNlERuhSZZYXnaKPOCBc+QL12HWM1ZomN5p+9H
MsGVfIkCgYBPxEGYBPkew1wZozJk5Gu7CFFZGrDFcdI7NnkBNdrWQjc6E7zjAUkM
hg1JTXYxBTofwasiFuq95mnGInQoc756bWzT9xUTCAkM+xzzlE7dtsx7CMXZsLL8
1AJOkYckUhUbO7NsvXe1LczGzDLV6+xA+j29TSdQsNSdukuht+hyAg==
-----END RSA PRIVATE KEY-----
)EOF";

WiFiClientSecure espClient;
PubSubClient client(espClient);

const int analogPin = 36;
const int totalReadings = 10;
const unsigned long readingInterval = 2 * 60 * 60 * 1000; // 2 horas em milissegundos

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Espera a serial estar pronta
  }
  delay(1000);
  Serial.println("\n--- Iniciando setup... ---");

  setupWiFi();

  // Configura o cliente MQTT
  Serial.println("Configurando cliente MQTT...");
  client.setServer(mqttServer, mqttPort);
  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);
  Serial.println("Cliente MQTT configurado.");

  // Tenta conectar ao MQTT
  reconnectMQTT();

  Serial.println("--- Setup concluído ---");
}

void loop() {
  Serial.println("\n--- Iniciando ciclo de leituras ---");
  
  while (true) {
    if (!client.connected()) {
      Serial.println("Cliente MQTT desconectado. Tentando reconectar...");
      reconnectMQTT();
    }
    client.loop();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conexão WiFi perdida. Tentando reconectar...");
      setupWiFi();
    }

    float average = readAverageMoisture();
    String moistureLevel = classifyMoistureLevel(average);

    // Publica os dados no MQTT
    String payload = "{\"moisture\":" + String(average) + ", \"status\":\"" + moistureLevel + "\"}";
    Serial.println("Tentando publicar no tópico 'moisture_sensor'...");
    if (client.publish("moisture_sensor", payload.c_str())) {
      Serial.println("Publicação bem-sucedida!");
      Serial.println(payload.c_str());
    } else {
      Serial.println("Falha na publicação. Estado do cliente: " + String(client.state()));
    }

    Serial.println("Deseja fazer outra leitura? (S/N)");
    String response = promptForInput("");
    
    if (response.equalsIgnoreCase("N")) {
      Serial.println("Aguardando 2 horas antes da próxima verificação...");
      delay(readingInterval);
    } else if (!response.equalsIgnoreCase("S")) {
      Serial.println("Resposta inválida. Assumindo 'Não'.");
      Serial.println("Aguardando 2 horas antes da próxima verificação...");
      delay(readingInterval);
    }
  }
}

void setupWiFi() {
  while (true) {
    scanNetworks();

    String ssid = promptForInput("Digite o nome da rede WiFi: ");
    String password = promptForInput("Digite a senha da rede WiFi: ");

    Serial.println("SSID fornecido: " + ssid);
    Serial.println("Senha fornecida: " + password);

    if (connectToWiFi(ssid.c_str(), password.c_str())) {
      break;
    } else {
      Serial.println("Falha na conexão. Deseja tentar novamente? (S/N)");
      String response = promptForInput("");
      if (response.equals("s")) {
        Serial.println("Encerrando tentativas de conexão WiFi.");
        break;
      }
    }
  }
}

void scanNetworks() {
  Serial.println("Escaneando redes WiFi...");
  int n = WiFi.scanNetworks();
  Serial.println("Escaneamento concluído");
  if (n == 0) {
    Serial.println("Nenhuma rede encontrada");
  } else {
    Serial.print(n);
    Serial.println(" redes encontradas");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }
  Serial.println("");
}

String promptForInput(const char* prompt) {
  Serial.println(prompt);
  while (!Serial.available()) {
    // Espera pela entrada do usuário
  }
  String input = Serial.readStringUntil('\n');
  input.trim();  // Remove espaços em branco no início e no fim
  Serial.println("Entrada recebida: " + input);
  return input;
}

bool connectToWiFi(const char* ssid, const char* password) {
  Serial.println("Tentando conectar-se a " + String(ssid));

  WiFi.begin(ssid, password);

  int tentativa = 0;
  while (WiFi.status() != WL_CONNECTED && tentativa < 30) {
    delay(1000);
    tentativa++;
    Serial.print("Tentativa ");
    Serial.print(tentativa);
    Serial.print(" - Status da conexão: ");
    Serial.println(WiFi.status());
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Falha ao conectar ao WiFi.");
    Serial.print("Código do erro: ");
    Serial.println(WiFi.status());
    return false;
  }
}

void reconnectMQTT() {
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    Serial.print("Tentativa ");
    Serial.print(attempts + 1);
    Serial.println(" de conexão ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao MQTT com sucesso!");
      return;
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
    attempts++;
  }
  if (!client.connected()) {
    Serial.println("Falha em todas as tentativas de conexão MQTT.");
    Serial.println("Deseja tentar novamente? (S/N)");
    String response = promptForInput("");
    if (response.equals("s")) {
      reconnectMQTT();
    } else {
      Serial.println("Encerrando tentativas de conexão MQTT.");
    }
  }
}

float readAverageMoisture() {
  Serial.println("Iniciando leituras de umidade...");
  int sum = 0;
  for (int i = 0; i < totalReadings; i++) {
    int sensorValue = analogRead(analogPin);
    float percentage = (4095 - sensorValue) / 40.95;
    sum += percentage;
    Serial.print("Leitura ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(percentage);
    Serial.println("%");
    delay(1000);
  }
  float average = sum / totalReadings;
  Serial.print("Média das leituras: ");
  Serial.print(average);
  Serial.println("%");
  return average;
}

String classifyMoistureLevel(float moisture) {
  String classification;
  if (moisture < 20) {
    classification = "muito seco";
  } else if (moisture < 40) {
    classification = "seco";
  } else if (moisture < 60) {
    classification = "normal";
  } else if (moisture < 80) {
    classification = "úmido";
  } else {
    classification = "muito úmido";
  }
  Serial.println("Classificação da umidade: " + classification);
  return classification;
}
