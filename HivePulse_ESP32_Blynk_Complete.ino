// ======================================================
// HIVEPULSE - COMPLETE WORKING INTEGRATED CODE
// ESP32 + DHT11 + MPU6050 + MIC + POT
// LCD + 3 LEDs + BUZZER + WiFi + Blynk
// ======================================================


// ======================================================
// BLYNK DETAILS
// ======================================================

#define BLYNK_TEMPLATE_ID "TMPL3r2_DEib3"
#define BLYNK_TEMPLATE_NAME "Hive Pulse"
#define BLYNK_AUTH_TOKEN "OJanv4QWExJGRzX8HH8vlTKYZxvsCbE2"


// ======================================================
// LIBRARIES
// ======================================================

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <math.h>


// ======================================================
// WIFI DETAILS
// CHANGE ONLY THESE IF HOTSPOT DETAILS CHANGE
// ======================================================

char ssid[] = "not for you";
char pass[] = "qwertyui";


// ======================================================
// PIN CONNECTIONS
// ======================================================

#define DHT_PIN       27
#define DHT_TYPE      DHT11

#define MIC_PIN       34
#define ENERGY_PIN    35

#define GREEN_LED     13
#define YELLOW_LED    12
#define RED_LED       14

#define BUZZER_PIN    25

#define SDA_PIN       21
#define SCL_PIN       22


// ======================================================
// DEVICES
// ======================================================

DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define MPU_ADDR 0x68


// ======================================================
// THRESHOLDS
// LOW ENERGY = LESS SENSITIVE
// HIGH ENERGY = MORE SENSITIVE
// ======================================================

const int SOUND_THRESHOLD_LOW  = 12;
const int SOUND_THRESHOLD_MED  = 7;
const int SOUND_THRESHOLD_HIGH = 4;

const int VIB_THRESHOLD_LOW  = 1200;
const int VIB_THRESHOLD_MED  = 800;
const int VIB_THRESHOLD_HIGH = 350;


// ======================================================
// VARIABLES
// ======================================================

float temperature = 0;
float humidity = 0;

unsigned long lastDHTRead = 0;
unsigned long lastBlynkSend = 0;
unsigned long lastLCDUpdate = 0;

unsigned long alertUntil = 0;

String previousState = "";


// ======================================================
// CONNECT WIFI
// ======================================================

void connectWiFi() {

  Serial.println();
  Serial.println("Connecting to WiFi...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONNECTING WIFI");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long startAttempt = millis();

  // Try for 20 seconds
  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startAttempt < 20000
  ) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WIFI CONNECTED!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WIFI CONNECTED");

    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    delay(1500);

  }

  else {

    Serial.println("WIFI FAILED!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WIFI FAILED");

    lcd.setCursor(0, 1);
    lcd.print("LOCAL MODE");

    delay(1500);
  }
}


// ======================================================
// CONNECT BLYNK
// ======================================================

void connectBlynk() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("Skipping Blynk - No WiFi");

    return;
  }

  Serial.println("Connecting to Blynk...");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("CONNECT BLYNK");

  Blynk.config(BLYNK_AUTH_TOKEN);

  if (Blynk.connect(10000)) {

    Serial.println("BLYNK CONNECTED!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("BLYNK CONNECTED");

    delay(1000);

  }

  else {

    Serial.println("BLYNK FAILED!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("BLYNK FAILED");

    lcd.setCursor(0, 1);
    lcd.print("LOCAL MODE");

    delay(1500);
  }
}


// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("============================");
  Serial.println("      HIVEPULSE STARTING");
  Serial.println("============================");


  // --------------------------------------------------
  // LED SETUP
  // --------------------------------------------------

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);


  // --------------------------------------------------
  // BUZZER
  // --------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);


  // --------------------------------------------------
  // ADC
  // --------------------------------------------------

  analogReadResolution(12);


  // --------------------------------------------------
  // I2C
  // LCD + MPU6050 SHARE GPIO21 / GPIO22
  // --------------------------------------------------

  Wire.begin(SDA_PIN, SCL_PIN);


  // --------------------------------------------------
  // LCD
  // --------------------------------------------------

  lcd.init();

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("HIVEPULSE");

  lcd.setCursor(0, 1);
  lcd.print("STARTING...");


  // --------------------------------------------------
  // DHT
  // --------------------------------------------------

  dht.begin();


  // --------------------------------------------------
  // MPU6050
  // --------------------------------------------------

  Wire.beginTransmission(MPU_ADDR);

  Wire.write(0x6B);

  Wire.write(0x00);

  byte mpuStatus =
    Wire.endTransmission();


  if (mpuStatus == 0) {

    Serial.println("MPU6050 CONNECTED");

  }

  else {

    Serial.println("MPU6050 NOT FOUND");
  }


  delay(1000);


  // --------------------------------------------------
  // WIFI
  // --------------------------------------------------

  connectWiFi();


  // --------------------------------------------------
  // BLYNK
  // --------------------------------------------------

  connectBlynk();


  // --------------------------------------------------
  // SYSTEM READY
  // --------------------------------------------------

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("HIVEPULSE");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  digitalWrite(GREEN_LED, HIGH);


  Serial.println();
  Serial.println("============================");
  Serial.println("    HIVEPULSE SYSTEM READY");
  Serial.println("============================");


  delay(1500);
}


// ======================================================
// MICROPHONE
// ======================================================

int readSoundActivity() {

  int minimum = 4095;

  int maximum = 0;

  unsigned long startTime =
    millis();


  // Sample microphone for 50 ms

  while (millis() - startTime < 50) {

    int value =
      analogRead(MIC_PIN);


    if (value < minimum) {

      minimum = value;
    }


    if (value > maximum) {

      maximum = value;
    }
  }


  return maximum - minimum;
}


// ======================================================
// MPU6050 ACCELERATION
// ======================================================

long getAccelerationMagnitude() {

  Wire.beginTransmission(MPU_ADDR);

  Wire.write(0x3B);


  if (Wire.endTransmission(false) != 0) {

    return 16384;
  }


  Wire.requestFrom(
    MPU_ADDR,
    6,
    true
  );


  if (Wire.available() < 6) {

    return 16384;
  }


  int16_t ax =
    (Wire.read() << 8) |
    Wire.read();


  int16_t ay =
    (Wire.read() << 8) |
    Wire.read();


  int16_t az =
    (Wire.read() << 8) |
    Wire.read();


  float magnitude =
    sqrt(
      (float)ax * ax +
      (float)ay * ay +
      (float)az * az
    );


  return (long)magnitude;
}


// ======================================================
// VIBRATION ACTIVITY
// ======================================================

long readVibrationActivity() {

  long minimum = 100000;

  long maximum = 0;

  unsigned long startTime =
    millis();


  // Sample for 100 ms

  while (millis() - startTime < 100) {

    long magnitude =
      getAccelerationMagnitude();


    if (magnitude < minimum) {

      minimum = magnitude;
    }


    if (magnitude > maximum) {

      maximum = magnitude;
    }


    delay(2);
  }


  return maximum - minimum;
}


// ======================================================
// LED CONTROL
// ======================================================

void setLEDs(
  bool green,
  bool yellow,
  bool red
) {

  digitalWrite(
    GREEN_LED,
    green ? HIGH : LOW
  );


  digitalWrite(
    YELLOW_LED,
    yellow ? HIGH : LOW
  );


  digitalWrite(
    RED_LED,
    red ? HIGH : LOW
  );
}


// ======================================================
// LCD
// ======================================================

void updateLCD(
  String state,
  int energyPercent,
  int sound,
  long vibration
) {

  // Don't refresh constantly

  if (
    millis() - lastLCDUpdate < 500 &&
    state == previousState
  ) {

    return;
  }


  lastLCDUpdate =
    millis();

  previousState =
    state;


  lcd.clear();


  // --------------------------------------------------
  // SENTINEL
  // --------------------------------------------------

  if (state == "SENTINEL") {

    lcd.setCursor(0, 0);

    lcd.print("HIVE NORMAL");


    lcd.setCursor(0, 1);

    lcd.print("E:");

    lcd.print(energyPercent);

    lcd.print("% ");


    lcd.print("T:");

    lcd.print((int)temperature);
  }


  // --------------------------------------------------
  // INVESTIGATE
  // --------------------------------------------------

  else if (state == "INVESTIGATE") {

    lcd.setCursor(0, 0);

    lcd.print("INVESTIGATING");


    lcd.setCursor(0, 1);

    lcd.print("S:");

    lcd.print(sound);

    lcd.print(" V:");

    lcd.print(vibration);
  }


  // --------------------------------------------------
  // ALERT
  // --------------------------------------------------

  else {

    lcd.setCursor(0, 0);

    lcd.print("HIVE ANOMALY!");


    lcd.setCursor(0, 1);

    lcd.print("CHECK HIVE");
  }
}


// ======================================================
// LOOP
// ======================================================

void loop() {


  // ==================================================
  // BLYNK
  // ==================================================

  if (Blynk.connected()) {

    Blynk.run();
  }


  // ==================================================
  // 1. ENERGY
  // ==================================================

  int rawEnergy =
    analogRead(ENERGY_PIN);


  int energyPercent =
    map(
      rawEnergy,
      0,
      4095,
      0,
      100
    );


  energyPercent =
    constrain(
      energyPercent,
      0,
      100
    );


  // ==================================================
  // 2. DHT11
  // ==================================================

  if (
    millis() - lastDHTRead >= 2000
  ) {

    lastDHTRead =
      millis();


    float newHumidity =
      dht.readHumidity();


    float newTemperature =
      dht.readTemperature();


    if (!isnan(newHumidity)) {

      humidity =
        newHumidity;
    }


    if (!isnan(newTemperature)) {

      temperature =
        newTemperature;
    }
  }


  // ==================================================
  // 3. SOUND
  // ==================================================

  int sound =
    readSoundActivity();


  // ==================================================
  // 4. VIBRATION
  // ==================================================

  long vibration =
    readVibrationActivity();


  // ==================================================
  // 5. ENERGY-ADAPTIVE THRESHOLDS
  // ==================================================

  int soundThreshold;

  int vibrationThreshold;

  String energyMode;


  // --------------------------------------------------
  // LOW ENERGY
  // --------------------------------------------------

  if (energyPercent < 30) {

    energyMode = "LOW";


    soundThreshold =
      SOUND_THRESHOLD_LOW;


    vibrationThreshold =
      VIB_THRESHOLD_LOW;
  }


  // --------------------------------------------------
  // MEDIUM ENERGY
  // --------------------------------------------------

  else if (energyPercent < 70) {

    energyMode = "MEDIUM";


    soundThreshold =
      SOUND_THRESHOLD_MED;


    vibrationThreshold =
      VIB_THRESHOLD_MED;
  }


  // --------------------------------------------------
  // HIGH ENERGY
  // --------------------------------------------------

  else {

    energyMode = "HIGH";


    soundThreshold =
      SOUND_THRESHOLD_HIGH;


    vibrationThreshold =
      VIB_THRESHOLD_HIGH;
  }


  // ==================================================
  // 6. EVENT DETECTION
  // ==================================================

  bool soundEvent =
    sound > soundThreshold;


  bool vibrationEvent =
    vibration > vibrationThreshold;


  // ==================================================
  // 7. STATE MACHINE
  // ==================================================

  String state;


  // BOTH SOUND + VIBRATION

  if (
    soundEvent &&
    vibrationEvent
  ) {

    state = "ALERT";


    // Hold alert for 3 seconds

    alertUntil =
      millis() + 3000;


    Serial.println();
    Serial.println(
      "*** MULTIMODAL HIVE ANOMALY ***"
    );

    Serial.println(
      "Inspection recommended"
    );
  }


  // Keep alert visible

  else if (
    millis() < alertUntil
  ) {

    state = "ALERT";
  }


  // ONE SENSOR ONLY

  else if (
    soundEvent ||
    vibrationEvent
  ) {

    state = "INVESTIGATE";
  }


  // NORMAL

  else {

    state = "SENTINEL";
  }


  // ==================================================
  // 8. LEDs + BUZZER
  // ==================================================

  if (state == "SENTINEL") {

    setLEDs(
      true,
      false,
      false
    );


    digitalWrite(
      BUZZER_PIN,
      LOW
    );
  }


  else if (
    state == "INVESTIGATE"
  ) {

    setLEDs(
      false,
      true,
      false
    );


    digitalWrite(
      BUZZER_PIN,
      LOW
    );
  }


  else {

    setLEDs(
      false,
      false,
      true
    );


    digitalWrite(
      BUZZER_PIN,
      HIGH
    );
  }


  // ==================================================
  // 9. LCD
  // ==================================================

  updateLCD(
    state,
    energyPercent,
    sound,
    vibration
  );


  // ==================================================
  // 10. BLYNK DASHBOARD
  //
  // V0 = Temperature
  // V1 = Humidity
  // V2 = Vibration
  // V3 = Sound
  // V4 = Energy
  // ==================================================

  if (
    Blynk.connected() &&
    millis() - lastBlynkSend >= 1000
  ) {

    lastBlynkSend =
      millis();


    Blynk.virtualWrite(
      V0,
      temperature
    );


    Blynk.virtualWrite(
      V1,
      humidity
    );


    Blynk.virtualWrite(
      V2,
      vibration
    );


    Blynk.virtualWrite(
      V3,
      sound
    );


    Blynk.virtualWrite(
      V4,
      energyPercent
    );
  }


  // ==================================================
  // 11. SERIAL MONITOR
  // ==================================================

  Serial.println();
  Serial.println(
    "========= HIVEPULSE ========="
  );


  Serial.print(
    "Temperature : "
  );

  Serial.print(
    temperature
  );

  Serial.println(" C");


  Serial.print(
    "Humidity    : "
  );

  Serial.print(
    humidity
  );

  Serial.println(" %");


  Serial.print(
    "Sound       : "
  );

  Serial.print(
    sound
  );

  Serial.print(
    " / Threshold "
  );

  Serial.println(
    soundThreshold
  );


  Serial.print(
    "Vibration   : "
  );

  Serial.print(
    vibration
  );

  Serial.print(
    " / Threshold "
  );

  Serial.println(
    vibrationThreshold
  );


  Serial.print(
    "Energy Raw  : "
  );

  Serial.println(
    rawEnergy
  );


  Serial.print(
    "Energy      : "
  );

  Serial.print(
    energyPercent
  );

  Serial.println("%");


  Serial.print(
    "Energy Mode : "
  );

  Serial.println(
    energyMode
  );


  Serial.print(
    "WiFi        : "
  );

  Serial.println(
    WiFi.status() == WL_CONNECTED
    ? "CONNECTED"
    : "NOT CONNECTED"
  );


  Serial.print(
    "Blynk       : "
  );

  Serial.println(
    Blynk.connected()
    ? "CONNECTED"
    : "NOT CONNECTED"
  );


  Serial.print(
    "Sound Event : "
  );

  Serial.println(
    soundEvent
    ? "YES"
    : "NO"
  );


  Serial.print(
    "Vib Event   : "
  );

  Serial.println(
    vibrationEvent
    ? "YES"
    : "NO"
  );


  Serial.print(
    "STATE       : "
  );

  Serial.println(
    state
  );


  Serial.println(
    "============================="
  );


  delay(150);
}
