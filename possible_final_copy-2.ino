#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>

#define Vout A0
#define R0  3600.0  // R0 from Wheatstone Bridge
#define R   100000.0 // R From Wheatstone Bridge
#define Vin 9.0


const int buttonPin = 6;      // Digital pin connected to the push button
const int chipSelect = 10;    // Chip select pin for the SD card

File dataFile;
bool logging = false;
unsigned long lastDebounceTime = 0;  // The last time the button state changed
unsigned long debounceDelay = 50;    // The debounce time; increase if the output flickers
int lastButtonState = HIGH;          // The previous state of the button
int buttonState;                    // The current state of the button

double A = 0.001130027; // -15 Celsius
double B = 0.0002339933; // 35 Celsius 
double C = 8.88096e-8; // 85 Celsius

double Vo, W_BridgeVoltage, ThermistorResistance, temperature;
//const float Gain = 40000.0 / 20000.0; //gain = 1.9996
const float Gain = 1.85;

const int RS = 9, EN = 8, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);


void setup() {
  Serial.begin(1000000);
  pinMode(buttonPin, INPUT_PULLUP); // Use the internal pull-up resistor
  pinMode(A0, INPUT);
  lcd.begin(16, 2);
  // Initialize the SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  
  // Create a new file for logging data
  dataFile = SD.open("data.txt", FILE_WRITE);
  if (!dataFile) {
    Serial.println("Error opening data.txt");
    return;
  }
  
}

void loop() {
  // Read the sensor value
  // Always update the temperature and display it on the LCD
  int a = analogRead(Vout);
  Vo = (5/1023.0) * a; // Calculate the real voltage (0V~5V)
  Vo = Vo - 0.005224719;
  Serial.print("Vout: ");
  Serial.println(Vo);

  W_BridgeVoltage = Vo/Gain;

  double X = W_BridgeVoltage/Vin;
  double Y = (R0/(R+R0));

  ThermistorResistance = (R * (X + Y)) / (1 - (X + Y));

  temperature = 1 / (A + B * log(ThermistorResistance) + C * pow(log(ThermistorResistance), 3));

  temperature -= 273.15;

  Serial.print(temperature, 2);
  Serial.println("C"); 

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperature: ");
  lcd.setCursor(0, 1);
  lcd.print(temperature, 2); // Print on LCD with only 2 decimal places
  lcd.print("C");
  

  // Check the state of the button
  int reading = digitalRead(buttonPin);

  // Check if the button state has changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis(); // Reset the debounce timer
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has been stable for longer than the debounce delay
    if (reading != buttonState) {
      buttonState = reading;

      // Only toggle logging if the new button state is LOW
      if (buttonState == LOW) {
        logging = !logging;

        if (logging) {
          Serial.println("Logging started");
          dataFile = SD.open("data.txt", FILE_WRITE); 
        }
         
        else {
          Serial.println("Logging stopped");
          dataFile.close();
        }
      }
    }
  }

  // Save the reading. Next time through the loop, it'll be the lastButtonState
  lastButtonState = reading;

  // Logging data if logging is enabled
  if (logging) { 
      dataFile.print(millis() / 1000.0); // Time in seconds
      dataFile.print(", ");
      dataFile.println(temperature);
  }
delay(100);
}