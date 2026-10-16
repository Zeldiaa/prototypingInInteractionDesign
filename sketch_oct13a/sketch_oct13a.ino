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

#define BRIGHTNESS 200

// ---------------- NEOPIXELS ----------------

Adafruit_NeoPixel strip(
  PIXEL_COUNT,
  PIXEL_PIN,
  NEO_GRB + NEO_KHZ800
);

// ---------------- KNOCK SETTINGS ----------------

int knockValue = 0;
int knockThreshold = 100;

bool waitingForSecondKnock = false;
unsigned long firstKnockTime = 0;
unsigned long lastKnockTime = 0;

const unsigned long doubleTapWindow = 400;
const unsigned long knockCooldown = 200;

// ---------------- LIGHT SETTINGS ----------------

int mode = 0;  // 0 = lights off, 1 = rainbow

unsigned long lastRainbowUpdate = 0;
const unsigned long rainbowInterval = 20;

long firstPixelHue = 0;

// LED startup sequence
int startupPixel = 0;
bool startupSequenceActive = false;

unsigned long lastStartupUpdate = 0;
const unsigned long startupInterval = 100;  // Time between LEDs (ms)

// LED fade-out settings
bool fadingOut = false;
int fadeBrightness = BRIGHTNESS;

unsigned long lastFadeUpdate = 0;
const unsigned long fadeInterval = 15;
const int fadeStep = 1;

// ---------------- BUTTON SETTINGS ----------------

int lastButtonState = HIGH;
int buttonState = HIGH;

// ---------------- VIBRATION SETTINGS ----------------

int vibrationStrength = 0;
int fadeAmount = 1;

unsigned long lastVibrationUpdate = 0;
const int waveSpeed = 30;  // Smaller = faster waves

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
          fadingOut = true;
          fadeBrightness = BRIGHTNESS;
          lastFadeUpdate = now;

          startupSequenceActive = false;

          Serial.println("Lights fading out");
        }

        if (mode == 1) {
          firstPixelHue = 0;

          // Start the sequential LED startup
          startupPixel = 0;
          startupSequenceActive = true;
          lastStartupUpdate = now;

          strip.clear();
          strip.show();

          Serial.println("Lights starting");
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

if (mode == 1 && startupSequenceActive) {

    if (now - lastStartupUpdate >= startupInterval) {
      lastStartupUpdate = now;

      // Turn on one LED at a time
      int i = startupPixel;

      int phase = (i * 256 / 5 + firstPixelHue) % 256;

      int blend;
      if (phase < 128) {
        blend = phase * 255 / 128;
      } else {
        blend = (255 - phase) * 255 / 128;
      }

      int red = 255;
      int green = map(blend, 0, 255, 0, 65);
      int blue = 0;

      strip.setPixelColor(
        startupPixel,
        strip.Color(red, green, blue)
      );

      strip.show();

      startupPixel++;

      // Once every LED is on, start the animation
      if (startupPixel >= strip.numPixels()) {
        startupSequenceActive = false;
        lastRainbowUpdate = now;
      }
    }
  }

  // Normal animation begins after startup
  if (mode == 1 &&
      !startupSequenceActive &&
      now - lastRainbowUpdate >= rainbowInterval) {

    lastRainbowUpdate = now;

    for (int i = 0; i < strip.numPixels(); i++) {

      int phase = (i * 256 / 5 + firstPixelHue) % 256;

      int blend;
      if (phase < 128) {
        blend = phase * 255 / 128;
      } else {
        blend = (255 - phase) * 255 / 128;
      }

      int red = 255;
      int green = map(blend, 0, 255, 0, 65);
      int blue = 0;

      strip.setPixelColor(
        i,
        strip.Color(red, green, blue)
      );
    }

    strip.show();

    // Slow rotation
    firstPixelHue = (firstPixelHue + 1) % 256;
  }

  // -----------------------------------------------
// 5. FADE OUT LIGHTS WITHOUT BLOCKING THE LOOP
// -----------------------------------------------

if (fadingOut && now - lastFadeUpdate >= fadeInterval) {

  lastFadeUpdate = now;

  fadeBrightness -= fadeStep;

  if (fadeBrightness <= 0) {
    fadeBrightness = 0;

    strip.clear();
    strip.show();

    fadingOut = false;

    // Restore normal brightness for next time
    strip.setBrightness(BRIGHTNESS);

    Serial.println("Lights OFF");
  }
  else {
    strip.setBrightness(fadeBrightness);
    strip.show();
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
      Serial.println(vibrationStrength);

      if (vibrationStrength == 255) {
        Serial.println("Maximum");
        vibrationStrength = 254;
        fadeAmount = -1;
      }

      if (vibrationStrength <= 100) {
        Serial.println("Minimum");
        vibrationStrength = 101;
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
    //analogWrite(MOTOR_PIN, 100);
    vibrationStrength = 100;
    active = true;
  }
  return active;
}
