#include <SPI.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>
#include "LedMatrix.h"

// Wi-Fi point
const char* ssid     = "MGBot";
const char* password = "Terminator812";

// Bot account
#define BOTtoken "281184648:AAHlaMTRFrxZqD4VlN9akahmGPX8U0BtzS4"
#define BOTname "led_string_bot"
#define BOTusername "led_string_bot"
TelegramBOT bot(BOTtoken, BOTname, BOTusername);

#define LED_UPDATE_TIME 10         // Update time for LED string
#define DHT22_UPDATE_TIME 5000    // Update time for DHT22 sensor
#define TELEGRAM_UPDATE_TIME 1000  // Update time for telegram bot

// DHT22 sensor
#define DHT22_PIN 4
DHT dht22(DHT22_PIN, DHT22, 15);

// LED matrix
#define NUM_LED_DEVICES 8
#define LED_CS_PIN      2
LedMatrix ledMatrix = LedMatrix(NUM_LED_DEVICES, LED_CS_PIN);

// Timer counters
unsigned long timer_led = 0;
unsigned long timer_dht22 = 0;
unsigned long timer_telegram = 0;

// Data from sensors
float h1 = 0;
float t1 = 0;

// Text mode
int text_mode = 0;

void setup()
{
  // Init serial port
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Init Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Start telegram bot library
  bot.begin();

  // Init DHT22
  dht22.begin();

  // First measurement
  readDHT22();

  // Init LED matrix
  ledMatrix.init();
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  ledMatrix.setCharWidth(9);

  // Set ini string on LED matrix
  ledMatrix.setText(String(t1, 1) + "`C " + String(h1, 1) + "%");
}

void loop()
{
  // Execute scroll cycle for LED string
  if (millis() > timer_led + LED_UPDATE_TIME)
  {
    ledMatrix.clear();
    ledMatrix.setIntensity(10);
    ledMatrix.scrollTextLeft();
    ledMatrix.drawText();
    ledMatrix.commit();
    // Update Telegram and DHT22 when LED is blank
    if (ledMatrix.myTextOffset == 0)
    {
      // DHT22 sensor timeout
      if (millis() > timer_dht22 + DHT22_UPDATE_TIME)
      {
        readDHT22();
        Serial.print("Air temperature: ");
        Serial.print(t1);
        Serial.println(" *C");
        Serial.print("Air humidity: ");
        Serial.print(h1);
        Serial.println(" %");
        if (text_mode == 0)
        {
          ledMatrix.setNextText(String(t1, 1) + "`C " + String(h1, 1) + "%");
        }
        timer_dht22 = millis();
      }
      // Execute Telegram Bot
      if (millis() > timer_telegram + TELEGRAM_UPDATE_TIME)
      {
        bot.getUpdates(bot.message[0][1]);
        Telegram_ExecMessages();
        timer_telegram = millis();
      }
    }
    timer_led = millis();
  }
}

// Read DHT22 sensor
void readDHT22()
{
  h1 = dht22.readHumidity();
  t1 = dht22.readTemperature();
  if (isnan(h1) || isnan(t1))
  {
    Serial.println("Failed to read from DHT22 sensor!");
  }
}

// Execute Telegram events
void Telegram_ExecMessages()
{
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)
  {
    bot.message[i][5] = bot.message[i][5].substring(0, bot.message[i][5].length());
    String str1 = bot.message[i][5];
    String str2 = bot.message[i][5];
    str1.toUpperCase();
    Serial.println("Message: " + str1);
    if ((str1 == "START") || (str1 == "\/START"))
    {
      bot.sendMessage(bot.message[i][4], "Привет! Я светодиодная строка!", "");
    }
    else if ((str1 == "STOP") || (str1 == "\/STOP"))
    {
      bot.sendMessage(bot.message[i][4], "Пока!", "");
    }
    else if ((str1 == "AIR") || (str1 == "WEATHER"))
    {
      bot.sendMessage(bot.message[i][4], "Температура воздуха: " + String(t1, 1) + " *C", "");
      bot.sendMessage(bot.message[i][4], "Влажность воздуха: " + String(h1, 1) + " %", "");
      text_mode = 0;
      ledMatrix.clear();
      ledMatrix.setText(String(t1, 1) + "`C " + String(h1, 1) + "%");
    }
    else if ((str1 == "AIR TEMP") || (str1 == "AIR TEMPERATURE"))
    {
      bot.sendMessage(bot.message[i][4], "Температура воздуха: " + String(t1, 1) + " *C", "");
      text_mode = 0;
      ledMatrix.clear();
      ledMatrix.setText(String(t1, 1) + "`C " + String(h1, 1) + "%");
    }
    else if ((str1 == "AIR HUM") || (str1 == "AIR HUMIDITY"))
    {
      bot.sendMessage(bot.message[i][4], "Влажность воздуха: " + String(h1, 1) + " %", "");
      text_mode = 0;
      ledMatrix.clear();
      ledMatrix.setText(String(t1, 1) + "`C " + String(h1, 1) + "%");
    }
    else
    {
      bot.sendMessage(bot.message[i][4], "Выводим новую строку...", "");
      text_mode = 1;
      ledMatrix.clear();
      ledMatrix.setText(str2);
    }
  }
  bot.message[0][0] = "";
}

