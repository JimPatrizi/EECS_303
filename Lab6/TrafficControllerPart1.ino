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
const int buttonPin = 2;

//int SWITCH = HIGH;
int reading = LOW;
//int previous = LOW;

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

    //Init Ra, Gb = HIGH

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

//    //Ra, Gb = High
//
//      digitalWrite(Ra,HIGH);
//      delay(1000);
//      digitalWrite(Rb, LOW);
//      analogWrite(Gb, brightnessHigh);
//      delay(5000);
//      analogWrite(Gb, brightnessLow);
//      delay(1000);
//      analogWrite(Gb, brightnessOff);
//
//    //Ga, Rb = High
//
//      digitalWrite(Rb, HIGH);
//      delay(1000);
//      digitalWrite(Ra, LOW);
//      analogWrite(Ga, brightnessHigh);
//      delay(5000);
//      analogWrite(Ga, brightnessLow);
//      delay(1000);
//      analogWrite(Ga, brightnessOff);
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
