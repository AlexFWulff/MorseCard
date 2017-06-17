#include "SSD1306_minimal.h"
#include "MorseCodeDefs.h"
SSD1306_Mini oled;

#define MORSE_BUTTON 1
#define LETTER_BUTTON 2
#define SPACE_BUTTON 3
#define LED_ONE 8
#define LED_TWO 7

#define SPEED_FACTOR 1

static int morseButton;
static int letterButton;

static long timeWhenClosed = 0;
static long timeWhenOpened = 0;
static bool dotStarted = false;
static bool dotFinished = false;
static bool lastClosed = false;

char charStack[5];
int indexer = 0;

void setup() {
  oled.init(0x3c);
  delay(100);
  oled.startScreen();
  oled.clear();
  oled.cursorTo(0,0);

  pinMode(MORSE_BUTTON, INPUT_PULLUP);
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  GIMSK |= _BV(PCIE0);   // Enable Pin Change Interrupts
  PCMSK0 |= _BV(PCINT1);
  PCMSK0 |= _BV(PCINT2);
  sei(); //Enable interrupts
}

ISR(PCINT0_vect) {
  //If the button is pressed
  if (!digitalRead(MORSE_BUTTON) && !lastClosed) {
    dotStarted = true;
    timeWhenClosed = millis();
    lastClosed = true;
    digitalWrite(LED_ONE, HIGH);
  }

  //If the button is released
  if (digitalRead(MORSE_BUTTON) && lastClosed) {
    dotFinished = true;
    timeWhenOpened = millis();
    lastClosed = false;
    digitalWrite(LED_ONE, LOW);
  }
}

void loop() {
  if (dotFinished) {
    long dotTime = timeWhenOpened - timeWhenClosed;
    if (dotTime < 300 * SPEED_FACTOR) {
      charStack[indexer] = '.';
      indexer++;
    }

    else {
      charStack[indexer] = '-';
      indexer++;
    }

    dotFinished = false;
  }

  if (dotStarted) {
    int spaceTime = timeWhenClosed - timeWhenOpened;

    if ((spaceTime > 100 * SPEED_FACTOR * 3) && (spaceTime < 350 * SPEED_FACTOR * 3)) {
      digitalWrite(LED_TWO, HIGH);
      checkString(charStack);
      indexer = 0;
      memset(&charStack[0], 0, sizeof(charStack));
      digitalWrite(LED_TWO, LOW);
    }
    
    dotStarted = false;
  }

  if ((millis() - timeWhenOpened > 350 * SPEED_FACTOR * 3) && (charStack[0] == '.' || charStack[0] == '-')) {
    digitalWrite(LED_TWO, HIGH);
    checkString(charStack);
    oled.printChar(' ');
    indexer = 0;
    memset(&charStack[0], 0, sizeof(charStack));
    digitalWrite(LED_TWO, LOW);
  }
}

//Can't really pass char arrays. Instead, the pointer gets passed, thus char compare[] turns into char * compare
void checkString(char * compare) {
  //Unwrap the pointer to the array into a "real" array
  char buf[7];
  for (int i = 0; i < 6; i++) {
    buf[i] = compare[i];
  }

  int correctIndex = -1;
  
  for (int iterator = 0; iterator < 36; iterator++) {
    char buf2[7];
    strcpy_P(buf2, (char*)pgm_read_word(&(morse_table[iterator])));
    
    if (strcmp(buf, buf2) == 0) {
      correctIndex = iterator;
    }
  }
  
  if (correctIndex == -1) {
    
  }

  else {
    char characterBuffer = pgm_read_byte(alphabet + correctIndex);
    oled.printChar(characterBuffer);
  }
}

