#include <Servo.h>
// top comments

#define SERVO_PIN 10
#define SERVO_CLOSED 90
#define SERVO_OPEN 20
#define IR_PIN A2
// assume IR is blocked when IR phototransistor value is more than IR_THRESHOLD
#define IR_THRESHOLD 128
#define BUTTON_PIN 13
#define LED_PIN 11
#define LDR_PIN A0

struct callback_request {
  unsigned long wakeBy;
};

#define flag_high(field, flag) (((field & flag) == flag) ? HI
85
    if (fast) wakeOffset = 3000;GH : LOW)

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
  static int state = 0; // incremented after wakeOffset milliseconds
  static bool fast = false; // schedule after button is presed
  unsigned long now = millis();
  unsigned long wakeOffset = 2000;
  if (button_pressed) fast = true; // set to fast schedule when button is pressed
  // debug code
  // if (fast) Serial.println("fast");
  // else Serial.println("normal");
  state %= 6;
  switch (state) {
  case 0:
    reconcile({ LIGHT_R, LIGHT_G, LIGHT_R });
    // change lights so:
    //   through    light is red
    //   dead end   light is green
    //   pedestrian light is red
    if (fast) wakeOffset = 1000; // if fast schedule, switch to next state in 1000ms instead of normal 2000ms
    break;
  case 1:
    reconcile({ LIGHT_R, LIGHT_Y, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  case 2:
    reconcile({ LIGHT_R, LIGHT_R, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  case 3:
    if (fast && !button_pressed) {
      fast = false;
      wakeOffset = 3000;
    }
    reconcile({ LIGHT_G, LIGHT_R, LIGHT_G });
    break;
  case 4:
    reconcile({ LIGHT_Y, LIGHT_R, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  case 5:
    reconcile({ LIGHT_R, LIGHT_R, LIGHT_R });
    if (fast) wakeOffset = 1000;
    break;
  }
  state ++;
  struct callback_request ret = { now + wakeOffset }; // request to call this function wakeOffset ms later
  return ret;
}

Servo servo;
unsigned long callStep = 0; // call step when millis() >= callStep

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
  // === IR
  pinMode(IR_PIN, INPUT);
}

void handleIR() {
  static int state = 1;
  static unsigned long notepadTime = 0; // random millis() value (meaning dependent on state)
  // === States
  // 1 - nothing is happening
  // 2 - waiting for gate to open (open gate at notepad time)
  // 3 - waiting for gate to close
  int val = analogRead(IR_PIN);
  // debug print
  // Serial.print("IR ");
  // Serial.println(val);

  if (val > IR_THRESHOLD && state == 1) {
    state = 2;
    notepadTime = millis() + 2000; // "after 2 seconds"
  }
  if (state == 2 && notepadTime < millis()) {
    Serial.println("open");
    servo.write(SERVO_OPEN);
    state = 3;
    notepadTime = millis() + 1500; // "stays open for 1.5 seconds"
  }
  if (state == 3 && notepadTime < millis()) {
    Serial.println("close");
    servo.write(SERVO_CLOSED);
    state = 1;
    notepadTime = 0; // clear time for hygiene
  }
}

void loop()
{
  static bool prev_button = false;
  bool button_pressed = false;
  bool call = false;
  if (millis() >= callStep) {
    call = true;
  }
  button_pressed = digitalRead(BUTTON_PIN);
  if (button_pressed && prev_button != button_pressed) {
    call = true;
    prev_button = button_pressed;
  }
  if (call) {
    struct callback_request cr = step(button_pressed);
    callStep = cr.wakeBy;
    Serial.print("called ");
    Serial.println(callStep);
  }
  
  // === IR
  handleIR();
  
  delay(1);
  // NOTE: to make sure millis() diffing calculations are over 0
  //       (note that above does not handle SCHEDULE_END -> 0)
  //       not sure if this is needed but it doesn't do much harm...
  
  // === Streetlight
  int val = analogRead(LDR_PIN);
  if (val > 120) digitalWrite(LED_PIN, HIGH);
  else digitalWrite(LED_PIN, LOW);
}


