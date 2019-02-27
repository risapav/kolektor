/*
* Arduino
*
* (c) Peter Riša 2019
*
* riadenie ohrevu vody cez slnečný kolektor
*/
#include <Arduino.h>
#include <Wire.h>

// knižnice pre komunikáciu s teplomermi Dallas DS1820
#include <OneWire.h>
#include <DallasTemperature.h>
// knižnica pre zobrazovanie na LCD
#include <LiquidCrystal.h>

// konštanty pre riadenie čerpadla
#define CERPADLO_CHOD 1
#define CERPADLO_STOP 0

// konštanty pre nadefinovanie displeja LCD display 1602A
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

// konštanty pre zapnutie ohrevu
#define TEPLOTA_MAX 45.0
#define TEPLOTA_VIAC 5.0
#define TEPLOTA_MENEJ 3.0

// koľko môže byť DS18S20 teplomerov pripojených
#define MAX_DS1820_SENSORS 5
// pole teplôt
// teplota[0] = T1 - teplota kolektora
// teplota[1] = T2 - teplota v bojleri
// teplota[2] = T3 - teplota okolia
float teplota[MAX_DS1820_SENSORS];

// definovanie vstupov a výstupov arduina
// signalizacna LED
int ledPin = 0; // LED je na pine D0
// rele spustajuce motor
int relayPin = 9; // relé je na pine D9
int num_temp = 0; //pocet teplomerov pripojenych
// vytvoření instance oneWireDS z knihovny OneWire
// teplomery
OneWire  oneWireDS(1);  // teplomery sú na pine D1
// vytvoření instance senzoryDS z knihovny DallasTemperature
DallasTemperature senzoryDS(&oneWireDS);

byte addr[MAX_DS1820_SENSORS][8];
char buf[20];

// definovanie rozhrania displeja (rs, enable, d4, d5, d6, d7)
// D2, D3, D4, D5, D6, D7
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// na začiatok čerpadlo stojí
int cerpadlo = CERPADLO_STOP;

// priprav všetko pred spustením regulácie
void setup() {

    // najprv predpokladame, že čerpadlo nejde
    cerpadlo = CERPADLO_STOP;

    // iniciacia led
    pinMode(ledPin, OUTPUT);  //Set pin for output
    digitalWrite(ledPin, HIGH);  // HIGH is off, LOW is on

    // iniciacia rele modulu
    pinMode(relayPin, OUTPUT);  //Set pin for output
    digitalWrite(relayPin, HIGH);  // HIGH is off, LOW is on

    // začni komunikovať s displejom
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);

    //napis uvodny text
    lcd.setCursor(0,0); // riadok hore
    lcd.print("(c) Peter Risa");
    lcd.setCursor(0,1); // riadok dole
    lcd.print("Slnecny kolektor");
    delay(3000); // 3 seconds delay

    // začni komunikovať s teplomermi
    senzoryDS.begin();

    //zisti počet teplomerov
    lcd.clear(); // Clears the LCD screen
    lcd.setCursor(0,0); // riadok hore
    lcd.print("DS1820 Test");

    byte i;
    for(i=0;i < MAX_DS1820_SENSORS;i++) {
        if (!oneWireDS.search(addr[i])) {
            lcd.setCursor(0,1); // riadok dole
            lcd.print("No more addresses.");
            oneWireDS.reset_search();
            delay(250);
            break;
        }
        num_temp ++; // počet teplomerov zvýš o 1
        sprintf(buf, "Pocet teplomerov: %d",num_temp);
        lcd.setCursor(0,1); // riadok dole
        lcd.print(buf);
        delay(2000);
    }
    lcd.setCursor(0,0); // riadok hore
    lcd.print("Aktualny stav");
}

// riadenie ohrevu
void loop() {

    // zapne LED
    digitalWrite(ledPin, LOW);  // HIGH is off, LOW is on
    // načíta všetky teplomery
    byte sensor;
    senzoryDS.requestTemperatures();
    // uloží teploty do poľa teplota
    for (sensor=0;sensor < num_temp;sensor++) {
        teplota[sensor]=senzoryDS.getTempCByIndex(sensor);
    }

    //  čerpadlo má bežať ak teploty sú aspoň 2
    if(num_temp<2)
        return;

    // ak teplota T1 (teplota kolektora) je o TEPLOTA_VIAC väčšia
    // ako T2 (teplota v bojleri), potom spusti čerpadlo
    if(teplota[0] > (teplota[1] + TEPLOTA_VIAC))
        cerpadlo=CERPADLO_CHOD;

    // ak teplota T1 (teplota kolektora) bez TEPLOTA_MENEJ je menšia
    // ako T2 (teplota v bojleri), potom vypni čerpadlo
    if((teplota[0] - TEPLOTA_MENEJ) < teplota[1] )
        cerpadlo=CERPADLO_STOP;

    // ak teplota T2 (teplota v bojleri) je viac ako TEPLOTA_MAX,
    // zastav čerpadlo, aby sme neprehriali bojler
    if(teplota[1] > TEPLOTA_MAX)
        cerpadlo=CERPADLO_STOP;

    //teraz skutočne vypni alebo zapni čerpadlo
    if(cerpadlo == CERPADLO_CHOD) {
        // zapni čerpadlo
        digitalWrite(relayPin, LOW);
    } else {
        // vypni čerpadlo
        digitalWrite(relayPin, HIGH);
    }

    //zobraz teplotu na displej uloz ich do pola teplot
    for (sensor=0;sensor < num_temp;sensor++) {
        lcd.setCursor(0,1);
        sprintf(buf, "C: %d, T%d: ",cerpadlo, sensor+1);
        lcd.print(buf);
        lcd.print(teplota[sensor]);
        delay(1000);
    }

    // vypne LED
    digitalWrite(ledPin, HIGH);  // HIGH is off, LOW is on
    delay(1000);

}
