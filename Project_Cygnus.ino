#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------- OBJECTS --------
BluetoothSerial SerialBT;
WebServer server(80);

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- PINS --------
#define LED1 2
#define LED2 4
#define FAN_IN1 25
#define FAN_IN2 26
#define FAN_EN 27
#define BUZZER 23

// -------- BUZZER PWM --------
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 2000
#define BUZZER_RESOLUTION 8

// -------- STATES --------
String led1State = "OFF";
String led2State = "OFF";
String fanState  = "OFF";

String command = "";

// -------- WIFI --------
const char* ssid = "CYGNUS";
const char* password = "12345678";

// -------- BUZZER --------
void beep(int duration) {
  ledcWrite(BUZZER_CHANNEL, 128);
  delay(duration);
  ledcWrite(BUZZER_CHANNEL, 0);
}

void successBeep() {
  beep(100);
}

void offBeep() {
  beep(80); delay(80); beep(80);
}

void errorBeep() {
  beep(400);
}

// -------- OLED --------
void updateDisplay(String msg = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.println("CYGNUS SYSTEM");

  display.setCursor(0,15);
  display.print("LED1: "); display.println(led1State);

  display.setCursor(0,30);
  display.print("LED2: "); display.println(led2State);

  display.setCursor(0,45);
  display.print("FAN : "); display.println(fanState);

  if (msg != "") {
    display.setCursor(0,55);
    display.println(msg);
  }

  display.display();
}

// -------- COMMAND --------
void executeCommand(String cmd) {

  cmd.toLowerCase();
  cmd.trim();

  Serial.println("CMD: " + cmd);

  // -------- FAN --------
  if (cmd.indexOf("fan") >= 0 && cmd.indexOf("on") >= 0) {
    digitalWrite(FAN_IN1, HIGH);
    digitalWrite(FAN_IN2, LOW);
    fanState = "ON";
    successBeep();
  }

  else if (cmd.indexOf("fan") >= 0 && cmd.indexOf("of") >= 0) {
    digitalWrite(FAN_IN1, LOW);
    digitalWrite(FAN_IN2, LOW);
    fanState = "OFF";
    offBeep();
  }

  // -------- LED1 --------
  else if (cmd.indexOf("led 1") >= 0 || cmd.indexOf("led one") >= 0) {

    if (cmd.indexOf("on") >= 0) {
      digitalWrite(LED1, HIGH);
      led1State = "ON";
      successBeep();
    }

    else if (cmd.indexOf("of") >= 0) {
      digitalWrite(LED1, LOW);
      led1State = "OFF";
      offBeep();
    }
  }

  // -------- LED2 --------
  else if (cmd.indexOf("led 2") >= 0 || cmd.indexOf("led two") >= 0) {

    if (cmd.indexOf("on") >= 0) {
      digitalWrite(LED2, HIGH);
      led2State = "ON";
      successBeep();
    }

    else if (cmd.indexOf("of") >= 0) {
      digitalWrite(LED2, LOW);
      led2State = "OFF";
      offBeep();
    }
  }

  // -------- ALL OFF --------
  else if (cmd.indexOf("all") >= 0 || cmd.indexOf("shutdown") >= 0) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(FAN_IN1, LOW);
    digitalWrite(FAN_IN2, LOW);

    led1State = "OFF";
    led2State = "OFF";
    fanState = "OFF";

    offBeep();
  }

  else {
    errorBeep();
  }

  updateDisplay();
}

// -------- WEB PAGE --------
String webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<body style="text-align:center;background:black;color:white;">
<h2>CYGNUS CONTROL</h2>

<button onclick="send('led 1 on')">LED1 ON</button>
<button onclick="send('led 1 off')">LED1 OFF</button><br><br>

<button onclick="send('led 2 on')">LED2 ON</button>
<button onclick="send('led 2 off')">LED2 OFF</button><br><br>

<button onclick="send('fan on')">FAN ON</button>
<button onclick="send('fan off')">FAN OFF</button><br><br>

<button onclick="send('all off')" style="background:red;">ALL OFF</button>

<script>
function send(cmd){
  fetch('/cmd?c=' + cmd);
}
</script>

</body>
</html>
)rawliteral";

// -------- SETUP --------
void setup() {

  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_EN, OUTPUT);

  digitalWrite(FAN_EN, HIGH);

  // BUZZER
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER, BUZZER_CHANNEL);

  // OLED
  Wire.begin(18, 19);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  updateDisplay("Ready");

  // Bluetooth
  SerialBT.begin("CYGNUS");

  // WiFi AP
  WiFi.softAP(ssid, password);

  server.on("/", []() {
    server.send(200, "text/html", webpage);
  });

  server.on("/cmd", []() {
    executeCommand(server.arg("c"));
    server.send(200, "text/html", webpage);
  });

  server.begin();

  Serial.println("🔥 SYSTEM READY");
}

// -------- LOOP --------
void loop() {

  server.handleClient();

  if (SerialBT.available()) {
    command = SerialBT.readString();
    executeCommand(command);
  }
}