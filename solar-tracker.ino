#include <Servo.h>

Servo horizontalServo;
Servo verticalServo;

// LDR analog pins
const int TL = A0;
const int TR = A1;
const int BL = A2;
const int BR = A3;

// Servo pins
const int H_SERVO = 9;
const int V_SERVO = 10;

// Starting positions
int horizontalPos = 90;
int verticalPos = 90;

// Movement settings
const int tolerance = 50;
const int stepSize = 1;

const int minAngle = 15;
const int maxAngle = 165;

void setup() {
  Serial.begin(9600);

  horizontalServo.attach(H_SERVO);
  verticalServo.attach(V_SERVO);

  horizontalServo.write(horizontalPos);
  verticalServo.write(verticalPos);

  delay(1000);
}

void loop() {
  // Read photoresistors
  int tl = analogRead(TL);
  int tr = analogRead(TR);
  int bl = analogRead(BL);
  int br = analogRead(BR);

  // Average readings by direction
  int top = (tl + tr) / 2;
  int bottom = (bl + br) / 2;
  int left = (tl + bl) / 2;
  int right = (tr + br) / 2;

  int horizontalError = left - right;
  int verticalError = top - bottom;

  // Horizontal tracking
  if (abs(horizontalError) > tolerance) {
    if (horizontalError > 0) {
      horizontalPos += stepSize;
    } else {
      horizontalPos -= stepSize;
    }
  }

  // Vertical tracking
  if (abs(verticalError) > tolerance) {
    if (verticalError > 0) {
      verticalPos += stepSize;
    } else {
      verticalPos -= stepSize;
    }
  }

  // Restrict movement
  horizontalPos = constrain(
    horizontalPos, minAngle, maxAngle
  );

  verticalPos = constrain(
    verticalPos, minAngle, maxAngle
  );

  // Update servos
  horizontalServo.write(horizontalPos);
  verticalServo.write(verticalPos);

  // Debugging
  Serial.print("TL: ");
  Serial.print(tl);
  Serial.print(" TR: ");
  Serial.print(tr);
  Serial.print(" BL: ");
  Serial.print(bl);
  Serial.print(" BR: ");
  Serial.println(br);

  Serial.print("Horizontal: ");
  Serial.print(horizontalPos);
  Serial.print(" Vertical: ");
  Serial.println(verticalPos);

  delay(100);
}
