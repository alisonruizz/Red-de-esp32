#include <esp_now.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- CONFIGURACIÓN DEL SENSOR DS18B20 ---
#define ONE_WIRE_BUS 4  // Pin al que está conectado el sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- Identificadores para tipos de sensor ---
typedef enum {
  TIPO_DHT11,
  TIPO_LUZ,
  TIPO_DS18B20
} tipo_sensor_t;

// --- MAC del receptor ---
uint8_t receiverMAC[] = {0x3C, 0x8A, 0x1F, 0x08, 0x5E, 0x64};

// Estructura para enviar solo temperatura con el tipo de sensor
typedef struct {
  tipo_sensor_t tipo;
  float temperatura;
} temp_data_t;
temp_data_t dataToSend;

// Peer info
esp_now_peer_info_t peerInfo;

// Variable global para guardar última temperatura enviada
float ultimaTemperaturaEnviada = 0.0;

// Callback cuando se ha enviado un mensaje
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Mensaje enviado a ");
  Serial.print(macStr);
  Serial.print(" -> ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallo");
  Serial.print(" | Temperatura enviada: ");
  Serial.print(ultimaTemperaturaEnviada);
  Serial.println(" °C");
}

void setup() {
  Serial.begin(115200);
  sensors.begin();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(onDataSent);  // Registrar el callback

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverMAC)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Error al agregar peer");
      return;
    }
  }
}

void loop() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Error al leer temperatura");
    delay(2000);
    return;
  }

  dataToSend.tipo = TIPO_DS18B20;
  dataToSend.temperatura = temp;
  ultimaTemperaturaEnviada = temp;  // Guardar el valor para mostrarlo en el callback

  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));

  if (result != ESP_OK) {
    Serial.println("Fallo inmediato al enviar el mensaje");
  }

  delay(2000);
}

