#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

// ---------------- PIN SETTINGS ----------------

#define KNOCK_PIN   A0
#define PIXEL_PIN   6
#define PIXEL_COUNT 16

#define BUTTON_PIN  7
#define MOTOR_PIN   3

#define BRIGHTNESS 30

// ---------------- NEOPIXELS ----------------

Adafruit_NeoPixel strip(
  PIXEL_COUNT,
  PIXEL_PIN,
  NEO_GRB + NEO_KHZ800
);

// ---------------- KNOCK SETTINGS ----------------

int knockValue = 0;
int knockThreshold = 50;

bool waitingForSecondKnock = false;
unsigned long firstKnockTime = 0;
unsigned long lastKnockTime = 0;

const unsigned long doubleTapWindow = 400;
const unsigned long knockCooldown = 200;

// ---------------- LIGHT SETTINGS ----------------

int mode = 0;  // 0 = lights off, 1 = rainbow

unsigned long lastRainbowUpdate = 0;
const unsigned long rainbowInterval = 10;

long firstPixelHue = 0;

// ---------------- BUTTON SETTINGS ----------------

int lastButtonState = HIGH;
int buttonState = HIGH;

// ---------------- VIBRATION SETTINGS ----------------

int vibrationStrength = 0;
int fadeAmount = 1;

unsigned long lastVibrationUpdate = 0;
const int waveSpeed = 40;  // Smaller = faster waves

bool vibrationActive = false;

// ==================================================
// SETUP
// ==================================================

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  Serial.begin(9600);

  // Button connected between pin 7 and GND
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Vibration motor control output
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
}

// ==================================================
// MAIN LOOP
// ==================================================


void loop() {
  unsigned long now = millis();

  // -----------------------------------------------
  // 1. BUTTON AND VIBRATION MOTOR
  // -----------------------------------------------

  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    vibrationActive = switchVibrationState(vibrationActive);
    Serial.println(vibrationActive);
  }

  




lastButtonState = buttonState;


  // -----------------------------------------------
  // 2. KNOCK SENSOR
  // -----------------------------------------------

  knockValue = analogRead(KNOCK_PIN);

  if (knockValue >= knockThreshold &&
      now - lastKnockTime > knockCooldown) {

    lastKnockTime = now;

    if (!waitingForSecondKnock) {
      // First knock detected
      waitingForSecondKnock = true;
      firstKnockTime = now;

      Serial.println("First knock!");
    }
    else {
      // Second knock within the allowed time
      if (now - firstKnockTime <= doubleTapWindow) {

        Serial.println("DOUBLE TAP!");

        // Toggle the light mode
        mode = 1 - mode;

        if (mode == 0) {
          strip.clear();
          strip.show();
          Serial.println("Lights OFF");
        }

        if (mode == 1) {
          firstPixelHue = 0;
          Serial.println("Rainbow ON");
        }

        waitingForSecondKnock = false;
      }
    }
  }

  // -----------------------------------------------
  // 3. SINGLE KNOCK TIMEOUT
  // -----------------------------------------------

  if (waitingForSecondKnock &&
      now - firstKnockTime > doubleTapWindow) {

    Serial.println("Single knock");

    waitingForSecondKnock = false;
  }

  // -----------------------------------------------
  // 4. UPDATE LIGHTS WITHOUT BLOCKING THE LOOP
  // -----------------------------------------------

  if (mode == 1 &&
      now - lastRainbowUpdate >= rainbowInterval) {

    lastRainbowUpdate = now;

    for (int i = 0; i < strip.numPixels(); i++) {

      int pixelHue =
        firstPixelHue +
        (i * 65536L / strip.numPixels());

      strip.setPixelColor(
        i,
        strip.gamma32(strip.ColorHSV(pixelHue))
      );
    }

    strip.show();

    firstPixelHue += 256;

    if (firstPixelHue >= 3L * 65536L) {
      firstPixelHue = 0;
    }
  }

  // No long delay here:
  // the button, motor, sensor and lights stay responsive.

  //If the vibration is active, keep updating. Otherwise do nothing
  if (vibrationActive) {
    updateVibration();
  }
}

void updateVibration() {
    unsigned long now = millis();

    if (now - lastVibrationUpdate >= waveSpeed) {
      lastVibrationUpdate = now;

      vibrationStrength += fadeAmount;

      if (vibrationStrength == 255) {
        Serial.println("Maximum");
        vibrationStrength = 254;
        fadeAmount = -1;
      }

      if (vibrationStrength == 0) {
        Serial.println("Minimum");
        vibrationStrength = 1;
        fadeAmount = 1;
      }

      analogWrite(MOTOR_PIN, vibrationStrength);
    }
  }

//Update vibration state of the vibration motor
bool switchVibrationState(bool active) {
  if (active) {
    analogWrite(MOTOR_PIN, 0);
    active = false;
  }
  else {
    active = true;
  }
  return active;
}
