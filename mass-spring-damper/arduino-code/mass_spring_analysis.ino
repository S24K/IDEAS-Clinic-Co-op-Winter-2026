#include <math.h>

// -------------------------
// Ultrasonic sensor pins
// -------------------------
const int TRIG_PIN = 12;
const int ECHO_PIN = 13;

// -------------------------
// Rotary encoder settings
// -------------------------
#define CLICKS_PER_REVOLUTION 700
#define PI 3.1415926
#define ARM_LENGTH 1.3 // cm

const int ENCODER_A = 2;
const int ENCODER_B = 3;

// -------------------------
// Motor driver pin
// -------------------------
const int MOTOR_DRIVER_IN1 = 10;

// -------------------------
// Ultrasonic calibration
// -------------------------
// Leave at 1.0 for now, fix later after noise is reduced
double ultrasonicScale = 1.0;

// Choose sign based on your setup:
// +1  -> displacement = (rawDistance - initDist)
// -1  -> displacement = (initDist - rawDistance)
int ultrasonicSign = 1;

// -------------------------
// Ultrasonic timing/filtering
// -------------------------
unsigned long lastUltrasonicMicros = 0;
const unsigned long ultrasonicPeriod = 50000; // 50 ms = 20 Hz

double ultrasonicAlpha = 0.12;     // lower = smoother but more lag
double maxJumpCm = 1.5;            // reject sudden unrealistic jumps

// -------------------------
// Encoder data
// -------------------------
volatile int encoderPos = 0;
int prevEncoderPos = 0;
double armHeight = 0;

// -------------------------
// Distance variables
// -------------------------
double initDist = 0;       // absolute distance at startup
double filteredDist = 0;   // filtered displacement relative to initDist
double rawDist = 0;        // latest absolute raw distance
double dispDist = 0;       // latest relative displacement

// -------------------------
// Shared time variable
// -------------------------
unsigned long currentTime = 0;

// -------------------------
// Helper: median of 3 values
// -------------------------
double median3(double a, double b, double c) {
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

// -------------------------
// Single ultrasonic reading
// Returns distance in cm, or NAN if timeout/no echo
// -------------------------
double measureDistanceOnce() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 8000);

  if (duration == 0) {
    return NAN;
  }

  return duration * 0.0343 / 2.0;
}

// -------------------------
// Median-filtered ultrasonic reading
// Takes 3 readings spaced apart
// Returns median in cm, or NAN if any fail
// -------------------------
double measureDistanceMedian() {
  double a = measureDistanceOnce();
  delay(25);

  double b = measureDistanceOnce();
  delay(25);

  double c = measureDistanceOnce();

  if (isnan(a) || isnan(b) || isnan(c)) {
    return NAN;
  }

  return median3(a, b, c);
}

// -------------------------
// Get stable initial zero reference
// Averaging several median-filtered measurements
// -------------------------
double getInitialDistance() {
  const int N = 10;
  double sum = 0;
  int validCount = 0;

  for (int i = 0; i < N; i++) {
    double d = measureDistanceMedian();

    if (!isnan(d)) {
      sum += d;
      validCount++;
    }

    delay(30);
  }

  if (validCount == 0) {
    return 0;
  }

  return sum / validCount;
}

// -------------------------
// Setup
// -------------------------
void setup() {
  Serial.begin(500000);
  delay(1000);

  // Ultrasonic setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Encoder setup
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  digitalWrite(ENCODER_A, HIGH);
  digitalWrite(ENCODER_B, HIGH);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), updateEncoder, RISING);

  // Motor driver setup
  pinMode(MOTOR_DRIVER_IN1, OUTPUT);

  // Get startup zero point
  initDist = getInitialDistance();

  // Start displacement at zero
  filteredDist = 0;
  rawDist = initDist;
  dispDist = 0;
}

// -------------------------
// Main loop
// -------------------------
void loop() {
  currentTime = micros();

  motorSpeedControl();

  // Ultrasonic update only at chosen rate
  if (currentTime - lastUltrasonicMicros >= ultrasonicPeriod) {
    lastUltrasonicMicros = currentTime;

    double measured = measureDistanceMedian();

    if (!isnan(measured)) {
      rawDist = measured;

      // Relative displacement from startup reference
      dispDist = ultrasonicSign * (rawDist - initDist) * ultrasonicScale;

      // Reject sudden bad jumps before filtering
      if (abs(dispDist - filteredDist) < maxJumpCm) {
        filteredDist = filteredDist * (1.0 - ultrasonicAlpha) + ultrasonicAlpha * dispDist;
      }
    }
  }

  calculateArmHeight();

  // Print:
  // time(s) rawDistance(cm) displacement(cm) filteredDisplacement(cm) armHeight(cm)
  Serial.print(currentTime / 1e6, 6);
  Serial.print(" ");
  Serial.print(rawDist, 4);
  Serial.print(" ");
  Serial.print(dispDist, 4);
  Serial.print(" ");
  Serial.print(filteredDist, 4);
  Serial.print(" ");
  Serial.println(armHeight, 4);
}

// -------------------------
// Updates current arm height from encoder
// -------------------------
void calculateArmHeight() {
  noInterrupts();
  int currentPos = encoderPos;
  interrupts();

  if (currentPos != prevEncoderPos) {
    double angle = 2 * PI * currentPos / (double)CLICKS_PER_REVOLUTION;
    armHeight = 0.1 * sin(angle);
    prevEncoderPos = currentPos;
  }
}

// -------------------------
// Encoder ISR
// -------------------------
void updateEncoder() {
  if (digitalRead(ENCODER_B) == LOW) {
    encoderPos++;
  } else {
    encoderPos--;
  }
}

// -------------------------
// Motor speed control from potentiometer
// -------------------------
void motorSpeedControl() {
  int rawPotValue = analogRead(A2);

  // input range (0-715) based on your 3.3 V potentiometer setup
  int PWM_PotValue = map(rawPotValue, 0, 715, 0, 255);

  analogWrite(MOTOR_DRIVER_IN1, PWM_PotValue);
}
