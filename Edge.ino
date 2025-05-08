#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "esp_wifi.h"

// --- Telegram
#define BOTtoken "7982045470:AAECMR_d-6v_EpKlqk137a4XbWbZ-yT1SV0"
#define CHAT_ID "1856694079"

// --- Credenciales de WiFi ---
const char* ssid = "Alison";
const char* password = "Alison15";

// Canales
int canal_espnow = 1;
int canal_wifi = 6;

// Cliente seguro y bot de Telegram
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

// Umbrales arbitrarios
#define UMBRAL_LUZ 50.0
#define UMBRAL_TEMP_DHT 23.0
#define UMBRAL_HUM_DHT 90.0
#define UMBRAL_DS18B20 30.0

// Identificadores de sensores
typedef enum {
  TIPO_DHT11,
  TIPO_LUZ,
  TIPO_DS18B20
} tipo_sensor_t;

typedef struct {
  tipo_sensor_t tipo;
  float temperatura;
  float humedad;
} temp_hum_data_t;

typedef struct {
  tipo_sensor_t tipo;
  float luz;
} luz_data_t;

typedef struct {
  tipo_sensor_t tipo;
  float temperatura;
} ds18b20_data_t;

// Variables para manejar mensajes pendientes
String mensajePendiente = "";
bool enviarMensaje = false;

// Función para cambiar de canal
void cambiarCanal(int canal) {
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}

// Función mejorada para enviar notificaciones por Telegram
void enviarTelegram(String mensaje) {
  // Apagar ESP-NOW antes de cambiar de modo
  esp_now_deinit();

  WiFi.disconnect(true);     // Desconectar de WiFi anterior
  delay(500);

  cambiarCanal(canal_wifi);  // Cambiar al canal WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi");

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi conectado. Enviando mensaje...");

    secured_client.setInsecure();  // ⚠ Solo para pruebas

    if (bot.sendMessage(CHAT_ID, mensaje, "")) {
      Serial.println("✅ Notificación enviada por Telegram.");
    } else {
      Serial.println("❌ Error al enviar notificación.");
    }
  } else {
    Serial.println("\n❌ No se pudo conectar a WiFi.");
  }

  // Regresar a modo ESP-NOW
  WiFi.disconnect(true);
  delay(500);
  cambiarCanal(canal_espnow);
  WiFi.mode(WIFI_STA);  // Volver al modo STA para ESP-NOW

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Error al reactivar ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("✅ ESP-NOW reactivado");
  }
}

// Función de recepción ESP-NOW
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  tipo_sensor_t tipo;
  memcpy(&tipo, data, sizeof(tipo));

  switch (tipo) {
    case TIPO_DHT11: {
      if (len == sizeof(temp_hum_data_t)) {
        temp_hum_data_t sensor;
        memcpy(&sensor, data, sizeof(sensor));
        Serial.printf("DHT11: Temp=%.2f°C, Hum=%.2f%%\n", sensor.temperatura, sensor.humedad);
        if (sensor.temperatura > UMBRAL_TEMP_DHT || sensor.humedad > UMBRAL_HUM_DHT) {
          mensajePendiente = "⚠ DHT11 superó umbral:\nTemp: " + String(sensor.temperatura) + "°C\nHum: " + String(sensor.humedad) + "%";
          enviarMensaje = true;
        }
      }
      break;
    }

    case TIPO_LUZ: {
      if (len == sizeof(luz_data_t)) {
        luz_data_t luz;
        memcpy(&luz, data, sizeof(luz));
        Serial.printf("Luz: %.2f\n", luz.luz);
        if (luz.luz > UMBRAL_LUZ) {
          mensajePendiente = "⚠ Sensor de luz superó umbral:\nLuz: " + String(luz.luz);
          enviarMensaje = true;
        }
      }
      break;
    }

    case TIPO_DS18B20: {
      if (len == sizeof(ds18b20_data_t)) {
        ds18b20_data_t temp;
        memcpy(&temp, data, sizeof(temp));
        Serial.printf("DS18B20: Temp=%.2f°C\n", temp.temperatura);
        if (temp.temperatura > UMBRAL_DS18B20) {
          mensajePendiente = "⚠ DS18B20 superó umbral:\nTemp: " + String(temp.temperatura) + "°C";
          enviarMensaje = true;
        }
      }
      break;
    }

    default:
      Serial.println("Tipo de sensor desconocido.");
  }

  Serial.println("=======================");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  cambiarCanal(canal_espnow);  // Iniciar en canal ESP-NOW

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al iniciar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (enviarMensaje) {
    enviarTelegram(mensajePendiente);
    mensajePendiente = "";
    enviarMensaje = false;
  }

delay(100);
}
