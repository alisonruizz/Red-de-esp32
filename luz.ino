#include <esp_now.h>
#include <WiFi.h>

// MAC del receptor (ESP32 que recibe datos de luz)
uint8_t receiverMAC[] = {0x3C, 0x8A, 0x1F, 0x08, 0x5E, 0x64};

// Pin del sensor de luz (LDR)
const int lightSensorPin = 34;

// Identificadores para tipos de sensor
typedef enum {
  TIPO_DHT11,
  TIPO_LUZ,
  TIPO_DS18B20
} tipo_sensor_t;

// Estructura de datos para enviar (incluye el tipo de sensor)
typedef struct {
  tipo_sensor_t tipo;
  float luz;
} luz_data_t;

luz_data_t luzData;

// Callback para saber si se envió correctamente
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Estado del envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallo");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Modo estación (no crea red)

  // Iniciar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al iniciar ESP-NOW");
    return;
  }

  // Registrar callback de envío
  esp_now_register_send_cb(OnDataSent);

  // Configurar el receptor como peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;  // Mismo canal que WiFi
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverMAC)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Error al agregar peer");
      return;
    }
  }
}

void loop() {
  // Leer valor del sensor de luz
  int raw = analogRead(lightSensorPin);
  luzData.luz = (float)raw;

  // Asignar el tipo de sensor (TIPO_LUZ)
  luzData.tipo = TIPO_LUZ;

  // Enviar datos
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&luzData, sizeof(luzData));

  if (result == ESP_OK) {
    Serial.print("Luz enviada: ");
    Serial.println(luzData.luz);
  } else {
    Serial.println("Fallo en el envío");
  }

  delay(2000);  // Esperar 2 segundos antes de enviar de nuevo
}
