
//Librerías
#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "UniversalTelegramBot.h"
#include "ArduinoJson.h"

#include "U8g2lib.h"
#include "string"
#include "DHT_U.h"
#include "Adafruit_Sensor.h"
#include "Wire.h"

//Pantalla Display en pixeles
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

//Sensor
#define DHTPIN 23 
#define DHTTYPE DHT11 

// Inicializar el Sensor
DHT dht(DHTPIN, DHTTYPE);

//Pines
#define PIN_BOTON_SUBIR 35 
#define PIN_BOTON_BAJAR 34
bool estadoBoton1 = false;
bool estadoBoton2 = false;

//Máquina de Estados Principal
#define PANTALLA_INICIAL 1    
#define LIMPIAR_1 2
#define PANTALLA_CAMBIOS 3
#define LIMPIAR_2 4


#define PRIMER_BOTON 1    
#define ESPERA_1 2
#define SEGUNDO_BOTON 3
#define ESPERA_2 4
#define TERCER_BOTON 5
#define ESPERA_3 6

//Estado inicial de la máquina
int maquina = 1;
int botones = 1;

//WiFI
const char* ssid = "ORT-IoT";
const char* password = "OrtIOTnew22$2";

// Initialize Telegram BOT
#define BOTtoken "6085478906:AAGkmo5pVC1NyXsRvGHJQ10DszGOgBZxsws"
#define CHAT_ID "-1001851624507"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000; /// intervalo
unsigned long lastTimeBotRan; /// ultimo tiempo

//Constructores y variables globales
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int valorUmbral = 19;


float t;
char temp[5];


//Variables delay
unsigned long tiempoAhora, tiempoCambio, tiempoAviso;

#define intervalo 5000

//Funciones
void initWiFi();

// Funcion Telegram
void handleNewMessages(int numNewMessages) {
  Serial.println("Mensaje nuevo");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {  
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/temp") {
      bot.sendMessage(chat_id, temp , "");
    }
  }
}

void initWiFi() 
{       
  WiFi.mode(WIFI_STA);                                
  WiFi.begin(ssid , password );
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Bot Hola mundo", "");
  Serial.println("Hola Mundo");
}

void setup() {
  Serial.begin(115200);

  //Llamo a la función
  initWiFi(); 

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Bot Hola mundo", "");

  Serial.println(F("OLED test"));
  u8g2.begin();
  dht.begin();

  pinMode(PIN_BOTON_SUBIR, INPUT_PULLUP);    
  pinMode(PIN_BOTON_BAJAR, INPUT_PULLUP); 
}

void loop() {

  //Telegram
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Veo los msj nuevos");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

      //Temperatura
  t = dht.readTemperature();
  
  sprintf(temp, "%2.1f", t);
  Serial.println(temp);

  if (t >= valorUmbral && (tiempoAhora - tiempoAviso <= 10000))
  {
    bot.sendMessage(CHAT_ID, "Supero el valor umbral" , "");
  }

  char umbral[2];
  sprintf(umbral, "%i", valorUmbral);
  Serial.println(umbral);

  tiempoAhora = millis();

  //Máquina de Estados
  switch (maquina) {
    case PANTALLA_INICIAL:
 
      u8g2.setFont(u8g_font_5x7);
      u8g2.drawStr(0, 15, "Temperatura: ");
      u8g2.drawStr(80, 15, temp);
      u8g2.drawStr(0, 30, "VU:");
      u8g2.drawStr(80, 30, umbral);
      u8g2.sendBuffer();

      switch (botones) {
        case PRIMER_BOTON:
        Serial.println("PRIMER BOTON");
          if (digitalRead(PIN_BOTON_SUBIR) == LOW) {
          
            botones = ESPERA_1;
          }
          
          break;

        case ESPERA_1:
          Serial.println("ESPERA 1");
          if (digitalRead(PIN_BOTON_SUBIR) == HIGH) {
            tiempoCambio = millis();
            botones = SEGUNDO_BOTON;
          }
          break;

        case SEGUNDO_BOTON:
        Serial.println("SEGUNDO BOTON");
          if (digitalRead(PIN_BOTON_BAJAR) == LOW) {
            if (tiempoAhora - tiempoCambio <= intervalo)
            {
              //tiempoCambio = millis();
              botones = ESPERA_2;
            }
            else
            {
              botones = PRIMER_BOTON;
            }
          }

          break;

        case ESPERA_2:
          Serial.println("ESPERA 2");
          if (digitalRead(PIN_BOTON_BAJAR) == HIGH) {
            tiempoCambio = millis();
            botones = TERCER_BOTON;
          }
          break;

        case TERCER_BOTON: 
          Serial.println("TERCER BOTON");
          if (tiempoAhora - tiempoCambio <= intervalo)
          {
            if (digitalRead(PIN_BOTON_SUBIR) == LOW) {
              botones = ESPERA_3;
            }
          }
          else
          {
            botones = PRIMER_BOTON;
          }
          break;

        case ESPERA_3:
          Serial.println("ESPERA 3");
          if (digitalRead(PIN_BOTON_SUBIR) == LOW) {
            maquina = LIMPIAR_1;
            botones = PRIMER_BOTON;
          }
          break;
      }
      Serial.println("PANTALLA INICIAL");
      break;

    case LIMPIAR_1:
      
      Serial.println("LIMPIAR 1");
      if (digitalRead(PIN_BOTON_BAJAR) == HIGH && digitalRead(PIN_BOTON_SUBIR) == HIGH) {
        maquina = PANTALLA_CAMBIOS;
      }
      break;

    case PANTALLA_CAMBIOS:
      Serial.println("PANTALLA CAMBIOS");
      Serial.println(valorUmbral);
      u8g2.clearBuffer();
      u8g2.setFont(u8g_font_5x7);
      u8g2.drawStr(0, 30, "Valor umbral: ");
      u8g2.drawStr(80, 30, umbral);
      u8g2.sendBuffer();
      
      if (digitalRead(PIN_BOTON_BAJAR) == LOW && digitalRead(PIN_BOTON_SUBIR) == LOW) {
        maquina = LIMPIAR_2;
      }
      
      if (digitalRead(PIN_BOTON_SUBIR) == LOW){
        
        estadoBoton1 = true;
        Serial.println("BOTON_PRESIONADO");

      }
      
      if(digitalRead(PIN_BOTON_SUBIR) == HIGH && estadoBoton1 == true) 
      {
        estadoBoton1 = false;
        Serial.println("BOTON_SUELTO");
        valorUmbral = valorUmbral + 1;

      }    
      
       if (digitalRead(PIN_BOTON_BAJAR) == LOW){
        
        estadoBoton2 = true;
        Serial.println("BOTON_PRESIONADO");

      }
      
      if(digitalRead(PIN_BOTON_BAJAR) == HIGH && estadoBoton2 == true) 
      {
        estadoBoton2 = false;
        Serial.println("BOTON_SUELTO");
        valorUmbral = valorUmbral - 1;
      }
      
      break;

    case LIMPIAR_2:
      Serial.println("LIMPIAR 2");
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      if (digitalRead(PIN_BOTON_BAJAR) == HIGH && digitalRead(PIN_BOTON_SUBIR) == HIGH) {
        maquina = PANTALLA_INICIAL;
      }
      break;
  }
}
