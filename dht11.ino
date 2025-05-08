#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

#define DHTPIN 4         
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

// --- Identificadores para tipos de sensor ---
typedef enum {
  TIPO_DHT11,
  TIPO_LUZ,
  TIPO_DS18B20
} tipo_sensor_t;

// --- MAC del ESP32 que es el receptor ---
uint8_t receiverMAC[] = {0x3C, 0x8A, 0x1F, 0x08, 0x5E, 0x64};

// Estructura para enviar temperatura y humedad con tipo de sensor
typedef struct {
  tipo_sensor_t tipo;
  float temperatura;
  float humedad;
} temp_hum_data_t;
temp_hum_data_t dataToSend;

// Peer info
esp_now_peer_info_t peerInfo;

// Callback de envío
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Estado del envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallo");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  
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
  // Leer temperatura y humedad
  dataToSend.temperatura = dht.readTemperature();
  dataToSend.humedad = dht.readHumidity();

  // Validar datos
  if (isnan(dataToSend.temperatura) || isnan(dataToSend.humedad)) {
    Serial.println("Error al leer DHT");
    delay(2000);
    return;
  }

  // Asignar el tipo de sensor (TIPO_DHT11)
  dataToSend.tipo = TIPO_DHT11;

  // Enviar datos
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));

  if (result == ESP_OK) {
    Serial.print("Temperatura enviada: ");
    Serial.print(dataToSend.temperatura);
    Serial.print(" °C | Humedad: ");
    Serial.print(dataToSend.humedad);
    Serial.println(" %");
  } else {
    Serial.println("Fallo en el envío");
  }

  delay(2000);  // Esperar 2 segundos antes de enviar de nuevo
}