

#include <Servo.h>

// --- pins (change later if needed) ---
#define LDR_TL   A0
#define LDR_TR   A1
#define LDR_BL   A2
#define LDR_BR   A3
#define SERVO_PAN   9
#define SERVO_TILT 10

// angles. start somewhere in the middle so it doesn't smash the mount
int panAngle  = 90;
int tiltAngle = 90;

// mechanical stops (these are rough, set to your rig)
int PAN_MIN  = 10,  PAN_MAX  = 170;
int TILT_MIN = 20,  TILT_MAX = 160;

// “control” bits (just P-control, no full PID today)
float Kp = 0.015;   // if lazy: try 0.02 or 0.01
int   DEAD_BAND = 30;   // ignore tiny errors so it doesn’t wiggle forever
int   MAX_STEP  = 3;    // degrees per loop (cap so it moves smoothly-ish)

// smoothing (moving avg so readings aren’t jumpy)
const byte N_SAMPLES = 5;        // not too big or it feels sleepy
int bufTL[N_SAMPLES], bufTR[N_SAMPLES], bufBL[N_SAMPLES], bufBR[N_SAMPLES];
byte iBuf = 0;
bool bufFull = false;

// servos
Servo sPan, sTilt;

// ---- helpers (not fancy) ----
int clampi(int x, int lo, int hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

// NOTE: analogRead is already kinda noisy, this is fine
int readLDR(byte pin) {
  return analogRead(pin);   // 0..1023 (brighter => bigger number with this wiring)
}

// average of a tiny buffer
int avg(const int *b) {
  long sum = 0;
  byte n = bufFull ? N_SAMPLES : iBuf;
  if (n == 0) n = 1;           // don’t divide by 0 (ask me how i know…)
  for (byte i = 0; i < n; i++) sum += b[i];
  return (int)(sum / n);
}

// shove the latest readings into the rolling windows
void push(int TL, int TR, int BL, int BR) {
  bufTL[iBuf] = TL; bufTR[iBuf] = TR; bufBL[iBuf] = BL; bufBR[iBuf] = BR;
  iBuf++;
  if (iBuf >= N_SAMPLES) { iBuf = 0; bufFull = true; }
}

// seed buffers so first few loops aren’t trash
void seedBufs(int TL, int TR, int BL, int BR) {
  for (byte i = 0; i < N_SAMPLES; i++) {
    bufTL[i] = TL; bufTR[i] = TR; bufBL[i] = BL; bufBR[i] = BR;
  }
  iBuf = 0; bufFull = true;
}

// write both servos now (no easing, maybe later)
void pokeServos() {
  sPan.write(panAngle);
  sTilt.write(tiltAngle);
}

// ----------------- setup -----------------
void setup() {
  Serial.begin(115200);  // debugging, can comment out
  delay(100);

  sPan.attach(SERVO_PAN);
  sTilt.attach(SERVO_TILT);
  pokeServos();          // center them-ish
  delay(200);

  // read once to prime the buffers
  int TL = readLDR(LDR_TL);
  int TR = readLDR(LDR_TR);
  int BL = readLDR(LDR_BL);
  int BR = readLDR(LDR_BR);
  seedBufs(TL, TR, BL, BR);

  Serial.println("tracker on (student build)");  // yeah it booted
  // TODO: add a button to re-center if it gets lost
}

// ----------------- loop ------------------
void loop() {
  // 1) raw reads
  int TL = readLDR(LDR_TL);
  int TR = readLDR(LDR_TR);
  int BL = readLDR(LDR_BL);
  int BR = readLDR(LDR_BR);

  // 2) smooth them a bit
  push(TL, TR, BL, BR);
  int sTL = avg(bufTL);
  int sTR = avg(bufTR);
  int sBL = avg(bufBL);
  int sBR = avg(bufBR);

  // 3) figure out which way is brighter
  //    (with my wiring: brighter => bigger)
  int topAvg    = (sTL + sTR) / 2;
  int bottomAvg = (sBL + sBR) / 2;
  int leftAvg   = (sTL + sBL) / 2;
  int rightAvg  = (sTR + sBR) / 2;

  // error signals. if sign feels wrong for your build, flip them (see below)
  int errVert = topAvg - bottomAvg;   // + means sun “above”
  int errHorz = rightAvg - leftAvg;   // + means sun “to the right”

  // 4) kill tiny errors -> less buzzing
  if (abs(errHorz) < DEAD_BAND) errHorz = 0;
  if (abs(errVert) < DEAD_BAND) errVert = 0;

  // 5) proportional step only (no I/D today)
  int stepPan  = (int)(Kp * errHorz);
  int stepTilt = (int)(Kp * errVert);

  // cap max movement per loop so it doesn’t yeet itself
  if (stepPan  >  MAX_STEP) stepPan  =  MAX_STEP;
  if (stepPan  < -MAX_STEP) stepPan  = -MAX_STEP;
  if (stepTilt >  MAX_STEP) stepTilt =  MAX_STEP;
  if (stepTilt < -MAX_STEP) stepTilt = -MAX_STEP;

  // 6) update angles
  // if your rig moves the wrong way, literally do:
  //   stepPan = -stepPan;   or   stepTilt = -stepTilt;
  panAngle  = clampi(panAngle  + stepPan,  PAN_MIN,  PAN_MAX);
  tiltAngle = clampi(tiltAngle + stepTilt, TILT_MIN, TILT_MAX);

  // 7) actually move servos
  pokeServos();

  