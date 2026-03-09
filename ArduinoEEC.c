// EEC-IV Diagnostic Tool for Arduino Mega 2560 R3
// Based on Cart and EecIv libraries
// Uses HardwareSerial (Serial1) for TTL-RS485 communication

#include "Cart.h"
#include "EecIv.h"

// Hardware pins
#define RE_PIN 2    // RS485 DE/RE pin connected to Digital 2
#define BTN_1 4     // Button 1 on Digital 4
#define BTN_2 5     // Button 2 on Digital 5 
#define BTN_3 6     // Button 3 on Digital 6

// Mode selection (using buttons)
#define MODE_READ_FAULTS 0
#define MODE_KOEO        1
#define MODE_LIVE_DATA   2

// Global objects
Cart* cart;
EecIv* eecIv;
int currentMode = MODE_LIVE_DATA;  // Default mode

// Button states and debounce
bool lastBtn1State = HIGH;
bool lastBtn2State = HIGH;
bool lastBtn3State = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Function prototypes
void onDebugPrint(const char message[]);
void onFaultCodeRead(const uint8_t data[]);
void onLiveData(const uint8_t data[]);
void onFaultCodeFinished();
void checkButtons();

void setup() {
  // Initialize serial ports
  Serial.begin(115200);  // Console/debug port
  Serial1.begin(2400);   // RS485 communication port
  
  Serial.println(F("EEC-IV Diagnostic Tool - Arduino Mega 2560"));
  Serial.println(F("Initializing..."));
  
  // Setup pins
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  
  // Initialize communication objects
  cart = new Cart(&Serial1, RE_PIN);
  eecIv = new EecIv(cart);
  
  // Setup callbacks
  eecIv->debugPrint = onDebugPrint;
  eecIv->onFaultCodeRead = onFaultCodeRead;
  eecIv->onFaultCodeFinished = onFaultCodeFinished;
  eecIv->onLiveData = onLiveData;
  
  // Set initial operating mode
  eecIv->setMode(EecIv::LIVE_DATA);
  
  Serial.println(F("Starting EEC-IV communication..."));
  eecIv->restartReading();
}

void loop() {
  // Handle EEC-IV communication
  eecIv->mainLoop();
  
  // Check for button presses
  checkButtons();
}

// Debug callback - print diagnostic messages to Serial
void onDebugPrint(const char message[]) {
  Serial.print(F("[DEBUG] "));
  Serial.println(message);
}

// Handle fault codes when read
void onFaultCodeRead(const uint8_t data[]) {
  Serial.print(F("FAULT CODE: 0x"));
  Serial.print(data[0], HEX);
  Serial.print(F(" 0x"));
  Serial.println(data[1], HEX);
  
  // Decode fault if needed - you can add code here to interpret fault codes
}

// Handle live data packets
void onLiveData(const uint8_t data[]) {
  // Format and print the PID data
  Serial.print(F("LIVE DATA: 0x"));
  Serial.print(data[0], HEX);
  Serial.print(F(" 0x"));
  Serial.println(data[1], HEX);
  
  // You can add specific PID processing here
  // For example, to decode RPM, MAP, TPS, etc.
}

// Called when fault code reading is complete
void onFaultCodeFinished() {
  Serial.println(F("All fault codes read"));
}

// Check button presses with debounce
void checkButtons() {
  // Read current button states
  bool btn1State = digitalRead(BTN_1);
  bool btn2State = digitalRead(BTN_2);
  bool btn3State = digitalRead(BTN_3);
  
  // Check for button state changes
  if ((btn1State != lastBtn1State) || 
      (btn2State != lastBtn2State) || 
      (btn3State != lastBtn3State)) {
    lastDebounceTime = millis();
  }
  
  // Check if enough time has passed for debounce
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Button 1 - Fault Codes
    if (btn1State == LOW && lastBtn1State == HIGH) {
      Serial.println(F("Changing to FAULT CODE mode"));
      currentMode = MODE_READ_FAULTS;
      eecIv->setMode(EecIv::READ_FAULTS);
      eecIv->restartReading();
    }
    
    // Button 2 - KOEO (Key On Engine Off) Test
    if (btn2State == LOW && lastBtn2State == HIGH) {
      Serial.println(F("Changing to KOEO test mode"));
      currentMode = MODE_KOEO;
      eecIv->setMode(EecIv::KOEO);
      eecIv->restartReading();
    }
    
    // Button 3 - Live Data
    if (btn3State == LOW && lastBtn3State == HIGH) {
      Serial.println(F("Changing to LIVE DATA mode"));
      currentMode = MODE_LIVE_DATA;
      eecIv->setMode(EecIv::LIVE_DATA);
      eecIv->restartReading();
    }
  }
  
  // Update button state tracking
  lastBtn1State = btn1State;
  lastBtn2State = btn2State;
  lastBtn3State = btn3State;
}
