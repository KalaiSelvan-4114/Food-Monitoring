#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Define sensor pins
#define MQ4_PIN A0       // MQ-4 Gas Sensor
#define DHTPIN 7         // DHT Sensor Pin
#define DHTTYPE DHT11    // Change to DHT22 if needed
#define BUZZER_PIN 3     // Buzzer
#define GREEN_LED_PIN 4  // Green LED for Safe status
#define RED_LED_PIN 5    // Red LED for Spoiled status

// Define safe ranges
#define TEMP_MIN 20
#define TEMP_MAX 90
#define HUMIDITY_MIN 0
#define HUMIDITY_MAX 50
#define METHANE_MIN 300
#define METHANE_MAX 2000

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C LCD (address 0x27)

// Common I2C addresses for LCD
const uint8_t LCD_ADDRESSES[] = {0x27, 0x3F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
const int NUM_ADDRESSES = 9;

void setup() {
    Serial.begin(9600);
    dht.begin();
    
    // Initialize I2C
    Wire.begin();
    
    // Try to find the LCD
    bool lcdFound = false;
    for(int i = 0; i < NUM_ADDRESSES; i++) {
        Wire.beginTransmission(LCD_ADDRESSES[i]);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found LCD at address 0x");
            Serial.println(LCD_ADDRESSES[i], HEX);
            lcd = LiquidCrystal_I2C(LCD_ADDRESSES[i], 16, 2);
            lcdFound = true;
            break;
        }
    }
    
    if (!lcdFound) {
        Serial.println("No LCD found! Check connections:");
        Serial.println("1. SDA -> A4");
        Serial.println("2. SCL -> A5");
        Serial.println("3. VCC -> 5V");
        Serial.println("4. GND -> GND");
        Serial.println("5. I2C address might be different");
    } else {
        lcd.init();
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("System Starting...");
    }
    
    pinMode(MQ4_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    // Initialize LEDs to OFF state
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);

    Serial.println("Food Monitoring System Starting...");
}

bool isInSafeRange(float value, float min, float max) {
    return (value >= min && value <= max);
}

void loop() {
    // Read sensor data
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int methaneLevel = analogRead(MQ4_PIN);

    // Check if sensor readings are valid
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Sensor Error!");
        return;
    }

    // Check if all values are within safe ranges
    bool tempSafe = isInSafeRange(temperature, TEMP_MIN, TEMP_MAX);
    bool humiditySafe = isInSafeRange(humidity, HUMIDITY_MIN, HUMIDITY_MAX);
    bool methaneSafe = isInSafeRange(methaneLevel, METHANE_MIN, METHANE_MAX);

    // Determine food spoilage status
    String status = "Safe";
    if (!tempSafe || !humiditySafe || !methaneSafe) {
        status = "Spoiled";
        digitalWrite(GREEN_LED_PIN, LOW);    // Turn off green LED
        digitalWrite(RED_LED_PIN, HIGH);     // Turn on red LED
        digitalWrite(BUZZER_PIN, HIGH);      // Activate buzzer
    } else {
        digitalWrite(GREEN_LED_PIN, HIGH);   // Turn on green LED
        digitalWrite(RED_LED_PIN, LOW);      // Turn off red LED
        digitalWrite(BUZZER_PIN, LOW);       // Deactivate buzzer
    }

    // Display data on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature, 1);
    lcd.print("C H:");
    lcd.print(humidity, 1);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Gas:");
    lcd.print(methaneLevel);
    lcd.print(" ");
    lcd.print(status);

    // Send data to Python via Serial
    Serial.print(temperature);
    Serial.print(",");
    Serial.print(humidity);
    Serial.print(",");
    Serial.print(methaneLevel);
    Serial.print(",");
    Serial.println(status);

    delay(5000); // Update every 5 seconds
}
