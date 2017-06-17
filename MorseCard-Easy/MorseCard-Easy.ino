#include "SSD1306_minimal.h"
#include "MorseCodeDefs.h"
SSD1306_Mini oled;

#define MORSE_BUTTON 1
#define LETTER_BUTTON 2
#define SPACE_BUTTON 3
#define LED_ONE 8
#define LED_TWO 7

bool morseButton = false;
bool letterButton = false;
bool spaceButton = false;
bool error = false;
bool dictionaryMode = false;
long letterDebounceTime = 0;
long spaceDebounceTime = 0;

long timeWhenClosed = 0;
long timeWhenOpened = 0;
bool dotStarted = false;
bool dotFinished = false;
bool lastClosed = false;

char charStack[5];
int indexer = 0;

void setup() {
  oled.init(0x3c);
  delay(100);
  oled.startScreen();
  oled.clear();
  oled.cursorTo(0,0);
  
  pinMode(MORSE_BUTTON, INPUT_PULLUP);
  pinMode(LETTER_BUTTON, INPUT_PULLUP);
  pinMode(SPACE_BUTTON, INPUT_PULLUP);
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  
  GIMSK |= _BV(PCIE0);   // Enable Pin Change Interrupts
  PCMSK0 |= _BV(PCINT1); //Enable Morse Button
  PCMSK0 |= _BV(PCINT2); //Enable Letter Button
  PCMSK0 |= _BV(PCINT3); //Enable Space Button
  sei(); //Enable interrupts
}

ISR(PCINT0_vect) {
    //If the Morse button is pressed
  if (!digitalRead(MORSE_BUTTON) && !lastClosed && !error && !dictionaryMode) {
    dotStarted = true;
    timeWhenClosed = millis();
    lastClosed = true;
    digitalWrite(LED_ONE, HIGH);
  }

  //If the Morse button is pressed and the error or dictionary message is displayed
  else if (!digitalRead(MORSE_BUTTON) && (error || dictionaryMode)) {
    morseButton = true;
  }

  //If the Morse button is released
  if (digitalRead(MORSE_BUTTON) && lastClosed && !error) {
    dotFinished = true;
    timeWhenOpened = millis();
    lastClosed = false;
    digitalWrite(LED_ONE, LOW);
  }
  
  if (!digitalRead(LETTER_BUTTON)) {
    letterButton = true;
  }

  if (!digitalRead(SPACE_BUTTON)) {
    spaceButton = true;
  }
}

void loop() {

  //If one of the buttons is pressed and there's an error message or the dictionary on-screen
  if ((error || dictionaryMode) && (morseButton || letterButton || spaceButton)) {
    morseButton = false;
    letterButton = false;
    spaceButton = false;
    dotFinished = false;

    //Reset Character Stack
    memset(&charStack[0], 0, sizeof(charStack));
    indexer = 0;
    
    //Clear the screen and reset the error and dictionary flags
    oled.clear();
    oled.cursorTo(0,0);
    error = false;
    dictionaryMode = false;
  }

  else {

  	//If the Morse button was pressed and released
    if (dotFinished) {
      long dotTime = timeWhenOpened - timeWhenClosed;

      //This was actually a signal from another node that the letter button was pressed
      if (dotTime == 2 || dotTime == 3 || dotTime == 4) {
      	checkString(charStack);
      	indexer = 0;
      	memset(&charStack[0], 0, sizeof(charStack));
      }

      //This was actually a signal from another node that the space key was pressed
      else if (dotTime == 5 || dotTime == 6 || dotTime == 7) {
      	oled.printChar(' ');
      	indexer = 0;
      	memset(&charStack[0], 0, sizeof(charStack));
      }

      //Held for less than 300ms. This means a dot.
      else if (dotTime < 300) {
        charStack[indexer] = '.';
        indexer++;
      }

      //Held for more than 2 seconds, meaning the user wants to access the dictionary
      else if (dotTime > 2000) {
        oled.clear();
        oled.drawImage(dictionary,0,0,128,4);
        dictionaryMode = true;
        oled.cursorTo(0,6);
        oled.printString("Press any button");
        oled.cursorTo(0,7);
        oled.printString("to dismiss.");
      }
  		
  	  //More than 300ms means a dash
      else {
        charStack[indexer] = '-';
        indexer++;
      }

      //Error: No standard morse code character has more than four dots or dashes
      if (indexer > 4) {
        throwTooManySymbolsError();
      }

      dotFinished = false;
      morseButton = false;
    }

    //If the letter button was pressed and it isn't an error
    if (letterButton && (millis() - letterDebounceTime > 500)) {
      digitalWrite(LED_TWO, HIGH);
      checkString(charStack);
      indexer = 0;
      memset(&charStack[0], 0, sizeof(charStack));
      letterDebounceTime = millis();
      letterButton = false;
      digitalWrite(LED_TWO, LOW);
    }

    //If the space button was pressed and it isn't an error
    if (spaceButton && (millis() - spaceDebounceTime > 500)) {
      digitalWrite(LED_TWO, HIGH);
      oled.printChar(' ');
      indexer = 0;
      memset(&charStack[0], 0, sizeof(charStack));
      spaceDebounceTime = millis();
      spaceButton = false;
      digitalWrite(LED_TWO, LOW);
    }
  }
}

void throwTooManySymbolsError() {
  oled.clear();
  oled.drawImage(errorImage,0,0,128,4);
  oled.cursorTo(0,4);
  oled.printString("Too many morse");
  oled.cursorTo(0,5);
  oled.printString("code symbols.");
  oled.cursorTo(0,6);
  oled.printString("Press any button");
  oled.cursorTo(0,7);
  oled.printString("to dismiss.");
  error = true;

}

void throwUnknownCharacterError() {
  oled.clear();
  oled.drawImage(errorImage,0,0,128,4);
  oled.cursorTo(0,4);
  oled.printString("No character found");
  oled.cursorTo(0,5);
  oled.printString("for this combo.");
  oled.cursorTo(0,6);
  oled.printString("Press any button");
  oled.cursorTo(0,7);
  oled.printString("to dismiss.");
  error = true;
}

//Can't really pass char arrays. Instead, the pointer gets passed, thus char compare[] turns into char * compare
void checkString(char * compare) {
  //Unwrap the pointer to the array into a "real" array
  char buf[5];
  for (int i = 0; i < 5; i++) {
    buf[i] = compare[i];
  }

  int correctIndex = -1;
  
  for (int iterator = 0; iterator < 26; iterator++) {
    
    char buf2[5];
    strcpy_P(buf2, (char*)pgm_read_word(&(morse_table[iterator])));
    
    if (strcmp(buf, buf2) == 0) {
      correctIndex = iterator;
    }
  }
  
  //Did not find the character. This is an error.
  if (correctIndex == -1) {
    throwUnknownCharacterError();
  }

  else {
    char characterBuffer = pgm_read_byte(alphabet + correctIndex);
    oled.printChar(characterBuffer);
  }
}

