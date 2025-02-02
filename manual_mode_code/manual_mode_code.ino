#include <WiFi.h>
#include <ESP32Servo.h>

Servo myservo;
static const int servoPin = 18;
static const int LDR_PIN = 34;
const float R_FIXED = 10000.0;
const int movementStep = 2;
const int delayBetweenSteps = 100;
const int minServoAngle = 0;
const int maxServoAngle = 180;
const int bufferZone = 5;
bool smartMode = true;
int currentServoPos = 90;

const char* ssid     = "shubh";
const char* password = "12345678";
WiFiServer server(80);
String header;

void setup() {
    Serial.begin(115200);
    myservo.attach(servoPin);
    myservo.write(currentServoPos);
    pinMode(LDR_PIN, INPUT);

    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected. IP Address: ");
    Serial.println(WiFi.localIP());
    server.begin();
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
        myservo.write(currentServoPos);
        Serial.print("Current Servo Position: ");
        Serial.println(currentServoPos);
        delay(delayBetweenSteps);
    }
}



void loop() {
    WiFiClient client = server.available();
    if (client) {
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                header += c;
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1'>");
                        client.println("<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js'></script>");
                        client.println("</head><body>");
                        client.println("<h1>ESP32 Smart Servo Control</h1>");
                        client.println("<p>Mode: <span id='mode'>" + String(smartMode ? "Smart" : "Manual") + "</span></p>");
                        client.println("<button onclick='toggleMode()'>Toggle Mode</button>");
                        client.println("<p>Position: <span id='servoPos'>" + String(currentServoPos) + "</span></p>");
                        client.println("<input type='range' min='0' max='180' class='slider' id='servoSlider' ");
                        client.println("onchange='setServo(this.value)' value='" + String(currentServoPos) + "' " + (smartMode ? "disabled" : "") + " />");
                        client.println("<script>");
                        client.println("function toggleMode() { $.get('/?mode=toggle', function(){ location.reload(); }); }");
                        client.println("function setServo(pos) { $.get('/?value=' + pos, function(){ $('#servoPos').text(pos); }); }");
                        client.println("</script></body></html>");
                        client.println();

                        if (header.indexOf("GET /?mode=toggle") >= 0) {
                            smartMode = !smartMode;
                        }
                        if (!smartMode && header.indexOf("GET /?value=") >= 0) {
                            int pos1 = header.indexOf('=');
                            String valueString = header.substring(pos1 + 1);
                            moveServo(valueString.toInt());
                        }
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        header = "";
        client.stop();
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
