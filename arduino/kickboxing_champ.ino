/*Tess Geiger and Bryan DiLaura
  Digital Design Lab - Kickboxing Champion Arduino code
  
  This code is intended to be loaded onto an Arduino UNO, to be used with the custom hardware
  we built to detect hits. This hardware consists of velostat pressure sensors, going into
  comparators, which are then multiplexed into the interrupt pin on the Arduino. The led's are 
  enabled/disabled using signal lines, which are going to the gate of NMOS transistors, on a
  separate power system. This is also intended to work in parallel with the Android app that 
  we have written, which will communicate to the Arduino via bluetooth.
  
  In order to hook up the arduino the following must be done (all wires from hardware to Arduino
  are labeled):
  - Interrupt line to pin 2
  - Right/Left multiplexer line to pin 5 (1 for right, 0 for left)
  - Punch/Kick multiplexer line to pin 6 (1 for kick, 0 for punch)
  - Signal to LED's for Right Kick on pin 9
  -         ''          Left Kick on pin 10
  -         ''          Right Punch on pin 11
  -         ''          Left Punch on pin 12
  - The 7.5V input power to Vin on the Arduino
  - The 5V power out to the 5V pin
  - The 3.3V power out on Arduino to power line on bluetooth
  - Ground connect for both external hardware and bluetooth
  - RX of bluetooth connects to TX of Arduino
  - TX of bluetooth connects to RX of Arduino
*/



//---------------------------------------------GLOBALS-------------------------------------------------
//flags
boolean hit = false;
boolean BTconnect = false;
boolean in_routine = false;
char difficulty = 'e';
boolean debug = false;

//score-keeping stuff
unsigned long prev_time;
unsigned long cur_time;
unsigned long times[64];

//array of strings to hold routines
char* routines[20];

//keep track of how many there are
int num_of_routines;

//the current routine number
int routine_num;

//the current place within the routine
int current_targ = 0;

//definitions of pins for i/o
int ledpin = 13;
const int punchL = 12, punchR = 11, kickL = 10, kickR = 9;
const int R_L = 5, P_K = 6;



//-----------------------------------------------HELPER FUNCTIONS-----------------------------------------------

//turn target led on, and adjust the multiplexer to sense interrupts coming from that sensor
void LED_ON(int target) {

  //set multiplexer
  switch (target) {
    case punchL:
      digitalWrite(R_L, LOW);
      digitalWrite(P_K, LOW);
      break;

    case punchR:
      digitalWrite(R_L, HIGH);
      digitalWrite(P_K, LOW);
      break;

    case kickL:
      digitalWrite(R_L, LOW);
      digitalWrite(P_K, HIGH);
      break;

    case kickR:
      digitalWrite(R_L, HIGH);
      digitalWrite(P_K, HIGH);
      break;
  }
  //turn on LED signal
  digitalWrite(target, HIGH);

}

//Turn off all of the LED's (does nothing with the multiplexer)
void LED_CLEAR(void) {
  digitalWrite(punchL, LOW);
  digitalWrite(punchR, LOW);
  digitalWrite(kickL, LOW);
  digitalWrite(kickR, LOW);
}

//Turn on all of the LED's (does nothing with multiplexer
void LED_ALL(void) {
  digitalWrite(punchL, HIGH);
  digitalWrite(punchR, HIGH);
  digitalWrite(kickL, HIGH);
  digitalWrite(kickR, HIGH);
}

//Blinks all of the LED's num number of times, with del delay (in ms) in between each flash
void LED_BLINK(int del, int num) {
  for (int i = 0; i < num; i++) {
    LED_ALL();
    delay(del);
    LED_CLEAR();
    delay(del);
  }
}

//Determines the target using the global routine number, and the current target number.
//In order to determine the next target in sequence, current target must be incremented.
//This function returns -1 when it reaches the null terminator of the routine string, otherwise
//it will return the target (as defined in globals).
int get_target(void) {
  char test = routines[routine_num][current_targ];
  if(debug) Serial.println(test);
  if (test == 0) {
    return -1;
  }
  if (test == 'a') {
    return punchL;
  }
  if (test == 'b') {
    return punchR;
  }
  if (test == 'A') {
    return kickL;
  }
  if (test == 'B') {
    return kickR;
  }
}

//Clears the times array, and resets the current time and previous time.
void CLEAR_TIMES(void) {
  for (int i; i < 64; i++) times[i] = 0;
  cur_time = 0;
  prev_time = 0;
}

//calculates "score" based on reaction times. This is an average for each target, as
//well as an overall average. (Weights could be added to specific targets, or a letter
//grade could be implemented here if desired)
void CALC_SCORE(void) {
  unsigned long rp = 0, lp = 0, rk = 0, lk = 0, total = 0;
  int rp_n = 0, lp_n = 0, rk_n = 0, lk_n = 0, total_n = 0;

  //loop through specific routine, checking for what the target was when that time
  //was recorded, and sum.
  for (int i = 0; i < strlen(routines[routine_num]); i++) {
    switch (routines[routine_num][i]) {
      case 'a':
        lp += times[i];
        lp_n++;
        total += times[i];
        total_n++;
        break;

      case 'b':
        rp += times[i];
        rp_n++;
        total += times[i];
        total_n++;
        break;

      case 'A':
        lk += times[i];
        lk_n++;
        total += times[i];
        total_n++;
        break;

      case 'B':
        rk += times[i];
        rk_n++;
        total += times[i];
        total_n++;
        break;
    }

  }

  //calculate average
  lp = lp / lp_n;
  rp = rp / rp_n;
  lk = lk / lk_n;
  rk = rk / rk_n;
  total = total / total_n;
  
  if(debug) Serial.write("Sending scores");
  if(debug) Serial.println();
  Serial.write("Z"); //tell android it's done
  
  unsigned long reaction;
  char score;
  for(int i=0;i<5;i++){
    delay(1000);
    if(i==0) reaction = lp;
    if(i==1) reaction = rp;
    if(i==2) reaction = lk;
    if(i==3) reaction = rk;
    if(i==4) reaction = total;
    //Serial.println(reaction);
    //grading is harder if punching since kicking takes more time
    if(i==0 || i==1) {
      if(reaction < 500) score='A';
      else if(reaction < 700) score='B';
      else if(reaction < 1250) score='C';
      else if(reaction == 4294967295) score='-'; //not included in routine
      else score='F';
    }
    //overall grade is a little less harsh
    else if(i==4) {
      if(reaction < 550) score='A';
      else if(reaction < 800) score='B';
      else if(reaction < 1500) score='C';
      else if(reaction == 4294967295) score='-';
      else score='F';
    }
    else{
      if(reaction < 700) score='A';
      else if(reaction < 900) score='B';
      else if(reaction < 2000) score='C';
      else if(reaction == 4294967295) score='-'; //not included in routine
      else score='F';
    }
    
    Serial.write(score);
  }
  //Serial.println();
  //Serial.println();
    

  //send scores to android
  /*Serial.write("sending scores");
  while (!Serial.available());
  Serial.println();
  Serial.print("left punch time = ");
  Serial.println((unsigned int)lp, DEC);
  Serial.print("right punch time = ");
  Serial.println((unsigned int)rp, DEC);
  Serial.print("left kick time = ");
  Serial.println((unsigned int)lk, DEC);
  Serial.print("right kick time = ");
  Serial.println((unsigned int)rk, DEC);
  Serial.print("total time = ");
  Serial.println((unsigned int)total, DEC);//<-------------------------may have to put delays in here??
  Serial.println();
  Serial.println();*/

}

//return a random integer, determined by the difficulty level. This is meant to be used to
//create a delay in between a hit, and the next target lighting up.
int CALC_DELAY(void) {
  int rand;
  //difficulty changes the limits on the random values
  switch (difficulty) {
    case 'e':
      rand = random(2000, 7000);
      break;

    case 'm':
      rand = random(1000, 5000);
      break;

    case 'h':
      rand = random(150, 1500);
      break;
  }

  return rand;
}




//-------------------------------------------------SETUP-------------------------------------------------------
void setup() {
  //set pins as outputs
  pinMode(ledpin, OUTPUT);
  pinMode(punchL, OUTPUT);
  pinMode(punchR, OUTPUT);
  pinMode(kickL, OUTPUT);
  pinMode(kickR, OUTPUT);
  pinMode(R_L, OUTPUT);
  pinMode(P_K, OUTPUT);

  //default outputs to off
  LED_CLEAR();
  //Init scores array to 0
  CLEAR_TIMES();

  //setup serial for bluetooth
  Serial.begin(9600);

  //attach interrupt for the hits
  attachInterrupt(0, is_hit, RISING);


  //Routines can be hard-coded in here. The scheme is
  //A - left kick
  //B - right kick
  //a - left punch
  //b - right punch
  //The number of routines must also be updated in order to reflect any added routines.
  routines[0] = "bbBbBBbbbBbBbB"; //know your rights
  routines[1] = "AaaaAaAaaAAaAa"; //leftovers
  routines[2] = "ababaaababbab";//traitor
  routines[3] = "ABABAABABB"; //get your kicks
  routines[4] = "ababABaBaabAbb"; //around the world
  routines[5] = "ab"; //tess is testing and doesn't want to do a lot of work

  num_of_routines = 5;

}


//-------------------------------------------------------------LOOP-------------------------------------------------------
void loop() {

  //make sure we are connected to the phone before we do anything
  //could implement "are you still there" functionality here
  if (!BTconnect) {
    while (Serial.available() == 0);
    // while(Serial.read() != "Hello there!");
    BTconnect = true;
  }

  //we are in a routine
  if (in_routine) {

    //is there a stop command from android?
    if (Serial.available() != 0) {
      //stop the routine
      in_routine = false;
      if(debug) Serial.write("Stopped");
      LED_CLEAR();
    }

    //did a hit interrupt happen?
    if (hit) {
      if (debug) {
        Serial.println("got a hit!");
        Serial.print("in routine #");
        Serial.println(routine_num, DEC);
        Serial.print("current active pad: ");
        Serial.println(get_target(), DEC);
        Serial.print("current_target=");
        Serial.println(current_targ, DEC);
      }

      //record current hit time
      cur_time = millis();
      //turn off leds
      LED_CLEAR();
      //calculate reaction time, and place it into array
      times[current_targ] = cur_time - prev_time;

      if (debug) {
        Serial.print("reaction time");
        Serial.println(times[current_targ]);
      }
      //prev_time = cur_time;

      //figure out what is next in routine (or if we're at the end)
      current_targ++;
      int next = get_target();

      //end of routine
      if (next == -1) {
        in_routine = false;
        //calculate and send score (happens in this function)
        CALC_SCORE();
        //blink the led's to show that the routine is over
        LED_BLINK(1000, 5);
      }

      //not end of routine
      else {
        //have a random delay based on difficulty
        delay(CALC_DELAY());
        //turn on the leds for the next target
        LED_ON(next);
        //record the time when the leds turn on
        prev_time = millis();
      }

      //make sure to set hit to false!
      hit = false;
    }

  }





  //we are waiting for input from bluetooth to choose routine
  else {
    //reset some things from the last routine
    current_targ = 0;
    CLEAR_TIMES();
    detachInterrupt(0);
    if (debug) Serial.println("Not in routine, S to start, D to change diff");

    //setting up the routine for this session. This can include setting the difficulty,
    //or starting a specific routine
    while (in_routine == false) {
      if (Serial.available()) {
        //get bluetooth information and make a decision based on this
        char mes = Serial.read();

        //start a routine
        if (mes == 'S') {
          //do some sort of handshake to get the routine?
          if(debug) Serial.write("ready S");

          //get which routine we are doing
          while (!Serial.available());
          int rout = Serial.read() - '0';

          //make sure that it is a valid routine number
          if (rout > num_of_routines) Serial.write('F');
          else {
            Serial.write('S');
            routine_num = rout;
            //give user feedback that we are starting, and turn on LED for target
            LED_BLINK(1000, 3);
            delay(500);
            LED_ON(get_target());

            //begin the game
            in_routine = true;
            hit = false;
            attachInterrupt(0, is_hit, RISING);
            prev_time = millis();
          }
        }

        //change the difficulty level
        if (mes == 'D') {
          //change difficulty
          Serial.write("r");//tell android it's ready
          while (!Serial.available());
          char diff = Serial.read();
          if (diff == 'e' || diff == 'm' || diff == 'h') {
            Serial.write('S');
            difficulty = diff;
          }
          else Serial.write('F');
        }

        if (mes == 'R') {
          //create routine?
        }
      }
    }
  }
}




//Interrupt routine
void is_hit() {
  hit = true;
}
