# MorseCard: The Tiny Morse Code Interpreter and Telegraph Basestation
## About this repository
In this repository you can find a variety of different sketches that will run on my MorseCard. If you're new to Morse code you can try out ```MultiStation-Easy.ino```, a sketch which allows you to get acquainted with the Morse code signals. ```MultiStation-Easy.ino``` also allows you to connect with other MorseCards via a speaker cable to send messages like a telegraph. ```Quiz.ino``` will give you a letter to signal out and check if you are correct.
<br><br>
For more advanced users, ```Advanced.ino``` exists. It will try its best to convert Morse Code into text, automatically recognizing spaces between letters and words. This mode also supports inter-card communication via a speaker cable.
<br><br>

You can check out the build instructions on <a href="">Hackster</a>. Fork this repository and create some sketches of your own!

## MultiStation-Easy.ino
The MorseCard has three buttons on it. These are labelled "Morse," "Lttr" (or letter), and "Space." The Morse button allows you to tap out the dots and dashes that compose characters. After you enter the code for a letter, tap the letter button to convert this letter into its textual representation. Then, tap the space button to add a space. Connect to another MorseCard via a speaker cable and the same message should appear on the screen of the connected device. This mode allows you to converse without having to worry about complicated timing.

## Quiz.ino
This sketch is a small game that helps you learn Morse code. It will display a letter on-screen, and you have to enter the correct code for that letter with the Morse button. When you're finished, press the letter button to see if you're correct! If you don't happen to get it right, the MorseCard will tell you the correct symbol for that letter.

