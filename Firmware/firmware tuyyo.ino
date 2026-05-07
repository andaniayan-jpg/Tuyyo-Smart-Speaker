#include <WiFi.h>
#include "driver/i2s.h"
#include <math.h>

const char* WIFI_SSID = "wifi name";
const char* WIFI_PASSWORD = "wifi password";

const char* AI_SERVER_IP = "192.168.1.10";
const int AI_SERVER_PORT = 5000;

// APA-106 / status LED data pin
#define STATUS_LED_PIN 12

// Push buttons from schematic
#define BUTTON_1 34
#define BUTTON_2 35
#define BUTTON_3 36

// Shared I2S clock pins from schematic
#define I2S_BCLK 4
#define I2S_LRCLK 5

// ICS-43434 microphone data pin
#define MIC_DATA_IN 6

// MAX98357A amplifier data pin
#define SPEAKER_DATA_OUT 7

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 512

#define I2S_PORT I2S_NUM_0

void blinkStatusPin(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(delayMs);
  }
}

void connectWiFi() {
  Serial.println();
  Serial.println("Connecting to Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempt = 0;

  while (WiFi.status() != WL_CONNECTED && attempt < 30) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    blinkStatusPin(3, 150);
  } else {
    Serial.println("Wi-Fi not connected.");
    Serial.println("Continuing in offline hardware test mode.");
    blinkStatusPin(5, 100);
  }
}

void setupI2SAudio() {
  i2s_config_t i2sConfig = {};
  i2sConfig.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
  i2sConfig.sample_rate = SAMPLE_RATE;
  i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  i2sConfig.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2sConfig.communication_format = I2S_COMM_FORMAT_I2S;
  i2sConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2sConfig.dma_buf_count = 4;
  i2sConfig.dma_buf_len = 256;
  i2sConfig.use_apll = false;
  i2sConfig.tx_desc_auto_clear = true;
  i2sConfig.fixed_mclk = 0;

  i2s_pin_config_t i2sPins = {};
  i2sPins.bck_io_num = I2S_BCLK;
  i2sPins.ws_io_num = I2S_LRCLK;
  i2sPins.data_out_num = SPEAKER_DATA_OUT;
  i2sPins.data_in_num = MIC_DATA_IN;

  esp_err_t installResult = i2s_driver_install(I2S_PORT, &i2sConfig, 0, NULL);

  if (installResult != ESP_OK) {
    Serial.print("I2S driver install failed. Error code: ");
    Serial.println(installResult);
    return;
  }

  esp_err_t pinResult = i2s_set_pin(I2S_PORT, &i2sPins);

  if (pinResult != ESP_OK) {
    Serial.print("I2S pin setup failed. Error code: ");
    Serial.println(pinResult);
    return;
  }

  i2s_zero_dma_buffer(I2S_PORT);

  Serial.println("I2S audio initialized.");
  Serial.println("Microphone: ICS-43434");
  Serial.println("Amplifier: MAX98357A");
}

void playTestTone() {
  Serial.println("Playing speaker test tone...");

  int frequency = 440;
  int durationMs = 700;
  int totalSamples = (SAMPLE_RATE * durationMs) / 1000;

  size_t bytesWritten;

  for (int i = 0; i < totalSamples; i++) {
    float t = (float)i / SAMPLE_RATE;
    int32_t sample = (int32_t)(60000000 * sin(2.0 * PI * frequency * t));

    i2s_write(I2S_PORT, &sample, sizeof(sample), &bytesWritten, portMAX_DELAY);
  }

  Serial.println("Speaker test tone finished.");
}

int readMicLevel() {
  int32_t buffer[BUFFER_SIZE];
  size_t bytesRead = 0;

  esp_err_t result = i2s_read(
    I2S_PORT,
    buffer,
    sizeof(buffer),
    &bytesRead,
    100 / portTICK_PERIOD_MS
  );

  if (result != ESP_OK || bytesRead == 0) {
    return 0;
  }

  int samplesRead = bytesRead / sizeof(int32_t);
  long total = 0;

  for (int i = 0; i < samplesRead; i++) {
    int32_t sample = buffer[i] >> 14;
    total += abs(sample);
  }

  return total / samplesRead;
}

void readButtons() {
  int button1State = digitalRead(BUTTON_1);
  int button2State = digitalRead(BUTTON_2);
  int button3State = digitalRead(BUTTON_3);

  if (button1State == LOW) {
    Serial.println("Button 1 pressed");
  }

  if (button2State == LOW) {
    Serial.println("Button 2 pressed");
  }

  if (button3State == LOW) {
    Serial.println("Button 3 pressed");
  }
}

void sendAudioToAIServer() {
  Serial.println("AI server communication is planned for the next firmware version.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);

  Serial.println();
  Serial.println("Starting TUYO Smart Speaker firmware...");
  Serial.println("Firmware version: 0.1");
  Serial.println("Board: Custom ESP32-WROOM-32 TUYO PCB");

  Serial.println();
  Serial.println("Pin map:");
  Serial.println("I2S BCLK  -> GPIO4");
  Serial.println("I2S LRCLK -> GPIO5");
  Serial.println("Mic SD    -> GPIO6");
  Serial.println("Speaker DIN -> GPIO7");
  Serial.println("LED/Data  -> GPIO12");
  Serial.println("Buttons   -> GPIO34, GPIO35, GPIO36");

  blinkStatusPin(2, 200);

  connectWiFi();
  setupI2SAudio();
  playTestTone();

  Serial.println();
  Serial.println("Setup complete.");
  Serial.println("Reading microphone levels...");
}

void loop() {
  int micLevel = readMicLevel();

  Serial.print("Mic level: ");
  Serial.println(micLevel);

  readButtons();

  if (micLevel > 500) {
    digitalWrite(STATUS_LED_PIN, HIGH);
  } else {
    digitalWrite(STATUS_LED_PIN, LOW);
  }

  delay(200);
}
