#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Configurações do MQTT
const char* mqttServer = "a3bw5rp1377npv-ats.iot.us-east-1.amazonaws.com";
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
MIIDWTCCAkGgAwIBAgIUaPPths3Kli913soE6MIc2llB1KIwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MDgyMzA1MDIx
NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKx6rGTK+B1EuBklPGno
IZMO4aMvYaKTn3Y37VnV2MucAmDIcIEaim5yqgYTMPSzn0emWu6ShBPNoObXEQg9
v6zd8pXVF7z/1jdjY7drlrys6U9EH6nwAloZKnxre/BDu2LMSkahRAUjX3W2AJAc
orGKGfc7Cmg3uxjDG/UAj9o3srV0YmNHkbwKlGcBeEClhUDurg3GWJ45yFj0uvX/
3WLYjvsM6f9N/ahtJmfr+zoX0iuz04JALFEApdlWfjdZodcZ1nUjVeowNrmMbmnm
OC3vWlD4DQ6KwFVDbNa/BGF0QH1u/0CSmuudcggESW0dikBP3z1C/+qkucm9VcZ5
H00CAwEAAaNgMF4wHwYDVR0jBBgwFoAUh1mNIUYHv6CTibS4vv8sipmUKe4wHQYD
VR0OBBYEFOrWOD3b7NNEhapjKfCPC7ua5DsiMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQB/R0rjpECjgQGkohmsMgRVtFLw
IOUshF4B5j3bJECpptsOIWq/cBt1I/jyXLrPoPlIPWJyYq9oY47WO0qByVhP6vYD
HZPK/JK2nZmuio+UYBGaMP9vQPxjDgLw+aK0DQYgjwSsnKeNzq7DVcdMQzEKPpCW
Njy8TwA3NGjdSo383/1EHIs6XzZe5tpUWAwzY7oioehwi52Tmn5nYDrW1E8fr81f
zj0le90+cEaqQ4NrTlg03ln8vmpCxiy5Hj6PGPjR43xAndUezVds4lQwNIk76yLn
KLidT+MBLyPd/4+HkQBfvb4Kn+6F8EeLBEsulvm0wCxKjDFqma7PgTDPmAPm
-----END CERTIFICATE-----
)EOF";

const char* privateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEArHqsZMr4HUS4GSU8aeghkw7hoy9hopOfdjftWdXYy5wCYMhw
gRqKbnKqBhMw9LOfR6Za7pKEE82g5tcRCD2/rN3yldUXvP/WN2Njt2uWvKzpT0Qf
qfACWhkqfGt78EO7YsxKRqFEBSNfdbYAkByisYoZ9zsKaDe7GMMb9QCP2jeytXRi
Y0eRvAqUZwF4QKWFQO6uDcZYnjnIWPS69f/dYtiO+wzp/039qG0mZ+v7OhfSK7PT
gkAsUQCl2VZ+N1mh1xnWdSNV6jA2uYxuaeY4Le9aUPgNDorAVUNs1r8EYXRAfW7/
QJKa651yCARJbR2KQE/fPUL/6qS5yb1VxnkfTQIDAQABAoIBAQCJUOEPHciLBLrM
yihe6MBSI/bfxEkm9gBuTfhZHTeMYphCFVH8dvTfGcrjK0Q+jQKyOG8MoPkmCv+e
yHp25TJEni8TuqM7hTM2xZoN3UoUzFSzFrlewgYdVQvOIoR5tHLrJVm9AYb10EOf
o1avZRzh2+DwQ8D1V+lMvYj0dY4RXBap2SGWrxaAWsdpy2Xo3SgtRkvXs+AcFdrW
NAiRc68jyIAlwIgKPSU1ZerNbrbnQI5csKg2qiP3VsHaRl4S6ntkJzyORqfhy8pz
19MAKLkYCcvtGRl/pWoiONzbftQz5MAsTT1TMBgjjCnRl68Fnb3D+Y+1pu0HMNtI
EyyxHAbBAoGBAOXpI+L4Yo7ZruQ5KJzv8aoIAKvgtsqGhCkxIwiEquLujtCxxaK2
R5GJJdIv7cWH2cDB87MKG8y/ePlGcQY0nWJH57/cLtgroMl3h7N/X1jQxoUoJRnP
sVqN9kDBfSGBBYZgylQ57AxW9TkdOBHWuETeDNDlRty/CUHZDx0hilSDAoGBAMAN
KMwdzQDEcSj5PGaPQTxv48IggGegRr2iXmGiuqMzP+k8eqFu1p3NbGG0c/Gau7mA
raeJtx9sgJBOtwhBLqLffetQozW3FZuTpqY7A5G4Xc2W3H7IKD7hvhCV3gf+2RRO
qaUi2JYte3z9oZYyjhOGkrgSegs8atzaFEChb5PvAoGBAJ0sdBcfXSlxUam+FbCs
LFbkH6lg9zWfHkyWxe912ulG0yWC0qy41lZ9HvkBQRiQFeI79aFJYNXpdAdeC7iF
Ua61n44/NVsdAE+awo+InSM3nu+7ERoDLajNcjK01BmKfb9u/gL0khWhgQVpn0I6
u0CBWNuaUoZopyh3/mgY3NuHAoGAGkty84d9Awbia3a8c1pX1zuGlpS2n/mM1ff+
LiYGocOpk3iJXcL8NXzjwvjfCwxheYOJwy+S2AWWEKwGWWX7SaeJ6QcQYZFgrv1n
Ssk3suLoTPbD18P15q4nxMOQM84L0MD4bzi3KNCvYKylTBg95aR+QB+fgBxUkUp2
jWbNDRcCgYEAvAr/+HX/Wp9mIK3prULX+kcM8/o1F89viNoJ2yrsLQDwee4Jy9Lw
cpImhK01f+RxL3m/QjjDqcdQJPwr4F6HkZj4oeeOGbCgWo0oupZcQRFyNmdC8yy3
G9FQLNLAPyAJ43TyYsQrqhBy34xlYL5n5R4UzG4EcAgb0UujZiE9kBs=
-----END RSA PRIVATE KEY-----
)EOF";

WiFiClientSecure espClient;
PubSubClient client(espClient);

const int analogPin = 36;
const int totalReadings = 10;

void setup() {
  Serial.begin(9600);  // Aumentada a velocidade da serial
  while (!Serial) {
    ; // Espera a serial estar pronta
  }
  delay(1000);
  Serial.println("\n--- Iniciando setup... ---");

  // Escaneia redes WiFi disponíveis
  scanNetworks();

  // Solicita o nome e senha da rede WiFi
  String ssid = promptForInput("Digite o nome da rede WiFi: ");
  String password = promptForInput("Digite a senha da rede WiFi: ");

  Serial.println("SSID fornecido: " + ssid);
  Serial.println("Senha fornecida: " + password);

  // Conecta à rede Wi-Fi com as credenciais fornecidas
  connectToWiFi(ssid.c_str(), password.c_str());

  // Configura o cliente MQTT
  Serial.println("Configurando cliente MQTT...");
  client.setServer(mqttServer, mqttPort);
  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);
  Serial.println("Cliente MQTT configurado.");

  // Tenta conectar ao MQTT
  reconnect();

  Serial.println("--- Setup concluído ---");
}

void loop() {
  Serial.println("\n--- Iniciando loop ---");
  
  if (!client.connected()) {
    Serial.println("Cliente MQTT desconectado. Tentando reconectar...");
    reconnect();
  }
  client.loop();

  float average = readAverageMoisture();
  String moistureLevel = classifyMoistureLevel(average);

  Serial.print("Média de umidade após 10 leituras: ");
  Serial.print(average);
  Serial.println("%");
  Serial.println("Classificação: " + moistureLevel);

  // Publica os dados no MQTT
  String payload = "{\"moisture\":" + String(average) + ", \"status\":\"" + moistureLevel + "\"}";
  Serial.println("Tentando publicar no tópico 'moisture_sensor'...");
  if (client.publish("moisture_sensor", payload.c_str())) {
    Serial.println("Publicação bem-sucedida!");
    Serial.println(payload.c_str());
  } else {
    Serial.println("Falha na publicação. Estado do cliente: " + String(client.state()));
  }

  Serial.println("Aguardando 5 segundos antes do próximo ciclo...");
  delay(5000);
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

void reconnect() {
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    Serial.print("Tentativa ");
    Serial.print(attempts + 1);
    Serial.println(" de conexão ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao MQTT com sucesso!");
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
