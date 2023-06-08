#include <Servo.h>

#define SERVO_PIN 10
#define BUTTON_PIN 13
#define LED_PIN 11
#define LDR_PIN A0

struct callback_request {
  unsigned long wakeBy;
};

#define flag_high(field, flag) (((field & flag) == flag) ? HIGH : LOW)

#define LIGHT_R 0x1
#define LIGHT_Y 0x2
#define LIGHT_G 0x4

struct situation {
  int through;
  int dead_end;
  int pedestrian;
};

void reconcile(struct situation now) {
  static struct situation prev = {0};
#  define flag_reconcile_debug(field, flag, pin) do { Serial.print(# field); Serial.print(# flag); Serial.print(now.field); Serial.print(","); Serial.print(flag); Serial.print(","); Serial.print((now.field & flag) == flag); Serial.print(" "); Serial.println(flag_high(now. field, flag)); digitalWrite(pin, flag_high(now. field, flag)); } while (0)
#  define flag_reconcile(field, flag, pin) do { digitalWrite(pin, flag_high(now. field, flag)); } while (0)
  // === Traffic Light Pins
  // R Y G
  // 2 3 4 not dead end
  // 5 6 7 dead end
  // 8   9 (pedestrian)
  flag_reconcile(through,    LIGHT_R, 2);
  flag_reconcile(through,    LIGHT_Y, 3);
  flag_reconcile(through,    LIGHT_G, 4);
  flag_reconcile(dead_end,   LIGHT_R, 5);
  flag_reconcile(dead_end,   LIGHT_Y, 6);
  flag_reconcile(dead_end,   LIGHT_G, 7);
  flag_reconcile(pedestrian, LIGHT_R, 8);
  flag_reconcile(pedestrian, LIGHT_G, 9);
  prev = now;
}

// Handles changes to signal lights.
// Called when a) program starts OR b) callback_request.wakeBy is in the current or the past OR c) button pressed.
struct callback_request step(bool button_pressed) {
  static int state = 0;
  static bool fast = false; // schedule after button is presed
  unsigned long now = millis();
  unsigned long wakeOffset = 2000;
  if (button_pressed) fast = true;
  switch (state) {
  case 0:
    reconcile({ LIGHT_R, LIGHT_G, LIGHT_R });
    if (fast) wakeOffset = 3000;
    break;
  case 1:
    if (fast && !button_pressed)
      fast = false;
    reconcile({ LIGHT_R, LIGHT_Y, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  case 2:
    reconcile({ LIGHT_R, LIGHT_R, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  case 3:
    reconcile({ LIGHT_G, LIGHT_R, LIGHT_G });
    if (fast) wakeOffset = 3000;
    break;
  case 4:
    if (fast && !button_pressed)
      fast = false;
    reconcile({ LIGHT_Y, LIGHT_R, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  case 5:
    reconcile({ LIGHT_R, LIGHT_R, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  }
  state ++;
  struct callback_request ret = { now + wakeOffset };
  return ret;
}

Servo servo;
unsigned long callStep = 0; // call step when millis() >= callStep
int servoTime = 0;

void setup()
{
  Serial.begin(9600);
  // === Streetlight
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT_PULLUP);
  // === Buttons
  pinMode(BUTTON_PIN, INPUT);
  // === Traffic Lights
  for (int pin = 2; pin <= 9; pin ++)
    pinMode(pin, OUTPUT);
  // === Servo
  servo.attach(SERVO_PIN);
}

void loop()
{
  if (millis() >= callStep) {
    bool button_pressed = digitalRead(BUTTON_PIN);
    struct callback_request cr = step(button_pressed);
    callStep = cr.wakeBy;
    Serial.print("called ");
    Serial.println(callStep);
  }

  // === Buttons et al
  if (digitalRead(BUTTON_PIN)) {
    Serial.println("button pressed");
  }
  
  // === Servo
  int adj = (servoTime % 360) >= 180 ? -1 : 1;
  servo.write(((servoTime % 360)/180)*180 + ((servoTime++) % 180) * adj);
  
  // delay(1);
  // // NOTE: to make sure cyclePrev < cycleNow (not cyclePrev == cycleNow!)
  // //       (note that above does not handle SCHEDULE_END -> 0)
  
  // === Streetlight
  int val = analogRead(LDR_PIN);
  if (val > 120) digitalWrite(LED_PIN, HIGH);
  else digitalWrite(LED_PIN, LOW);
}

