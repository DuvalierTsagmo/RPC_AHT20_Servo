/*
  Titre      : RPC
  Auteur     : Duvalier Tsagmo
  Date       : 28/09/2022
  Description: ce programme nous permet d'ecrire sur la carte SD
  Version    : 0.0.1
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_AHTX0.h>
#include "RTClib.h"
#include "WIFIConnector_MKR1000.h"
#include "MQTTConnector.h"
#include <stdio.h>
#include <Servo.h>

const int CHIPSELECT = 4;
RTC_DS3231 rtc;
Adafruit_AHTX0 aht;
String buffer;
String Humidite, Temperature, unixTime = "";
int espace1, espace2 = 0;
unsigned int const fileWriteTime = 2000;
unsigned int const timeToSend = 10000;

// Declaration de variable pour les LED et le servo moteur
const int pinBleu = 5;
const int pinRouge = 4;
Servo myservo;
int pos = 3;

boolean runEveryShort(unsigned long interval)

{
    static unsigned long previousMillis = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        return true;
    }

    return false;
}

boolean runEveryLong(unsigned long interval)

{
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        return true;
    }
    return false;
}

void lireCarteEnvoie()
{
    wifiConnect(); // Branchement au réseau WIFI
    MQTTConnect(); // Branchement au MQTT
    digitalWrite(LED_BUILTIN, HIGH);
    File dataFile = SD.open("datalog.txt", FILE_READ);
    if (dataFile)
    {
        // read from the file until there's nothing else in it:
        while (dataFile.available())
        {
            buffer = dataFile.readStringUntil('\n');
            Serial.println(buffer);
            sendPayloadOnly(buffer);
            ClientMQTT.loop();
        }
        int count = 10000;
        while (count)
        {
            ClientMQTT.loop();
            count--;
        }

        status = WL_IDLE_STATUS;
        WiFi.disconnect();
        WiFi.end();
        digitalWrite(LED_BUILTIN, LOW);
        // close the file:
        dataFile.close();
        SD.remove("datalog.txt");
        if (parametre == "rouge")
        {
            for (pos = 0; pos <= 90; pos += 1)
            { // goes from 0 degrees to 180 degrees
                // in steps of 1 degree
                myservo.write(pos); // tell servo to go to position in variable 'pos'
                delay(15);
            }
            digitalWrite(pinRouge, 1);
            digitalWrite(pinBleu, 0);
        }
        else if (parametre == "bleu")
        {
            for (pos = 90; pos >= 90; pos -= 1)
            { // goes from 0 degrees to 180 degrees
                // in steps of 1 degree
                myservo.write(pos); // tell servo to go to position in variable 'pos'
                delay(15);
            }
            digitalWrite(pinBleu, 1);
            digitalWrite(pinRouge, 0);
        }
        else if (methode == "false")
        {
            digitalWrite(pinBleu, 0);
            digitalWrite(pinRouge, 0);
        }
    }
    else
    {
        // if the file didn't open, print an error:
        Serial.println("ouverture d'erreur datalog.txt");
    }
}

void ecrireCarte()
{
    DateTime now = rtc.now();
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    // la chaine qui permet d'envoyer la date et heure
    String stringToWrite = "{\"ts\":" + String((double)(now.unixtime() + 10800) * 1000) + ",\"values\":{\"temperature\":" + temp.temperature + ",\"humidite\":" + humidity.relative_humidity + "}}\n";
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile)
    {

        dataFile.print(stringToWrite);
        Serial.println(stringToWrite);

        dataFile.close();
    }
    else
    {
        Serial.println("ouverture d'erreur datalog.txt");
    }
}

void setup()
{

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(pinBleu, OUTPUT);
    pinMode(pinRouge, OUTPUT);
    myservo.attach(9);

    Serial.begin(9600);
    if (!SD.begin(CHIPSELECT))
    {
        Serial.println("Card failed, or not present");
        while (1)
            ;
    }

    if (!aht.begin())
    {
        Serial.println("Impossible de trouver l'AHT ? Vérifiez le câblage");
        while (1)
            delay(10);
    }
    Serial.println(" AHT20 est trouve");

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1)
            delay(10);
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    File dataFile = SD.open("datalog.txt", FILE_WRITE | O_TRUNC);
    dataFile.close();
}

void loop()
{

    if (runEveryShort(fileWriteTime))
    {
        ecrireCarte();
    }

    if (runEveryLong(timeToSend))
    {
        lireCarteEnvoie();
    }
}
