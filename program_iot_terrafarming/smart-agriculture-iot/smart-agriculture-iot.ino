#include <WiFi.h>               
#include <WiFiClientSecure.h>    
#include <PubSubClient.h>        
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>
// #include "certificates.h"

// Definindo os pinos dos sensores
#define DHTPIN 18               // Pino do sensor DHT para umidade e temperatura do ar
#define DHTTYPE DHT22           // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 36    // Pino do sensor de umidade do solo
#define LUMINOSITY_PIN 39       // Pino do sensor de luminosidade
#define DS18B20_PIN 25           // Pino do sensor de temperatura DS18B20 S2 (ajuste conforme necessário)

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);           
DallasTemperature soilTempSensor(&oneWire); 

// Definindo as credenciais do AWS IoT
const char* mqttEndpoint = "aejwurestbom1-ats.iot.us-east-1.amazonaws.com";

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
MIIDWTCCAkGgAwIBAgIUR33cZJ5BIJQOywfUZRhduRDZHvcwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MTAxODE2MjMz
M1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM/c0Dj8dapsda/Jm1eI
2ZF8OjYLFgkfSvXEIv5sNkSpu+rE7hQF2BTbnqzodVcR6ndq9o/CSppTH+u/iwzv
hs0ViWcacUZpfcOdtsEFwCKy28ImZT4n6JePxbXUWZZtVEDEG1v5ai3LBiTlPPGB
rCds4t2TJ2pXKqWu2KVRO4ALLmiIHIPpa9rCr0xDoq3+g7PT1AecNYfLV7EsXrO7
5F743gphzjyDDyr9+Zl5hClcIXALJixUt9AQdBENY/wvdHpN5Z+19HAHjd2Ckuq0
Fh7tT+ED3cJVRAcjbSTTZlpxJ6JxALH7JKeTN/tsYalCyWGjzm4CmLgwxxMce8I9
idsCAwEAAaNgMF4wHwYDVR0jBBgwFoAUym2rFHDRmUM9Sr/uSvgSaBcACS8wHQYD
VR0OBBYEFO+nCZ3SueQXmiEUGmMtkplRt0fNMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQC9aG5jI2K2XiSKvi4tA2X42fsY
yEj2/LJVZ/GI4zHgipjqyn4280Zw8FLlodJ3A9BIfFammFbjcdt/qKG9mmTlY5s0
O9eInTQzZfyIrvnuHa5zQ3sfEF19nolx4Yo6JJr9+4iT3ZB3wg6IlpYkrt8DDAoG
FDtdNE5ZdoTB6llfjXtLZei+xGQ2ZLvJj4QSm6QqSeG1Tsdip3osyusKQ9EsuN0s
V2KdMELwJTO/aQenIzAuDnGW1hOt2dN10tIuL1R5bnCyrX8DwZiVR2+SQVrR6Aky
0/lXhrSOQaLg5XJCgmpdbEznxBhxu95ZjGBH9pgMCaSdSE6uJaL95Ggl7tBK
-----END CERTIFICATE-----
)EOF";

const char* privateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAz9zQOPx1qmx1r8mbV4jZkXw6NgsWCR9K9cQi/mw2RKm76sTu
FAXYFNuerOh1VxHqd2r2j8JKmlMf67+LDO+GzRWJZxpxRml9w522wQXAIrLbwiZl
Pifol4/FtdRZlm1UQMQbW/lqLcsGJOU88YGsJ2zi3ZMnalcqpa7YpVE7gAsuaIgc
g+lr2sKvTEOirf6Ds9PUB5w1h8tXsSxes7vkXvjeCmHOPIMPKv35mXmEKVwhcAsm
LFS30BB0EQ1j/C90ek3ln7X0cAeN3YKS6rQWHu1P4QPdwlVEByNtJNNmWnEnonEA
sfskp5M3+2xhqULJYaPObgKYuDDHExx7wj2J2wIDAQABAoIBAFYqQ8qLpL8rzLE9
En77xKzRYVQLzmujpDAyyQrMksZt0e8lCUgVkBg9Xg5xIksgqyArn9/B+6jzclUI
hryrAic7mUS7Kl+01SRk2WA0YQxBNmXKAsf8RSemup+AUk7QLU/XuzuqLYCkG3zp
5hR624FQWs7c9EbZsV0TGM2W2eJegHX7jVI3DD4ghHPsRwILHg3Ucz83GizBJW/L
I4B52uR/QxNE1I4aX0DQNBBSU0yUg+1HDFWPQL66YrOOPWB5UzjmTPI3pFf1isAC
CSL3Bf8dsfe+nKTM3nB2WdC1pU3Rkf7GUaKoY6BEZF9QVXedOxbjI7k5yc9+UWWn
9AyTMmkCgYEA9830nzClylS24ZP4jTAoz/6IjpKv8g5nUM1zpf7/TwvXnXtZsp3j
l/ZThRR2jdTA40F6rYz/pWSlsovXYWeQ8/F1Hiy32xsD9jHd3IQHXJf3B+bHSRU6
sTw4FO3jD2e5P2VSUW3BWpFz5T9CXBCB/8dDel+NdJ+ovAgFW91ZGq8CgYEA1ryw
H7BrHoe09DWiP8oMZK+M8NvBRXHqRJpFkTeON7rvMz+Hb4/lKACDePjIA6fhhpd5
GIwTP4AXHJFeEZgqQqjj4Y5VSVjXRyTiYGVCfOp26DIq0uTH2EuHclg1Md91Y/2H
d3s89lweITzcCZyI3RbOtLdYP8dNYBaCCJXnnpUCgYB8iRzvA9vOG1TteRfonNNl
9F1ciYuy8lop2ZbNTaGxcBokIuGpSoAe1sSSlP4fuVRW4YltvvabgEFlwbG0WgAX
GLnrOD4N9z2+dMEzGYc5mYWkiu6MZAbjG4hzvDnofBA1NA5yrd4GTiMYivommoU6
rkHTNkI44iRCmyVWTZ+CMQKBgGDmyPuj2tLuHmRNh6gNf0Y4SfuuzyqNW1AV5erA
DTds7eBMfMuFPb2tbaa7bVbo/UaFOCoxm8X+AW/s0WxTJE7sc9knJ6lvo8YBCP7C
8xv3mizx5o1AnEYo3zhkQaz9z7WNhQIP5NSvgREyq4DS2JgcYK8ARZySTYJc5dUG
AH15AoGBAPPrc1fT6Zv8c5HRaFbRcMdYd9Z2+C/IXA6JCBQiQea8ymbE+rBxwms7
1Aj20r3iwNdam7vmZeekS71IZpOhHKVkET8BGwjgpO3huJGI3fVSd1pvZI75nUGv
n/Q4zYgvgiTlWl7yDTGou5frubt7IIGOGDpgd4tP/9NpeHUfUMrx
-----END RSA PRIVATE KEY-----
)EOF";

// Variáveis de WiFi e MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);
String ssid, password;

// BLE Configuration
#define SERVICE_UUID "4fafc201-1fb5-459e-8d40-b6b0e6e9b6b2"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer;
BLECharacteristic *pCharacteristic;

// Declaração de funções
void listWiFiNetworks();
void connectWiFi();
void reconnectMQTT();
void sendToMQTT(String topic, String payload);
float calibrateSoilMoisture(int rawValue);
void bleSetup();

// Declaração da classe MyServerCallbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String json = pCharacteristic->getValue().c_str();
    Serial.println("Recebido JSON: " + json);

    // Parse JSON para obter credenciais
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (!error) {
      ssid = doc["ssid"].as<String>();
      password = doc["password"].as<String>();
      Serial.println("Credenciais WiFi recebidas:");
      Serial.println("SSID: " + ssid);
      Serial.println("Senha: " + password);
    } else {
      Serial.println("Falha ao analisar JSON");
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000); 

  dht.begin();  
  soilTempSensor.begin();

  bleSetup(); // Inicia o BLE
  Serial.println("BLE configurado, aguardando credenciais do WiFi...");

  while (ssid.isEmpty() || password.isEmpty()) {
    delay(1000); // Aguarda receber as credenciais
  }

  listWiFiNetworks();
  connectWiFi();

  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);
  client.setServer(mqttEndpoint, 8883); 
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }

  while (true) {
    float soilMoistureAvg = 0, soilTempAvg = 0, airMoistureAvg = 0, airTempAvg = 0, brightnessAvg = 0;

    for (int i = 0; i < 10; i++) {
      float soilMoisture = calibrateSoilMoisture(analogRead(SOIL_MOISTURE_PIN));
      float airHumidity = dht.readHumidity();
      float airTemp = dht.readTemperature(); 
      float luminosity = map(analogRead(LUMINOSITY_PIN), 0, 4095, 0, 100000); 
      
      soilTempSensor.requestTemperatures();
      float soilTemp = soilTempSensor.getTempCByIndex(0);

      // Substituir por zero se não houver leitura válida
      soilMoisture = isnan(soilMoisture) ? 0 : soilMoisture;
      soilTemp = (soilTemp == DEVICE_DISCONNECTED_C || isnan(soilTemp)) ? 0 : soilTemp;
      airHumidity = isnan(airHumidity) ? 0 : airHumidity;
      airTemp = isnan(airTemp) ? 0 : airTemp;
      luminosity = isnan(luminosity) ? 0 : luminosity;

      // Impressão das leituras em colunas separadas por '|'
      Serial.print("Leitura ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print("Umidade do Solo: " + String(soilMoisture) + "% | ");
      Serial.print("Temperatura do Solo: " + String(soilTemp) + "°C | ");
      Serial.print("Luminosidade: " + String(luminosity) + " Lux | ");
      Serial.print("Umidade do Ar: " + String(airHumidity) + "% | ");
      Serial.println("Temperatura do Ar: " + String(airTemp) + "°C");
      Serial.println("------------------------------");

      soilMoistureAvg += soilMoisture;
      soilTempAvg += soilTemp;
      airMoistureAvg += airHumidity;
      airTempAvg += airTemp;
      brightnessAvg += luminosity;

      delay(2000);
    }

    // Cálculo das médias
    soilMoistureAvg /= 10;
    soilTempAvg /= 10;
    airMoistureAvg /= 10;
    airTempAvg /= 10;
    brightnessAvg /= 10;

    // Envio das médias para o MQTT
    sendToMQTT("agriculture/soil/moisture", "{\"moisture\": " + String(soilMoistureAvg) + ", \"status\": \"Normal\", \"crops\": [\"Milho\", \"Soja\"]}");
    sendToMQTT("agriculture/soil/temperature", "{\"temperature\": " + String(soilTempAvg) + ", \"status\": \"Normal\", \"crops\": [\"Milho\", \"Arroz\"]}");
    sendToMQTT("agriculture/brightness", "{\"brightness\": " + String(brightnessAvg) + ", \"status\": \"Suficiente\", \"crops\": [\"Milho\", \"Girassol\"]}");
    sendToMQTT("agriculture/air/moisture", "{\"moisture\": " + String(airMoistureAvg) + ", \"status\": \"Alta\", \"crops\": [\"Café\", \"Tomate\"]}");
    sendToMQTT("agriculture/air/temperature", "{\"temperature\": " + String(airTempAvg) + ", \"status\": \"Alta\", \"crops\": [\"Soja\", \"Batata\"]}");

    client.loop();

    // Impressão das médias finais
    Serial.println("Médias finais:");
    Serial.print("Umidade do Solo: " + String(soilMoistureAvg) + "% | ");
    Serial.print("Temperatura do Solo: " + String(soilTempAvg) + "°C | ");
    Serial.print("Luminosidade: " + String(brightnessAvg) + " Lux | ");
    Serial.print("Umidade do Ar: " + String(airMoistureAvg) + "% | ");
    Serial.println("Temperatura do Ar: " + String(airTempAvg) + "°C");
    Serial.println("------------------------------");

    Serial.println("Deseja realizar uma nova medição agora ou esperar 2 horas?");
    Serial.println("Digite 'N' para nova medição ou 'E' para esperar 2 horas:");

    while (Serial.available() == 0) {} 
    String resposta = Serial.readString();
    resposta.trim();

    if (resposta == "E" || resposta == "e") {
      Serial.println("Esperando por 2 horas...");
      delay(7200000);
    } else if (resposta == "N" || resposta == "n") {
      Serial.println("Nova medição será feita agora.");
    } else {
      Serial.println("Comando não reconhecido. Nova medição será feita.");
    }
  }
}

void bleSetup() {
  BLEDevice::init("TerraFarming");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );

  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void listWiFiNetworks() {
  Serial.println("Procurando redes WiFi...");
  int n = WiFi.scanNetworks();
  Serial.println("Redes WiFi encontradas: " + String(n));
  for (int i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (RSSI: ");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm)");
    delay(10);
  }
}

void connectWiFi() {
  Serial.print("Conectando ao WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado ao WiFi com sucesso!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
    } else {
      Serial.print("falha com erro: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void sendToMQTT(String topic, String payload) {
  if (client.publish(topic.c_str(), payload.c_str())) {
    Serial.println("Mensagem enviada: " + payload);
  } else {
    Serial.println("Falha ao enviar mensagem");
  }
}

float calibrateSoilMoisture(int rawValue) {
  int dry = 700;
  int wet = 3200;
  return map(rawValue, dry, wet, 0, 100);
}
