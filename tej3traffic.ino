#include <Servo.h>

#define SERVO_PIN 12
#define BUTTON_PIN 13
#define LED_PIN 11
#define LDR_PIN A0

struct upd {
  bool on;
  int pin;
  int time;
};

#define SCHEDULE_END 12000
struct upd updates[] {
  {false, 5, 0},
  {true,  7, 0},
  {true,  8, 0}, // people
  {false, 9, 0}, // people

  {false, 7, 2000},
  {true,  6, 2000},

  {false, 6, 4000},
  {true,  5, 4000},
  {false, 8, 4000}, // people
  {true,  9, 4000}, // people

  {false, 2, 6000},
  {true,  4, 6000},

  {false, 4, 8000},
  {true,  3, 8000},

  {false, 3, 10000},
  {true,  2, 10000},
  {true,  8, 12000}, // people
  {false, 9, 12000}, // people
};

#define interval 100

// get new state for button press?
int getButtonVal(int time) {
  if (time <= 4000) return 3;
  if (time <= 5500) return 2;
  return 1;
}

Servo servo;
bool button;
int curr; // current state?
int prev, now, accel;
int servoTime;

void setup()
{
  Serial.begin(9600);
  // === Streetlight
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT_PULLUP);
  // === Buttons
  pinMode(BUTTON_PIN, INPUT);
  // === Traffic Lights
  for (int i = 0; i < sizeof(updates)/sizeof(upd); i++)
	pinMode(updates[i].pin, OUTPUT);
  servo.attach(SERVO_PIN);
  
  curr = -1; // idk
}

void loop()
{
  // === Buttons et al
  now = (millis() + accel) % SCHEDULE_END;
  if (digitalRead(BUTTON_PIN)) {
    Serial.println("button pressed");
    curr = getButtonVal(now % 7000);
  }
  
  static unsigned long wait_until = millis();
  if (millis() + accel > wait_until) {
    wait_until = millis() + interval;
    if (curr < 3 && curr > 0) accel += interval*2 /3;
    else if (curr == 0) accel -= interval/3;
  }
  if (curr > 0) curr = getButtonVal(now % 7000);
  if (curr == 0 && getButtonVal(now % 7000) == 3) curr = -1;

  // === Traffic Lights
  for (int i = 0; i < sizeof(updates)/sizeof(upd); i++) {
    if (prev > now) { // handle modulo
      prev = -1;
    }
    if (prev <= updates[i].time && updates[i].time <= now ) {
      // Serial.print("pin ");
      // Serial.print(updates[i].pin);
      // Serial.print(" on ");
      // Serial.print(updates[i].on);
      // Serial.println();
      digitalWrite(updates[i].pin, updates[i].on ? HIGH : LOW);
    }
  }
  
  int adj = (servoTime % 360) >= 180 ? -1 : 1;
  servo.write(((servoTime % 360)/180)*180 + ((servoTime++) % 180) * adj);
  Serial.println((servoTime % 360));
  Serial.println(curr);
  
  delay(1);
  // to make sure prev < now (not prev == now!)
  // NOTE: above does not handle SCHEDULE_END -> 0
  prev = now;
  
  // === Streetlight
  int val = analogRead(LDR_PIN);
  if (val > 120) digitalWrite(LED_PIN, HIGH);
  else digitalWrite(LED_PIN, LOW);
}
