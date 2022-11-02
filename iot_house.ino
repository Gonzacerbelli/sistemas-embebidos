//Librerias
//RFID
#include <SPI.h>
#include <MFRC522.h>
//Display 16x02
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//Sensor DHT11 para medir temperatura y humedad
#include <DHT.h>


// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 2
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

//Crear el objeto lcd  dirección  0x27, 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);

//RFID
#define RST_PIN         9
#define SS_PIN          10
MFRC522 rfid(SS_PIN, RST_PIN);


//Variables globales
bool acceso = 0;
long uid_tarjeta;
float tanque_de_agua = 1000;
bool bomba_encendida = 0;

//Leds
const int LED_ACCESO = 7;
const int LED_BOMBA = 6;

void setup() {
  //Inicializamos la comunicación serial
  Serial.begin(9600);
  //Escribimos por el monitor serie mensaje de inicio
  Serial.println("Inicio de sistema");

  //dht11
  dht.begin();

  //RFID
  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();   // Init MFRC522

  //LCD
  lcd.init();
  //Encender la luz de fondo.
  lcd.backlight();

  //LEDS
  pinMode(LED_ACCESO, OUTPUT);
  pinMode(LED_BOMBA, OUTPUT);
}

void calcular_tanque_de_agua() {

  //float nivel_agua = tanque_de_agua - analogRead(A0) / 10.23;

  if (bomba_encendida) {
    tanque_de_agua = tanque_de_agua + (analogRead(A0) / 10.23);
  } else {
    tanque_de_agua = tanque_de_agua - (analogRead(A0) / 10.23);
  }

  if (tanque_de_agua < 200) {
    bomba_encendida = 1;
    digitalWrite(LED_BOMBA, HIGH);
  }
  if (tanque_de_agua > 850) {
    bomba_encendida = 0;
    digitalWrite(LED_BOMBA, LOW);
  }
}

float leer_humedad() {
  return dht.readHumidity();
}

float leer_temperatura() {
  return dht.readTemperature();
}

float calcular_indice_de_calor(float temperatura, float humedad) {
  return dht.computeHeatIndex(temperatura, humedad, false);
}

void probar_conexion_rfid() {
  bool result = rfid.PCD_PerformSelfTest();
  Serial.print(F("\nPrueba de conexion con RFID: "));
  if (result)
    Serial.println(F("Conectado"));
  else
    Serial.println(F("Desconectado"));
}

long obtener_uid() {
  if ( ! rfid.PICC_ReadCardSerial()) { //Cortar una vez que se obtiene el Serial
    return -1;
  }
  unsigned long hex_num;
  hex_num =  rfid.uid.uidByte[0] << 24;
  hex_num += rfid.uid.uidByte[1] << 16;
  hex_num += rfid.uid.uidByte[2] <<  8;
  hex_num += rfid.uid.uidByte[3];
  rfid.PICC_HaltA(); // Dejar de leer
  return hex_num;
}

void entrar() {
  if (rfid.PICC_IsNewCardPresent()) {
    unsigned long uid = obtener_uid();
    if (uid != -1) {
      Serial.print("Tarjeta detectada, UID: " + String(uid));
      uid_tarjeta = uid;
      acceso = 1;
      digitalWrite(LED_ACCESO, HIGH);
      lcd.clear();
      lcd.print("Bienvenido");
    }
  }
}

void salir() {
  if (rfid.PICC_IsNewCardPresent()) {
    unsigned long uid = obtener_uid();
    if (uid != -1) {
      Serial.print("Tarjeta detectada, UID: " + String(uid));
      if (uid_tarjeta == uid) {
        lcd.clear();
        acceso = 0;
        uid_tarjeta = 0;
        digitalWrite(LED_ACCESO, LOW);
        lcd.print("Adios");
        Serial.print("Saliendo");
      }
    }
  }
}

void loop() {

  probar_conexion_rfid();

  if (!acceso) {
    lcd.setCursor(0, 0);
    lcd.print("Ingrese llave");

    Serial.println("Bienvenido. Escanear llave magnetica");

    entrar();

  } else {
    calcular_tanque_de_agua();
    float temperatura = leer_temperatura();
    float humedad = leer_humedad();
    float indice_de_calor = calcular_indice_de_calor(temperatura, humedad);

    Serial.println("El tanque de agua tiene " + String(tanque_de_agua) + " litros");
    Serial.println("La temperatura es de " + String(temperatura) + "°C");
    Serial.println("La humedad es del " + String(humedad) + "%");
    Serial.println("El índice de calor es de " + String(indice_de_calor) + "%");
    Serial.println();

    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temperatura) + "C - Ind Cal: " + String(indice_de_calor));
    lcd.setCursor(0, 1);
    lcd.print("Hum: " + String(humedad) + "% - Agua: " + String(tanque_de_agua) + "l");

    for (int i = 0; i < 7; i++) {
      lcd.scrollDisplayLeft();
    }

    salir();
  }


  delay(1500);

}
