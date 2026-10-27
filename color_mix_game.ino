#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int RGB_ON  = LOW;
const int RGB_OFF = HIGH;

const int BTN_P1_R = 5;
const int BTN_P1_G = 6;
const int BTN_P1_B = 7;

const int BTN_P2_R = 2;
const int BTN_P2_G = 3;
const int BTN_P2_B = 4;

const int CENTER_R = 8;
const int CENTER_G = 9;
const int CENTER_B = 10;

const int P1_SCORE_PINS[3] = {11, 12, 13};
const int P2_SCORE_PINS[3] = {A0, A1, A2};

const int BUZZER_PIN = A3;
const int BUZZER_ON  = LOW;
const int BUZZER_OFF = HIGH;

const int MAX_PATTERN = 10;
int soundPattern[MAX_PATTERN];
int soundLen = 0;
int soundStep = 0;
unsigned long soundStart = 0;
bool soundPlaying = false;

const int WIN_SCORE          = 3;
const int MEMORY_MIN_LEN     = 2;
const int MEMORY_MAX_LEN     = 4;
const int SHOW_ON_MS         = 700;
const int SHOW_OFF_MS        = 300;
const unsigned long RELEASE_MS = 40;

int p1_score = 0;
int p2_score = 0;

enum RoundMode {
  MODE_MIX,
  MODE_MEMORY
};
RoundMode roundMode = MODE_MIX;

int targetColor = 0;

int sequence[MEMORY_MAX_LEN];
int seqLen = 0;

struct PlayerInput {
  int pinR, pinG, pinB;
  bool waitRelease;
  int held;
  unsigned long lastActive;
  int progress;
  bool failed;
};

PlayerInput p1 = {BTN_P1_R, BTN_P1_G, BTN_P1_B, true, 0, 0, 0, false};
PlayerInput p2 = {BTN_P2_R, BTN_P2_G, BTN_P2_B, true, 0, 0, 0, false};

const int NUM_COLORS = 7;
int colorBag[NUM_COLORS];
int bagIndex = NUM_COLORS;
int lastColor = 0;

enum GameState {
  WAIT_START,
  COUNTDOWN,
  PLAYING,
  ROUND_WIN,
  GAME_OVER
};

GameState currentState = WAIT_START;

void setRGB(int pinR, int pinG, int pinB, bool r, bool g, bool b) {
  digitalWrite(pinR, r ? RGB_ON : RGB_OFF);
  digitalWrite(pinG, g ? RGB_ON : RGB_OFF);
  digitalWrite(pinB, b ? RGB_ON : RGB_OFF);
}

void setCenterColor(int code) {
  setRGB(CENTER_R, CENTER_G, CENTER_B, code & 1, (code >> 1) & 1, (code >> 2) & 1);
}

void playSound(const int *pattern, int len) {
  if (len > MAX_PATTERN) len = MAX_PATTERN;
  for (int i = 0; i < len; i++) soundPattern[i] = pattern[i];
  soundLen = len;
  soundStep = 0;
  soundStart = millis();
  soundPlaying = true;
  digitalWrite(BUZZER_PIN, BUZZER_ON);
}

void updateSound() {
  if (!soundPlaying) return;
  if (millis() - soundStart >= (unsigned long)soundPattern[soundStep]) {
    soundStep++;
    soundStart = millis();
    if (soundStep >= soundLen) {
      soundPlaying = false;
      digitalWrite(BUZZER_PIN, BUZZER_OFF);
    } else {
      digitalWrite(BUZZER_PIN, (soundStep % 2 == 0) ? BUZZER_ON : BUZZER_OFF);
    }
  }
}

void finishSound() {
  while (soundPlaying) updateSound();
}

void playSoundWait(const int *pattern, int len) {
  playSound(pattern, len);
  finishSound();
}

const int SND_START[]      = {60, 60, 60, 60, 200};
const int SND_COUNT[]      = {80};
const int SND_GO[]         = {400};
const int SND_MIX_START[]  = {120};
const int SND_MEMORY_SHOW[]= {100};
const int SND_YOUR_TURN[]  = {60, 80, 60};
const int SND_CORRECT[]    = {30};
const int SND_WRONG[]      = {350};
const int SND_HIT[]        = {70, 50, 70};
const int SND_SCORE[]      = {70, 50, 70, 50, 250};
const int SND_DRAW[]       = {500, 150, 500};

#define LEN(a) (sizeof(a) / sizeof(a[0]))

const int HB_RHYTHM[] = {
  3, 1, 4, 4, 4, 8,
  3, 1, 4, 4, 4, 8,
  3, 1, 4, 4, 4, 4, 8,
  3, 1, 4, 4, 4, 8
};
const int HB_UNIT_MS  = 110;
const int HB_ON_PCT   = 85;

void playVictorySong(bool p1Wins) {
  const int *pins = p1Wins ? P1_SCORE_PINS : P2_SCORE_PINS;

  for (unsigned int n = 0; n < LEN(HB_RHYTHM); n++) {
    unsigned long noteMs = (unsigned long)HB_RHYTHM[n] * HB_UNIT_MS;
    unsigned long onMs   = noteMs * HB_ON_PCT / 100;

    digitalWrite(BUZZER_PIN, BUZZER_ON);
    setCenterColor(nextColor());
    for (int i = 0; i < 3; i++) digitalWrite(pins[i], HIGH);
    delay(onMs);

    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    for (int i = 0; i < 3; i++) digitalWrite(pins[i], LOW);
    delay(noteMs - onMs);
  }

  setCenterColor(0);
  for (int i = 0; i < 3; i++) digitalWrite(pins[i], HIGH);
}

void setup() {
  lcd.init();
  lcd.backlight();

  pinMode(BTN_P1_R, INPUT_PULLUP);
  pinMode(BTN_P1_G, INPUT_PULLUP);
  pinMode(BTN_P1_B, INPUT_PULLUP);
  pinMode(BTN_P2_R, INPUT_PULLUP);
  pinMode(BTN_P2_G, INPUT_PULLUP);
  pinMode(BTN_P2_B, INPUT_PULLUP);

  digitalWrite(CENTER_R, RGB_OFF);
  digitalWrite(CENTER_G, RGB_OFF);
  digitalWrite(CENTER_B, RGB_OFF);

  pinMode(CENTER_R, OUTPUT);
  pinMode(CENTER_G, OUTPUT);
  pinMode(CENTER_B, OUTPUT);

  for (int i = 0; i < 3; i++) {
    pinMode(P1_SCORE_PINS[i], OUTPUT);
    pinMode(P2_SCORE_PINS[i], OUTPUT);
  }

  digitalWrite(BUZZER_PIN, BUZZER_OFF);
  pinMode(BUZZER_PIN, OUTPUT);

  resetGame();
}

void loop() {
  updateSound();

  switch (currentState) {
    case WAIT_START:
      showWaitStartScreen();
      checkStartCondition();
      break;

    case COUNTDOWN:
      runCountdown();
      startNewRound();
      currentState = PLAYING;
      break;

    case PLAYING:
      if (roundMode == MODE_MIX) {
        playMixRound();
      } else {
        playMemoryRound();
      }
      break;

    case ROUND_WIN:
      finishSound();
      delay(2000);
      if (p1_score >= WIN_SCORE || p2_score >= WIN_SCORE) {
        showGameOverScreen();
        playVictorySong(p1_score >= WIN_SCORE);
        currentState = GAME_OVER;
      } else {
        startNewRound();
        currentState = PLAYING;
      }
      break;

    case GAME_OVER:
      showGameOverScreen();
      checkStartCondition();
      break;
  }
}

void showWaitStartScreen() {
  lcd.setCursor(0, 0);
  lcd.print(" COLOR MIX GAME ");
  lcd.setCursor(0, 1);
  lcd.print("Press All 6 Btns");
}

void checkStartCondition() {
  if (digitalRead(BTN_P1_R) == LOW && digitalRead(BTN_P1_G) == LOW && digitalRead(BTN_P1_B) == LOW &&
      digitalRead(BTN_P2_R) == LOW && digitalRead(BTN_P2_G) == LOW && digitalRead(BTN_P2_B) == LOW) {
    randomSeed(micros());
    bagIndex = NUM_COLORS;
    resetGame();
    playSoundWait(SND_START, LEN(SND_START));
    currentState = COUNTDOWN;
  }
}

void runCountdown() {
  lcd.clear();
  for (int i = 3; i > 0; i--) {
    lcd.setCursor(0, 0);
    lcd.print("   READY? GO!   ");
    lcd.setCursor(7, 1);
    lcd.print(i);
    playSoundWait(SND_COUNT, LEN(SND_COUNT));
    delay(720);
  }
  playSoundWait(SND_GO, LEN(SND_GO));
  lcd.clear();
}

void startNewRound() {
  resetPlayerInput(p1);
  resetPlayerInput(p2);

  roundMode = (random(0, 2) == 0) ? MODE_MIX : MODE_MEMORY;

  if (roundMode == MODE_MIX) {
    targetColor = nextColor();
    updateScoreDisplay();
    lcd.setCursor(0, 1);
    lcd.print("   MIX COLOR!   ");
    setCenterColor(targetColor);
    playSound(SND_MIX_START, LEN(SND_MIX_START));
  } else {
    seqLen = random(MEMORY_MIN_LEN, MEMORY_MAX_LEN + 1);
    for (int i = 0; i < seqLen; i++) {
      sequence[i] = nextColor();
    }
    showSequence();
    updateScoreDisplay();
    showMemoryProgress();
  }
}

void resetPlayerInput(PlayerInput &p) {
  p.waitRelease = true;
  p.held = 0;
  p.lastActive = 0;
  p.progress = 0;
  p.failed = false;
}

void showSequence() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  MEMORY TIME!  ");
  lcd.setCursor(0, 1);
  lcd.print(" Watch " + String(seqLen) + " colors ");

  setCenterColor(0);
  delay(1000);

  for (int i = 0; i < seqLen; i++) {
    setCenterColor(sequence[i]);
    playSoundWait(SND_MEMORY_SHOW, LEN(SND_MEMORY_SHOW));
    delay(SHOW_ON_MS - SND_MEMORY_SHOW[0]);
    setCenterColor(0);
    delay(SHOW_OFF_MS);
  }
  playSound(SND_YOUR_TURN, LEN(SND_YOUR_TURN));
}

int readColor(const PlayerInput &p) {
  int c = 0;
  if (digitalRead(p.pinR) == LOW) c |= 1;
  if (digitalRead(p.pinG) == LOW) c |= 2;
  if (digitalRead(p.pinB) == LOW) c |= 4;
  return c;
}

int readCommittedColor(PlayerInput &p) {
  int c = readColor(p);

  if (p.waitRelease) {
    if (c == 0) p.waitRelease = false;
    return 0;
  }

  if (c != 0) {
    p.held |= c;
    p.lastActive = millis();
    return 0;
  }

  if (p.held != 0 && millis() - p.lastActive >= RELEASE_MS) {
    int committed = p.held;
    p.held = 0;
    return committed;
  }
  return 0;
}

void playMixRound() {
  int c1 = readColor(p1);
  int c2 = readColor(p2);

  if (p1.waitRelease && c1 == 0) p1.waitRelease = false;
  if (p2.waitRelease && c2 == 0) p2.waitRelease = false;

  bool p1_match = !p1.waitRelease && c1 == targetColor;
  bool p2_match = !p2.waitRelease && c2 == targetColor;

  if (p1_match) {
    awardRound(1);
  } else if (p2_match) {
    awardRound(2);
  }
}

void playMemoryRound() {
  if (handleMemoryPlayer(p1, 1)) return;
  if (handleMemoryPlayer(p2, 2)) return;

  if (p1.failed && p2.failed) {
    lcd.setCursor(0, 1);
    lcd.print(" BOTH WRONG! :( ");
    setCenterColor(0);
    finishSound();
    playSoundWait(SND_DRAW, LEN(SND_DRAW));
    currentState = ROUND_WIN;
  }
}

bool handleMemoryPlayer(PlayerInput &p, int playerNum) {
  if (p.failed) return false;

  int c = readCommittedColor(p);
  if (c == 0) return false;

  if (c == sequence[p.progress]) {
    p.progress++;
    if (p.progress >= seqLen) {
      awardRound(playerNum);
      return true;
    }
    playSound(SND_CORRECT, LEN(SND_CORRECT));
  } else {
    p.failed = true;
    playSound(SND_WRONG, LEN(SND_WRONG));
  }
  showMemoryProgress();
  return false;
}

void showMemoryProgress() {
  String s1 = p1.failed ? "XX " : String(p1.progress) + "/" + String(seqLen);
  String s2 = p2.failed ? "XX " : String(p2.progress) + "/" + String(seqLen);
  String line = "P1 " + s1 + "  P2 " + s2;
  while (line.length() < 16) line += " ";
  lcd.setCursor(0, 1);
  lcd.print(line);
}

void awardRound(int winner) {
  bool isHit;
  if (winner == 1) {
    isHit = (p2_score > 0);
    if (isHit) {
      p2_score--;
      showRoundWinner("P1 HIT!  P2 -1  ");
    } else {
      p1_score++;
      showRoundWinner("P1 SCORE!  +1   ");
    }
  } else {
    isHit = (p1_score > 0);
    if (isHit) {
      p1_score--;
      showRoundWinner("P2 HIT!  P1 -1  ");
    } else {
      p2_score++;
      showRoundWinner("P2 SCORE!  +1   ");
    }
  }

  if (isHit) {
    playSoundWait(SND_HIT, LEN(SND_HIT));
  } else {
    playSoundWait(SND_SCORE, LEN(SND_SCORE));
  }
}

void showRoundWinner(String winnerText) {
  updateScoreDisplay();

  lcd.setCursor(0, 1);
  lcd.print(winnerText);

  setCenterColor(0);

  currentState = ROUND_WIN;
}

void updateScoreDisplay() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(P1_SCORE_PINS[i], i < p1_score ? HIGH : LOW);
    digitalWrite(P2_SCORE_PINS[i], i < p2_score ? HIGH : LOW);
  }

  lcd.setCursor(0, 0);
  lcd.print("P1: " + String(p1_score) + "  vs  P2: " + String(p2_score) + " ");
}

void showGameOverScreen() {
  lcd.setCursor(0, 0);
  if (p1_score >= WIN_SCORE) {
    lcd.print(" PLAYER 1 WIN!! ");
  } else {
    lcd.print(" PLAYER 2 WIN!! ");
  }
  lcd.setCursor(0, 1);
  lcd.print("Press 6B: Reset ");

  for (int i = 0; i < 3; i++) {
    digitalWrite(p1_score >= WIN_SCORE ? P1_SCORE_PINS[i] : P2_SCORE_PINS[i], LOW);
  }
  delay(200);
  for (int i = 0; i < 3; i++) {
    digitalWrite(p1_score >= WIN_SCORE ? P1_SCORE_PINS[i] : P2_SCORE_PINS[i], HIGH);
  }
  delay(200);
}

void resetGame() {
  p1_score = 0;
  p2_score = 0;
  setCenterColor(0);
  for (int i = 0; i < 3; i++) {
    digitalWrite(P1_SCORE_PINS[i], LOW);
    digitalWrite(P2_SCORE_PINS[i], LOW);
  }
}

void shuffleBag() {
  for (int i = 0; i < NUM_COLORS; i++) {
    colorBag[i] = i + 1;
  }
  for (int i = NUM_COLORS - 1; i > 0; i--) {
    int j = random(0, i + 1);
    int temp = colorBag[i];
    colorBag[i] = colorBag[j];
    colorBag[j] = temp;
  }
  if (colorBag[0] == lastColor) {
    int j = random(1, NUM_COLORS);
    int temp = colorBag[0];
    colorBag[0] = colorBag[j];
    colorBag[j] = temp;
  }
  bagIndex = 0;
}

int nextColor() {
  if (bagIndex >= NUM_COLORS) {
    shuffleBag();
  }
  lastColor = colorBag[bagIndex];
  bagIndex++;
  return lastColor;
}
