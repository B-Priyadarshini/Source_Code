#include <ESP32Servo.h>
#include <TinyGPSPlus.h>

#define TRIG_PIN 25
#define ECHO_PIN 18
#define SERVO_PIN 19
#define VIBRATOR_PIN 21
#define PLAYE_PIN 23
#define BUTTON_PIN 27

#define GSM_RX 16
#define GSM_TX 17

HardwareSerial SIM900A(2);

#define GPS_RX 4
#define GPS_TX 5

HardwareSerial GPS(1);

TinyGPSPlus gps;

Servo myServo;

#define DETECTION_DISTANCE 30

#define SERVO_HOME 0
#define SERVO_ALERT 120

const char PHONE_NUMBER_1[] = "+919080569101";
const char PHONE_NUMBER_2[] = "+916381613820";


bool lastButtonState = HIGH;

unsigned long lastButtonTime = 0;

const unsigned long debounceTime = 300;

float getDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
  {
    return 999;
  }

  float distance = duration * 0.0343 / 2.0;

  return distance;
}

void playVoice()
{
  digitalWrite(PLAYE_PIN, HIGH);

  delay(200);

  digitalWrite(PLAYE_PIN, LOW);
}

void sendAT(const char *command, unsigned long waitTime)
{
  Serial.print(">> ");
  Serial.println(command);

  SIM900A.println(command);

  unsigned long startTime = millis();

  while (millis() - startTime < waitTime)
  {
    while (SIM900A.available())
    {
      Serial.write(SIM900A.read());
    }
  }

  Serial.println();
}

void sendSMS(const char *number, const char *message)
{
  Serial.print("Sending SMS to: ");
  Serial.println(number);

  SIM900A.println("AT+CMGF=1");

  delay(1000);

  SIM900A.print("AT+CMGS=\"");
  SIM900A.print(number);
  SIM900A.println("\"");

  delay(1500);

  SIM900A.print(message);

  SIM900A.write(26);

  delay(5000);

  Serial.println("SMS sent.");
}

void makeCall(const char *number)
{
  Serial.print("Calling: ");
  Serial.println(number);

  SIM900A.print("ATD");
  SIM900A.print(number);
  SIM900A.println(";");

  unsigned long startTime = millis();

  while (millis() - startTime < 15000)
  {
    while (GPS.available())
    {
      gps.encode(GPS.read());
    }

    while (SIM900A.available())
    {
      Serial.write(SIM900A.read());
    }

    delay(10);
  }

  SIM900A.println("ATH");

  delay(1000);

  Serial.println("Call ended.");
}

void emergencyAlert()
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("     EMERGENCY BUTTON PRESSED");
  Serial.println("========================================");

  Serial.println("Waiting for GPS location...");

  unsigned long startTime = millis();

  while (!gps.location.isValid() &&
         millis() - startTime < 10000)
  {
    while (GPS.available())
    {
      gps.encode(GPS.read());
    }

    delay(10);
  }

  if (gps.location.isValid())
  {
    double latitude = gps.location.lat();
    double longitude = gps.location.lng();

    Serial.println("GPS FIX FOUND");

    Serial.print("Latitude: ");
    Serial.println(latitude, 6);

    Serial.print("Longitude: ");
    Serial.println(longitude, 6);

    char smsMessage[250];

    snprintf(
      smsMessage,
      sizeof(smsMessage),
      "EMERGENCY ALERT! "
      "Emergency button pressed. "
      "Location: https://maps.google.com/?q=%.6f,%.6f",
      latitude,
      longitude
    );

    sendSMS(PHONE_NUMBER_1, smsMessage);

    delay(3000);

    sendSMS(PHONE_NUMBER_2, smsMessage);

    delay(3000);

    sendSMS(PHONE_NUMBER_3, smsMessage);

    delay(3000);
  }
  else
  {
    Serial.println("GPS LOCATION NOT AVAILABLE");

    const char noGPSMessage[] =
      "EMERGENCY ALERT! "
      "Emergency button pressed. "
      "GPS location unavailable.";

    sendSMS(PHONE_NUMBER_1, noGPSMessage);

    delay(3000);

    sendSMS(PHONE_NUMBER_2, noGPSMessage);

    delay(3000);

    sendSMS(PHONE_NUMBER_3, noGPSMessage);

    delay(3000);
  }

  makeCall(PHONE_NUMBER_1);

  delay(3000);

  makeCall(PHONE_NUMBER_2);

  delay(3000);

  makeCall(PHONE_NUMBER_3);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" SMS + CALL COMPLETED FOR ALL 3 NUMBERS");
  Serial.println("========================================");
}

void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(VIBRATOR_PIN, OUTPUT);

  digitalWrite(VIBRATOR_PIN, LOW);

  pinMode(PLAYE_PIN, OUTPUT);

  digitalWrite(PLAYE_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  myServo.attach(SERVO_PIN);

  myServo.write(SERVO_HOME);

  SIM900A.begin(
    9600,
    SERIAL_8N1,
    GSM_RX,
    GSM_TX
  );

  GPS.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );

  delay(3000);

  Serial.println("Initializing SIM900A...");

  sendAT("AT", 1000);
  sendAT("AT+CPIN?", 1000);
  sendAT("AT+CSQ", 1000);
  sendAT("AT+CREG?", 1000);
  sendAT("AT+CMGF=1", 1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32 SAFETY SYSTEM READY");
  Serial.println("========================================");
  Serial.println("Obstacle < 30 cm:");
  Serial.println("  Servo + Vibration + Voice ONLY");
  Serial.println();
  Serial.println("Emergency Button:");
  Serial.println("  SMS + Calls + GPS");
  Serial.println("========================================");
}

void loop()
{
  while (GPS.available())
  {
    gps.encode(GPS.read());
  }

  float distance = getDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance < DETECTION_DISTANCE)
  {
    Serial.println("OBSTACLE DETECTED!");

    digitalWrite(VIBRATOR_PIN, HIGH);

    playVoice();

    Serial.println("Servo: 0 -> 120");

    myServo.write(SERVO_ALERT);

    delay(1000);

    Serial.println("Servo: 120 -> 0");

    myServo.write(SERVO_HOME);

    delay(1000);
  }
  else
  {
    digitalWrite(VIBRATOR_PIN, LOW);

    myServo.write(SERVO_HOME);
  }

  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState == LOW &&
      lastButtonState == HIGH &&
      millis() - lastButtonTime > debounceTime)
  {
    lastButtonTime = millis();

    Serial.println();
    Serial.println("**************");
    Serial.println("     EMERGENCY BUTTON PRESSED");
    Serial.println("**************");

    emergencyAlert();

    while (digitalRead(BUTTON_PIN) == LOW)
    {
      while (GPS.available())
      {
        gps.encode(GPS.read());
      }

      delay(10);
    }

    Serial.println("Emergency button released.");
  }

  lastButtonState = currentButtonState;

  delay(100);
}