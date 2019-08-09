#include "ESP8266WiFi.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MCP4725.h>

bool DEBUG_CONSOLE  = false;
bool HC_LED_ENABLED = true;

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 hcDisplay(OLED_RESET);

Adafruit_MCP4725 hcDac;

bool HC_PEDAL_ENGAGED_CURRENT = false;
bool HC_PEDAL_ENGAGED_PREV = false;

// ===== CONFIG =====
uint8_t HC_LED_OUTPUT_PIN  = D4;
uint8_t HC_PEDAL_INPUT_PIN = D6;
uint8_t HC_ANALOG_IN_PIN   = A0;

int16_t  HC_PWM_OUTPUT_LEVEL_CURRENT   = 0; // can be negative, otherwise positive can overlap into max int
uint16_t HC_PWM_OUTPUT_LEVEL_MAX       = 1000; // 100% duty cycle is 1023
uint8_t  HC_PWM_OUTPUT_LEVEL_UP_STEP   = 20;
uint8_t  HC_PWM_OUTPUT_LEVEL_DOWN_STEP = 50; // brake faster then accelerate
// ===== END OF CONFIG =====

void displaySetup() {
  hcDisplay.begin(SSD1306_SWITCHCAPVCC, 0x3C); // 0x3C or 0x3D
  hcDisplay.clearDisplay();
  hcDisplay.display();
  hcDisplay.setTextSize(1); // 6 rows, 10 cols
  hcDisplay.setTextColor(WHITE);
  hcDisplay.display();
}

void displayPrepare() {
  hcDisplay.clearDisplay();
//  hcDisplay.display();
  hcDisplay.setCursor(0, 0);
  hcDisplay.print("Hover-Car");
}

int adcInput = 0;
float voltageIn = 0;
float MCP4725_reading;

void displayUpdate() {
  displayPrepare();
  String pedalStatus = (HC_PEDAL_ENGAGED_CURRENT) ? "YES" : "NO";
  hcDisplay.printf("\nDIR: %s", (true) ? "F=>" : "<=B");
  hcDisplay.printf("\nPDL: %s", (HC_PEDAL_ENGAGED_CURRENT) ? "YES" : "NO");
//  hcDisplay.printf("\nPWM: %d", HC_PWM_OUTPUT_LEVEL_CURRENT);

  adcInput = analogRead(HC_ANALOG_IN_PIN);
  voltageIn = (adcInput * 3.3 ) / 1024.0;
  hcDisplay.print("\nVop: ");
  hcDisplay.print(voltageIn, 3);
  
  hcDisplay.printf("\n\n%d", millis() / 100);
}

void setup() {
  if (DEBUG_CONSOLE) {
    Serial.begin(9600);
  }
  
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  
  if (HC_LED_ENABLED) {
    pinMode(HC_LED_OUTPUT_PIN, OUTPUT);
    digitalWrite(HC_LED_OUTPUT_PIN, HIGH); // LED off
  }
  
  pinMode(HC_PEDAL_INPUT_PIN, INPUT_PULLUP);
  
  displaySetup();
  displayUpdate();

  hcDac.begin(0x60);
  hcDac.setVoltage(0, false);
}

void loop() {
  HC_PEDAL_ENGAGED_CURRENT = (digitalRead(HC_PEDAL_INPUT_PIN) == LOW) ? true : false;

  if (HC_PEDAL_ENGAGED_PREV != HC_PEDAL_ENGAGED_CURRENT) {

    if (DEBUG_CONSOLE) {
      Serial.print("Pedal: ");
      Serial.print((HC_PEDAL_ENGAGED_CURRENT) ? "YES" : "NO");
      Serial.print(", was ");
      Serial.print((HC_PEDAL_ENGAGED_PREV) ? "YES" : "NO");
      Serial.println();
    }
    
    switch (HC_PEDAL_ENGAGED_CURRENT) {
      case true: // pedal pressed
        if (HC_LED_ENABLED) {
          digitalWrite(HC_LED_OUTPUT_PIN, LOW); // LED on
        }
        break;
        
      case false: // pulled up, no pedal
        if (HC_LED_ENABLED) {
          digitalWrite(HC_LED_OUTPUT_PIN, HIGH); // LED off
        }
        break;
    }

    HC_PEDAL_ENGAGED_PREV = HC_PEDAL_ENGAGED_CURRENT;
    displayUpdate();
  }

  if (HC_PEDAL_ENGAGED_CURRENT && HC_PWM_OUTPUT_LEVEL_CURRENT < HC_PWM_OUTPUT_LEVEL_MAX) {
    HC_PWM_OUTPUT_LEVEL_CURRENT += HC_PWM_OUTPUT_LEVEL_UP_STEP;
    if (HC_PWM_OUTPUT_LEVEL_CURRENT > HC_PWM_OUTPUT_LEVEL_MAX) { // if exeed MAX
      HC_PWM_OUTPUT_LEVEL_CURRENT = HC_PWM_OUTPUT_LEVEL_MAX;
    }
    
    if (DEBUG_CONSOLE) {
      Serial.printf("PWM: %d" + HC_PWM_OUTPUT_LEVEL_CURRENT);
      Serial.println();
    }

    hcDac.setVoltage(HC_PWM_OUTPUT_LEVEL_CURRENT, false);
    if (DEBUG_CONSOLE) {
      MCP4725_reading = (3.3/4096.0) * HC_PWM_OUTPUT_LEVEL_CURRENT; //3.3 is your supply voltage
      Serial.print("Expected Voltage: ");
      Serial.println(MCP4725_reading, 3);
    }
    
    displayUpdate();
    
  } else if (!HC_PEDAL_ENGAGED_CURRENT && HC_PWM_OUTPUT_LEVEL_CURRENT > 0) {
    HC_PWM_OUTPUT_LEVEL_CURRENT -= HC_PWM_OUTPUT_LEVEL_DOWN_STEP;
    if (HC_PWM_OUTPUT_LEVEL_CURRENT < 0) { // if we went negative
      HC_PWM_OUTPUT_LEVEL_CURRENT = 0;
    }
    
    if (DEBUG_CONSOLE) {
      Serial.printf("PWM: %d" + HC_PWM_OUTPUT_LEVEL_CURRENT);
      Serial.println();
    }

    hcDac.setVoltage(HC_PWM_OUTPUT_LEVEL_CURRENT, false);
    if (DEBUG_CONSOLE) {
      MCP4725_reading = (3.3/4096.0) * HC_PWM_OUTPUT_LEVEL_CURRENT; //3.3 is your supply voltage
      Serial.print("Expected Voltage: ");
      Serial.println(MCP4725_reading, 3);
    }
    

    
    displayUpdate();
  }

  // finally, update display
  hcDisplay.display();
  
  delay(100);
}
