#include <ESP32Servo.h>

const int servoPin = 18;
const int LDR_PIN = 2;
const float R_FIXED = 10000.0;
const int movementStep = 4;
const int delayBetweenSteps = 100;
const int minServoAngle = 0;
const int maxServoAngle = 180;
const int bufferZone = 5;

Servo blindServo;
int currentServoPos = 90;
bool smartMode = true;  

void setup() {
    Serial.begin(115200);
    blindServo.attach(servoPin);
    blindServo.write(currentServoPos);
    pinMode(LDR_PIN, INPUT);
    Serial.println("ESP32 Ready - Waiting for commands...");
}

void moveServo(int targetPos) {
    Serial.print("Moving servo to target position: ");
    Serial.println(targetPos);

    while (currentServoPos != targetPos) {
        if (currentServoPos < targetPos) {
            currentServoPos += movementStep;
            if (currentServoPos > targetPos) currentServoPos = targetPos;
        } else {
            currentServoPos -= movementStep;
            if (currentServoPos < targetPos) currentServoPos = targetPos;
        }
        blindServo.write(currentServoPos);
        Serial.print("Current Servo Position: ");
        Serial.println(currentServoPos);
        delay(delayBetweenSteps);
    }
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        Serial.print("Received Command: ");
        Serial.println(command);

        if (command == "OPEN") {
            smartMode = false;
            Serial.println("Executing: Open Blinds");
            moveServo(maxServoAngle);
        } 
        else if (command == "CLOSE") {
            smartMode = false;
            Serial.println("Executing: Close Blinds");
            moveServo(minServoAngle);
        } 
        else if (command == "SMART") {
            smartMode = true;
            Serial.println("Executing: Smart Mode Activated");
        } 
        else if (command.startsWith("ANGLE")) {
            smartMode = false;
            int angle = command.substring(6).toInt();
            Serial.print("Executing: Set Servo Angle to ");
            Serial.println(angle);
            moveServo(angle);
        }
    }

    if (smartMode) {
        int adcValue = analogRead(LDR_PIN);
        float voltage = (adcValue / 4095.0) * 3.3;
        float ldrResistance = R_FIXED * ((3.3 / voltage) - 1);

        Serial.print("ADC: ");
        Serial.print(adcValue);
        Serial.print(" | Voltage: ");
        Serial.print(voltage, 2);
        Serial.print("V | LDR Resistance: ");
        Serial.print(ldrResistance, 2);
        Serial.println(" Ω");

        int targetServoPos = map(ldrResistance, 15000, 30000, minServoAngle, maxServoAngle);
        targetServoPos = constrain(targetServoPos, minServoAngle, maxServoAngle);

        if (abs(targetServoPos - currentServoPos) > bufferZone) {
            Serial.print("Smart Adjusting Servo from ");
            Serial.print(currentServoPos);
            Serial.print("° to ");
            Serial.print(targetServoPos);
            Serial.println("°");
            moveServo(targetServoPos);
        } else {
            Serial.println("Blinds Hold Position (Stable Light Level)");
        }
    }

    delay(1000);
}
