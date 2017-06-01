#include "SSD1306_minimal.h"
#include "MorseCodeDefs.h"
SSD1306_Mini oled;

#define MORSE_BUTTON 1
#define LETTER_BUTTON 2
#define SPACE_BUTTON 3

bool morseButton = false;
bool letterButton = false;
bool spaceButton = false;
bool onScreenMessage = false;
bool needNewQuiz = true;
char quizCharacter = 'a';
long displayStartTime = 0;
bool incorrectAnswer = false;
int lastIndex = 0;

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

  GIMSK |= _BV(PCIE0);   // Enable Pin Change Interrupts
  PCMSK0 |= _BV(PCINT1); //Enable Morse Button
  PCMSK0 |= _BV(PCINT2); //Enable Letter Button
  PCMSK0 |= _BV(PCINT3); //Enable Space Button
  sei(); //Enable interrupts

  //0 is unconnected - use it to seed the somewhat random sequences
  randomSeed(analogRead(0));
}

ISR(PCINT0_vect) {
  //If the morse button is pressed
  if (!digitalRead(MORSE_BUTTON) && !lastClosed && !onScreenMessage) {
    dotStarted = true;
    timeWhenClosed = millis();
    lastClosed = true;
  }

  //If the morse button is pressed but there's a graphic on the screen
  else if (!digitalRead(MORSE_BUTTON) && onScreenMessage) {
    morseButton = true;
  }
  
  //If the button is released
  if (digitalRead(MORSE_BUTTON) && lastClosed) {
    dotFinished = true;
    timeWhenOpened = millis();
    lastClosed = false;
  }
  
  if (!digitalRead(LETTER_BUTTON)) {
    letterButton = true;
  }

  if (!digitalRead(SPACE_BUTTON)) {
    spaceButton = true;
  }
}

void loop() {

  //Print the last letter at the bottom of the display if the user is wrong
  if (needNewQuiz) {
    if (incorrectAnswer) {
      oled.cursorTo(0,7);
      oled.printChar(quizCharacter);
      oled.printString(" = ");

      //Get the code for the last character read from the table of values
      char codeBuffer[5];
      strcpy_P(codeBuffer, (char*)pgm_read_word(&(morse_table[lastIndex])));

      oled.printString(codeBuffer);
      incorrectAnswer = false;
    }
    
    lastIndex = random(0,26); //gives between 0 and 25 inclusive
    quizCharacter = pgm_read_byte(alphabet + lastIndex); //rndm letter

    oled.cursorTo(0,0);
    oled.printString("Enter the");
    oled.cursorTo(0,1);
    oled.printString("following char: ");
    oled.printChar(quizCharacter);
    oled.cursorTo(0,4);
    needNewQuiz = false;
  }

  //When there's a graphic on-screen but it times out or the user presses a button, everything should reset
  if (onScreenMessage && (morseButton || letterButton || spaceButton || (millis() - displayStartTime > 1500))) {
    morseButton = false;
    letterButton = false;
    spaceButton = false;
    dotFinished = false;

    //Reset Character Stack
    memset(&charStack[0], 0, sizeof(charStack));
    indexer = 0;

    oled.clear();
    oled.cursorTo(0,0);
    onScreenMessage = false;

    //Triggers the above if statement, saying a new quiz is needed
    needNewQuiz = true;
  }

  else {

    //If the user has depressed and let go of the morse button
    if (dotFinished) {

      //How long it was held down for. Better to do this here rather than the ISR
      long dotTime = timeWhenOpened - timeWhenClosed;

      //Held for less than 300ms. This means a dot.
      if (dotTime < 300) {
        charStack[indexer] = '.';
        oled.printChar('.');
        indexer++;
      }
  		
  	  //More than 300ms means a dash
      else {
        charStack[indexer] = '-';
        oled.printChar('-');
        indexer++;
      }

      dotFinished = false;
      morseButton = false;
    }

    //The letter button signals that the user is ready to verify what they entered
    if (letterButton) {
      checkString(charStack);
      onScreenMessage = true;
      letterButton = false;
    }

    if (spaceButton) {
      //No use for this button
      spaceButton = false;
    }
  }
}

//Can't really pass char arrays. Instead, the pointer gets passed, thus char compare[] turns into char * compare
void checkString(char * compare) {
  //Unwrap the pointer to the array into a "real" array
  char buf[5];
  for (int i = 0; i < 5; i++) {
    buf[i] = compare[i];
  }

  int correctIndex = -1;

  //Go through all the letters of the alphabet and see if there's a match
  for (int iterator = 0; iterator < 26; iterator++) {
    //Get the code for this letter from the table of codes
    char buf2[5];
    strcpy_P(buf2, (char*)pgm_read_word(&(morse_table[iterator])));

    //Check to see if the current code stack equals this code
    if (strcmp(buf, buf2) == 0) {

      //If it does, then save the index of this correct letter
      correctIndex = iterator;
    }
  }
  
  //this variable retaining its initial value means the code sequence was invalid
  if (correctIndex == -1) {
    oled.clear();
    oled.drawImage(incorrect,0,0,128,8);
    displayStartTime = millis();
    incorrectAnswer = true;
  }

  else {
    //Get the entered character using the index found and see if it matches the quiz character
    char characterBuffer = pgm_read_byte(alphabet + correctIndex);

    //If they are equal, the user was successful!
    if (characterBuffer == quizCharacter) {
      oled.clear();
      oled.drawImage(correct,0,0,128,8);
      displayStartTime = millis();
    }

    //If they're not equal, the user entered a valid but incorrect character
    else {
      oled.clear();
      oled.drawImage(incorrect,0,0,128,8);
      displayStartTime = millis();
      incorrectAnswer = true;
    }
  }
}

