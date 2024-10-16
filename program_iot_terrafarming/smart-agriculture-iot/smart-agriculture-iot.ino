#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <UUID.h>
#include <EEPROM.h>
#include "BluetoothSerial.h"
#include "certificates.h"

#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_defs.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

const char PROGMEM MQTT_SERVER[] = "ahbccp4thrap8-ats.iot.us-east-1.amazonaws.com";
const uint16_t MQTT_PORT = 8883;

WiFiClientSecure espClient;
PubSubClient client(espClient);

const char* BLUETOOTH_NAME = "TerraFarming";  // Alterado para corresponder à imagem

const uint8_t SOIL_MOISTURE_PIN = 36;
const uint8_t LIGHT_SENSOR_PIN = 39;
const uint8_t TOTAL_READINGS = 10;
constexpr unsigned long READING_INTERVAL = 2UL * 60 * 60 * 1000;

const uint16_t WET = 2550;
const uint16_t DRY = 650;

#define DHTPIN 21
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

UUID uuid;
String deviceUUID;

struct WiFiCredentials {
  char ssid[32];
  char password[64];
} storedCredentials;

void sendLogMessage(const String& message) {
  Serial.println(message);
  if (SerialBT.hasClient()) {
    SerialBT.println(message);
  }
}

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Dispositivo Bluetooth conectado");
  }
  if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("Dispositivo Bluetooth desconectado");
  }
  if (event == ESP_SPP_START_EVT) {
    Serial.println("Serviço Bluetooth iniciado");
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Aguarda a conexão serial
  }
  delay(1000);
  sendLogMessage(F("Setup: Iniciando..."));

  EEPROM.begin(sizeof(WiFiCredentials));
  pinMode(0, INPUT_PULLUP);

  if (!initBluetooth()) {
    sendLogMessage(F("Falha ao iniciar Bluetooth. Reiniciando..."));
    delay(5000);
    ESP.restart();
  }

  uuid.generate();
  deviceUUID = uuid.toCharArray();
  sendLogMessage("UUID gerado: " + deviceUUID);
  sendLogMessage(F("Aguardando UUID do frontend..."));

  if (!waitForUUIDVerification(300000)) { // 5 minutos de timeout
    sendLogMessage(F("Timeout na verificação do UUID. Reiniciando..."));
    ESP.restart();
  }

  sendLogMessage(F("Aguardando configuração do Wi-Fi..."));
  if (!waitForWiFiCredentials(300000)) { // 5 minutos de timeout
    sendLogMessage(F("Timeout na configuração do Wi-Fi. Reiniciando..."));
    ESP.restart();
  }

  if (!connectToWiFi(storedCredentials.ssid, storedCredentials.password)) {
    ESP.restart();
  }

  client.setServer(MQTT_SERVER, MQTT_PORT);
  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);

  dht.begin();
  pinMode(LIGHT_SENSOR_PIN, INPUT);

  sendLogMessage(F("Setup concluído. Iniciando funções principais."));
}

bool initBluetooth() {
  Serial.println("Iniciando Bluetooth...");
  
  if (!btStart()) {
    Serial.println("Falha ao iniciar Bluetooth");
    return false;
  }
  Serial.println("Bluetooth core iniciado");
  
  if (esp_bluedroid_init() != ESP_OK) {
    Serial.println("Falha ao inicializar Bluedroid");
    return false;
  }
  Serial.println("Bluedroid inicializado");
  
  if (esp_bluedroid_enable() != ESP_OK) {
    Serial.println("Falha ao habilitar Bluedroid");
    return false;
  }
  Serial.println("Bluedroid habilitado");
  
  if (!SerialBT.begin(BLUETOOTH_NAME)) {
    Serial.println("Falha ao iniciar BluetoothSerial");
    return false;
  }
  
  Serial.println("Bluetooth iniciado com sucesso");
  Serial.print("Nome do dispositivo Bluetooth: ");
  Serial.println(BLUETOOTH_NAME);
  
  // Registrar o callback
  SerialBT.register_callback(btCallback);
  
  // Imprimir o endereço Bluetooth
  const uint8_t* btAddr = esp_bt_dev_get_address();
  char btAddrStr[18];
  sprintf(btAddrStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          btAddr[0], btAddr[1], btAddr[2], btAddr[3], btAddr[4], btAddr[5]);
  Serial.print("Endereço Bluetooth: ");
  Serial.println(btAddrStr);
  
  return true;
}

bool waitForUUIDVerification(unsigned long timeout) {
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (verifyUUID()) {
      return true;
    }
    delay(100);
  }
  return false;
}

bool waitForWiFiCredentials(unsigned long timeout) {
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (receiveWiFiCredentials()) {
      return true;
    }
    delay(1000);
  }
  return false;
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  if (WiFi.status() != WL_CONNECTED && !connectToWiFi(storedCredentials.ssid, storedCredentials.password)) {
    ESP.restart();
  }

  delay(1000);  // Dê um tempo para o sistema inicializar
  if (!initBluetooth()) {
    sendLogMessage(F("Falha ao iniciar Bluetooth. Reiniciando..."));
    delay(5000);
    ESP.restart();
  }

  static unsigned long lastBluetoothCheck = 0;
  if (millis() - lastBluetoothCheck > 30000) { // Verifica a cada 30 segundos
    lastBluetoothCheck = millis();
    checkBluetoothStatus();
  }

  performReadings();
  delay(READING_INTERVAL);
}

void checkBluetoothStatus() {
  if (!SerialBT.hasClient()) {
    Serial.println("Bluetooth não está conectado");
    Serial.println("Tentando reiniciar o Bluetooth...");
    SerialBT.end();
    delay(1000);
    if (initBluetooth()) {
      Serial.println("Bluetooth reiniciado com sucesso");
    } else {
      Serial.println("Falha ao reiniciar Bluetooth");
    }
  } else {
    Serial.println("Bluetooth está conectado");
  }
}

bool verifyUUID() {
  if (SerialBT.available()) {
    String inputUUID = SerialBT.readStringUntil('\n');
    inputUUID.trim();
    
    if (inputUUID == deviceUUID) {
      sendLogMessage(F("UUID verificado com sucesso"));
      return true;
    }
    sendLogMessage(F("UUID incorreto. Aguardando novo UUID."));
  }
  return false;
}

bool receiveWiFiCredentials() {
  if (SerialBT.available()) {
    String receivedData = SerialBT.readStringUntil('\n');
    receivedData.trim();
    
    int separatorIndex = receivedData.indexOf(',');
    if (separatorIndex != -1) {
      String ssid = receivedData.substring(0, separatorIndex);
      String password = receivedData.substring(separatorIndex + 1);
      
      strncpy(storedCredentials.ssid, ssid.c_str(), sizeof(storedCredentials.ssid) - 1);
      strncpy(storedCredentials.password, password.c_str(), sizeof(storedCredentials.password) - 1);
      storedCredentials.ssid[sizeof(storedCredentials.ssid) - 1] = '\0';
      storedCredentials.password[sizeof(storedCredentials.password) - 1] = '\0';
      EEPROM.put(0, storedCredentials);
      EEPROM.commit();
      
      sendLogMessage(F("Credenciais Wi-Fi recebidas e armazenadas"));
      return true;
    }
  }
  return false;
}

bool connectToWiFi(const char* ssid, const char* password) {
  sendLogMessage("Conectando a " + String(ssid));
  WiFi.begin(ssid, password);
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    sendLogMessage("Wi-Fi configurado com sucesso. IP: " + WiFi.localIP().toString());
    return true;
  }
  sendLogMessage(F("Falha na conexão Wi-Fi"));
  return false;
}

void reconnectMQTT() {
  uint8_t attempts = 0;
  while (!client.connected() && attempts < 5) {
    if (client.connect("ESP32Client")) return;
    delay(5000);
    attempts++;
  }
  ESP.restart();
}

float mapMoisture(int sensorValue) {
  float percentage = map(sensorValue, DRY, WET, 0, 100);
  return constrain(percentage, 0, 100);
}

String classifyLevel(float value, const char* levels[], const float thresholds[], uint8_t size) {
  for (uint8_t i = 0; i < size - 1; i++) {
    if (value < thresholds[i]) return levels[i];
  }
  return levels[size - 1];
}

void performReadings() {
  float averages[4] = {0}; // moisture, temperature, humidity, light
  const char* moistureLevels[] = {"muito seco", "seco", "normal", "úmido", "muito úmido"};
  const float moistureThresholds[] = {20, 40, 60, 80};
  const char* tempLevels[] = {"baixo", "normal", "alto"};
  const float tempThresholds[] = {15, 30};
  const char* humidityLevels[] = {"baixo", "normal", "alto"};
  const float humidityThresholds[] = {30, 70};
  const char* lightLevels[] = {"muito baixo", "baixo", "médio", "alto", "muito alto"};
  const float lightThresholds[] = {20, 40, 60, 80};

  for (int i = 0; i < TOTAL_READINGS; i++) {
    averages[0] += mapMoisture(analogRead(SOIL_MOISTURE_PIN));
    averages[1] += dht.readTemperature();
    averages[2] += dht.readHumidity();
    averages[3] += map(analogRead(LIGHT_SENSOR_PIN), 0, 4095, 0, 100);
    delay(2000);
  }

  for (int i = 0; i < 4; i++) averages[i] /= TOTAL_READINGS;

  publishMQTTData("agriculture/soil/moisture", averages[0], classifyLevel(averages[0], moistureLevels, moistureThresholds, 5));
  publishMQTTData("agriculture/soil/temperature", averages[1], classifyLevel(averages[1], tempLevels, tempThresholds, 3));
  publishMQTTData("agriculture/air/moisture", averages[2], classifyLevel(averages[2], humidityLevels, humidityThresholds, 3));
  publishMQTTData("agriculture/brightness", averages[3], classifyLevel(averages[3], lightLevels, lightThresholds, 5));
}

void publishMQTTData(const char* topic, float value, String status) {
  String payload = "{\"value\":" + String(value) + ",\"status\":\"" + status + "\",\"timestamp\":\"" + String(millis()) + "\",\"uuid\":\"" + deviceUUID + "\"}";
  if (!client.publish(topic, payload.c_str())) {
    sendLogMessage(F("Falha na publicação MQTT"));
  }
}
