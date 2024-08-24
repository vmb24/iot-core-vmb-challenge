# iot-core-vmb-challenge
VMB IOT smart agriculture solution for challenge using c++ for connecting with MQTT broker in AWS IOT Core for service other software components.

Componentes Necessários
    ESP32: O microcontrolador principal.
    Módulo de Reconhecimento de Voz V3: Para reconhecimento de comandos de voz.
    Módulo de Síntese de Voz TTS: Para gerar respostas em áudio.
    Câmera ESP32-CAM: Para capturar imagens.
    Sensores e Atuadores: Sensores de ultrassom, motores DC, servo motor, sensor de pH do solo, etc.
    Alto-falante: Para emitir a voz de resposta.
    Conectividade WiFi: Para comunicação com AWS IoT Core.
    Passos para Integração
    Configuração do Módulo de Reconhecimento de Voz V3

    Conecte o módulo V3 ao ESP32 via comunicação serial.
    Grave os comandos de voz no módulo V3.
    Configuração do Módulo TTS

    Conecte o módulo TTS ao ESP32 via comunicação serial.
    Configure as mensagens de resposta.
    Configuração da Câmera ESP32-CAM

    Conecte a câmera ao ESP32.
    Configure a captura de imagens e o processamento das mesmas.
    Integração com AWS IoT Core

    Envie dados de pH e outros dados relevantes para o AWS IoT Core.
    Conexões
    Módulo V3 ao ESP32:

    VCC do módulo V3 ao 3.3V do ESP32.
    GND do módulo V3 ao GND do ESP32.
    TX do módulo V3 ao RX do ESP32.
    RX do módulo V3 ao TX do ESP32.
    Módulo TTS ao ESP32:

    VCC do módulo TTS ao 3.3V do ESP32.
    GND do módulo TTS ao GND do ESP32.
    TX do módulo TTS ao RX2 do ESP32.
    RX do módulo TTS ao TX2 do ESP32.
    Câmera ESP32-CAM ao ESP32:

    VCC da câmera ao 5V do ESP32.
    GND da câmera ao GND do ESP32.
    U0R da câmera ao U0T do ESP32.
    U0T da câmera ao U0R do ESP32.
    GPIO 0 ao GND para modo de upload de firmware (remova após upload).

Código do ESP32
    Aqui está um exemplo de como integrar o módulo V3, o módulo TTS, e a câmera ESP32-CAM com o ESP32, adicionando a funcionalidade de reconhecimento de solo agrícola e frutas saudáveis:

    #include <Arduino.h>
    #include <WiFi.h>
    #include <PubSubClient.h>
    #include <Servo.h>
    #include "esp_camera.h"
    #include "certs.h" // Arquivo onde estão armazenadas as credenciais

    // Definições dos pinos dos sensores de ultrassom
    #define TRIGGER_PIN_LEFT 12
    #define ECHO_PIN_LEFT 14
    #define TRIGGER_PIN_RIGHT 27
    #define ECHO_PIN_RIGHT 26

    // Definições dos pinos do driver do motor
    #define MOTOR_LEFT_FORWARD 5
    #define MOTOR_LEFT_BACKWARD 18
    #define MOTOR_RIGHT_FORWARD 19
    #define MOTOR_RIGHT_BACKWARD 21

    // Definição do pino do sensor de pH do solo
    #define PH_SENSOR_PIN 34  // Pino analógico

    // Definição do pino do servo motor
    #define SERVO_PIN 25

    // Definições para conexão WiFi e MQTT
    const char* ssid = "your_SSID";
    const char* password = "your_PASSWORD";
    const char* aws_endpoint = "your-aws-endpoint.iot.your-region.amazonaws.com";
    const char* aws_topic = "phReadings";

    WiFiClientSecure net;
    PubSubClient client(net);
    Servo armServo;

    // Configuração do módulo de reconhecimento de voz V3
    #define RECOGNITION_RX 16
    #define RECOGNITION_TX 17
    HardwareSerial voiceSerial(2);

    // Configuração do módulo de síntese de voz TTS
    #define TTS_RX 4
    #define TTS_TX 15
    HardwareSerial ttsSerial(1);

    // Configuração da câmera ESP32-CAM
    #define PWDN_GPIO_NUM     -1
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM      21
    #define SIOD_GPIO_NUM      26
    #define SIOC_GPIO_NUM      27

    #define Y9_GPIO_NUM       35
    #define Y8_GPIO_NUM       34
    #define Y7_GPIO_NUM       39
    #define Y6_GPIO_NUM       36
    #define Y5_GPIO_NUM       19
    #define Y4_GPIO_NUM       18
    #define Y3_GPIO_NUM        5
    #define Y2_GPIO_NUM        4
    #define VSYNC_GPIO_NUM    25
    #define HREF_GPIO_NUM     23
    #define PCLK_GPIO_NUM     22

    camera_config_t config;

    // Funções para controle do robô
    void setupMotors();
    void setupServo();
    void setupMicrophone();
    void moveForward();
    void moveBackward();
    void turnLeft();
    void turnRight();
    void stopMotors();
    void checkSoilPH();
    String readVoiceCommand();
    void executeCommand(String command);
    void setupWiFi();
    void setupMQTT();
    void setupVoiceRecognition();
    void setupTTS();
    void setupCamera();
    void speak(const char* message);
    bool detectSoil();
    bool detectFruitOrVegetable();

    void setup() {
    Serial.begin(115200);
    setupWiFi();
    setupMQTT();
    setupMotors();
    setupServo();
    setupVoiceRecognition();
    setupTTS();
    setupCamera();
    }

    void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        // Capturar e processar comando de voz
        String command = readVoiceCommand();
        if (command != "") {
        executeCommand(command);
        }
    }

    client.loop();
    }

    void setupWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    }

    void setupMQTT() {
    net.setCACert(ca_cert);
    net.setCertificate(client_cert);
    net.setPrivateKey(private_key);
    client.setServer(aws_endpoint, 8883);

    while (!client.connected()) {
        Serial.println("Connecting to AWS IoT Core...");
        if (client.connect("ESP32Client")) {
        Serial.println("Connected to AWS IoT Core");
        } else {
        Serial.print("Failed to connect, rc=");
        Serial.print(client.state());
        delay(2000);
        }
    }
    }

    void setupMotors() {
    pinMode(MOTOR_LEFT_FORWARD, OUTPUT);
    pinMode(MOTOR_LEFT_BACKWARD, OUTPUT);
    pinMode(MOTOR_RIGHT_FORWARD, OUTPUT);
    pinMode(MOTOR_RIGHT_BACKWARD, OUTPUT);
    }

    void setupServo() {
    armServo.attach(SERVO_PIN);
    armServo.write(90);  // Posição inicial do braço
    }

    void setupVoiceRecognition() {
    voiceSerial.begin(9600, SERIAL_8N1, RECOGNITION_RX, RECOGNITION_TX);
    Serial.println("Voice Recognition Module Initialized");
    }

    void setupTTS() {
    ttsSerial.begin(9600, SERIAL_8N1, TTS_RX, TTS_TX);
    Serial.println("TTS Module Initialized");
    }

    void setupCamera() {
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    Serial.println("Camera Module Initialized");
    }

    String readVoiceCommand() {
    if (voiceSerial.available()) {
        String command = voiceSerial.readStringUntil('\n');
        Serial.print("Voice Command Received: ");
        Serial.println(command);
        return command;
    }
    return "";
    }

    void speak(const char* message) {
        ttsSerial.print(message);
        Serial.print("Speaking: ");
        Serial.println(message);
    }

    void executeCommand(String command) {
        if (command == "forward") {
            moveForward();
            speak("Moving forward");
        } else if (command == "backward") {
            moveBackward();
            speak("Moving backward");
        } else if (command == "left") {
            turnLeft();
            speak("Turning left");
        } else if (command == "right") {
            turnRight();
            speak("Turning right");
        } else if (command == "stop") {
            stopMotors();
            speak("Stopping");
        } else if (command == "Retire métricas do pH do solo") {
            if (detectSoil()) {
                checkSoilPH();
                speak("Medição de pH do solo concluída");
            } else {
                speak("Não encontrei um solo com terra");
            }
        } else if (command == "Recolha frutas ou vegetais maduros") {
            if (detectSoil()) {
                if (detectFruitOrVegetable()) {
                    // Código para colher frutas ou vegetais
                    speak("Frutas ou vegetais maduros encontrados e recolhidos");
                } else {
                    speak("Nenhuma fruta ou vegetal maduro encontrado");
                }
            } else {
                speak("Não encontrei um solo com terra");
            }
        } else {
            speak("Comando não reconhecido");
       }
    }

    void moveForward() {
        digitalWrite(MOTOR_LEFT_FORWARD, HIGH);
        digitalWrite(MOTOR_LEFT_BACKWARD, LOW);
        digitalWrite(MOTOR_RIGHT_FORWARD, HIGH);
        digitalWrite(MOTOR_RIGHT_BACKWARD, LOW);
    }

    void moveBackward() {
        digitalWrite(MOTOR_LEFT_FORWARD, LOW);
        digitalWrite(MOTOR_LEFT_BACKWARD, HIGH);
        digitalWrite(MOTOR_RIGHT_FORWARD, LOW);
        digitalWrite(MOTOR_RIGHT_BACKWARD, HIGH);
    }

    void turnLeft() {
        digitalWrite(MOTOR_LEFT_FORWARD, LOW);
        digitalWrite(MOTOR_LEFT_BACKWARD, HIGH);
        digitalWrite(MOTOR_RIGHT_FORWARD, HIGH);
        digitalWrite(MOTOR_RIGHT_BACKWARD, LOW);
    }

    void turnRight() {
        digitalWrite(MOTOR_LEFT_FORWARD, HIGH);
        digitalWrite(MOTOR_LEFT_BACKWARD, LOW);
        digitalWrite(MOTOR_RIGHT_FORWARD, LOW);
        digitalWrite(MOTOR_RIGHT_BACKWARD, HIGH);
    }

    void stopMotors() {
        digitalWrite(MOTOR_LEFT_FORWARD, LOW);
        digitalWrite(MOTOR_LEFT_BACKWARD, LOW);
        digitalWrite(MOTOR_RIGHT_FORWARD, LOW);
        digitalWrite(MOTOR_RIGHT_BACKWARD, LOW);
    }

    void checkSoilPH() {
        armServo.write(0);
        delay(2000);
        int sensorValue = analogRead(PH_SENSOR_PIN);
        float voltage = sensorValue * (3.3 / 4095.0);
        float phValue = (voltage - 0.5) * 3.5;
        Serial.print("Soil pH: ");
        Serial.println(phValue);
    
        if (client.connected()) {
            String payload = "{\"phValue\": " + String(phValue) + "}";
            client.publish(aws_topic, payload.c_str());
            Serial.println("Data sent to AWS IoT Core");
        }

        armServo.write(90);
        delay(2000);
    }

    bool detectSoil() {
        // Função para capturar e analisar a imagem para detectar solo
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            return false;
        }

        // Código de processamento de imagem para detectar solo
        // Suponha que a função retorna true se o solo é detectado, caso contrário, false
        // Este é um exemplo de como você pode implementar o processamento de imagem
        bool soilDetected = true;  // Placeholder para o resultado da detecção de solo

        esp_camera_fb_return(fb);
            return soilDetected;
        }

        bool detectFruitOrVegetable() {
            // Função para capturar e analisar a imagem para detectar frutas ou vegetais maduros
            camera_fb_t * fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
                return false;
        }

        // Código de processamento de imagem para detectar frutas ou vegetais maduros
        // Suponha que a função retorna true se frutas ou vegetais maduros são detectados, caso contrário, false
        // Este é um exemplo de como você pode implementar o processamento de imagem
        bool fruitDetected = true;  // Placeholder para o resultado da detecção de frutas

        esp_camera_fb_return(fb);
        return fruitDetected;
    }
