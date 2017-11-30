/**
 * Lab *? Part One
 * Traffic Controller
 * 
 */

//LED Port Declarations
const int Ra = 12;
const int Ga = 9;
const int Rb = 11;
const int Gb = 10;
const int buttonPin = 4;
const int crosswalkAPin = 3;
const int crosswalkBPin = 2;

//int SWITCH = HIGH;
int reading = LOW;
//int previous = LOW;
int loopMode = 0;  //0 = normal loop, 1 = crosswalk A = 1, crossswalk B = 2;

int brightnessHigh = 255;
int brightnessLow = 70;
int brightnessOff = 0;

//// the follow variables are long's because the time, measured in miliseconds,
//// will quickly become a bigger number than can be stored in an int.
//long time = 0;         // the last time the output pin was toggled
//long debounce = 200;   // the debounce time, increase if the output flickers

void setup() 
{
  // declare pin 9 to be an output for pwm:
  pinMode(Ga, OUTPUT);
  pinMode(Gb, OUTPUT);
  //Ra and Rb are seperate normal outputs for red leds
  pinMode(Ra, OUTPUT);
  pinMode(Rb, OUTPUT);
  pinMode(buttonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(crosswalkAPin), crosswalkA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(crosswalkBPin), crosswalkB_ISR, RISING);
  
  Serial.begin(9600);
}


/**
 * State Machine
 */
void loop() 
{
  //Read current button state (used as a switch)
  reading = digitalRead(buttonPin);
    Serial.println("loop start");
  Serial.println(reading);
  
//
//  // if the input just went from LOW and HIGH and we've waited long enough
//  // to ignore any noise on the circuit, toggle the current switch state
//  if (reading == HIGH && previous == LOW && millis() - time > debounce)
//  {
//    if (SWITCH == HIGH)
//      SWITCH = LOW;
//    else
//      SWITCH = HIGH;
//
//    time = millis();    
//  }
//  previous = reading;
  
  int state;
  if(reading == HIGH)
  {
    if(loopMode==0 || loopMode==3){
      normalTrafficLoop();
    }
    else if(loopMode==1){
      crosswalkALoop();
    }
    else if(loopMode==2){
      crosswalkBLoop();
    }


    }
     else
     {
        //turn all LEDs off:
        for(int thisPin = 9; thisPin <= 12; thisPin++)
        {
          digitalWrite(thisPin, LOW);
        }
       delay(1000);
       Serial.println("Off");
     }
}

void normalTrafficLoop(){

    loopMode = 0;       //reset the flag
    Serial.println("Doing normal traffic loop");
      digitalWrite(Ra, HIGH);
      digitalWrite(Rb, HIGH);
      delay(1000);
      digitalWrite(Rb, LOW);
      analogWrite(Gb, brightnessHigh);
      delay(5000);
      analogWrite(Gb, brightnessLow);
      delay(1000);
      analogWrite(Gb, brightnessOff);
      
    //Ga, Rb = HIGH
      digitalWrite(Rb, HIGH);
      delay(1000);
      digitalWrite(Ra, LOW);
      analogWrite(Ga, brightnessHigh);
      delay(5000);
      analogWrite(Ga, brightnessLow);
      delay(1000);
      analogWrite(Ga, brightnessOff);
  
}

void crosswalkALoop(){
  
      loopMode = 3;       //reset the flag to 3, prevents running two back to back crosswalk cycles
    Serial.println("Doing Crosswalk A loop");
      digitalWrite(Ra, HIGH);
      digitalWrite(Rb, HIGH);
      delay(1000);
      digitalWrite(Rb, LOW);
      analogWrite(Gb, brightnessHigh);
      delay(5000);
      analogWrite(Gb, brightnessLow);
      delay(1000);
      analogWrite(Gb, brightnessOff);
  
}

void crosswalkBLoop(){

    loopMode = 3;       //reset the flag to 3, prevents running two back to back crosswalk cycles
    Serial.println("Doing Crosswalk B loop");
      digitalWrite(Ra, HIGH);
      digitalWrite(Rb, HIGH);
      delay(1000);
      digitalWrite(Ra, LOW);
      analogWrite(Ga, brightnessHigh);
      delay(5000);
      analogWrite(Ga, brightnessLow);
      delay(1000);
      analogWrite(Ga, brightnessOff);
      
}

/**
 * Interrupt handlers for the crosswalk buttons
 */

void crosswalkA_ISR() {
  
    Serial.println("Crosswalk A triggered");
  if(loopMode!=3){      //if the flag is set to 3 a crosswalk loop was just executed, prevent it from doing two crosswalk cycles back to back
    loopMode = 1;       //set the flag to run the crosswalk A cycle at the end of the current one
  }
}

void crosswalkB_ISR() {
  
    Serial.println("Crosswalk B triggered");
 if(loopMode!=3){      //if the flag is set to 3 a crosswalk loop was just executed, prevent it from doing two crosswalk cycles back to back
  loopMode = 2;       //set the flag to run the crosswalk B cycle at the end of the current one
 }
}


