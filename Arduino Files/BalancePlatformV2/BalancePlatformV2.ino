/* Balance Platform Code
 *  Connor Morrow
 *  8/26/2019
 *  
 *  This code is used to control the balance platform used for determining balance characteristics of robots.
 *  The code holds its output at a constant 2.5V (the voltage for no motor velocity), until it detects a high signal coming from the H-Bridge.
 *  When the high signal is recieved, the Arduino switches to producing a sine wave output. 
 *	
 *	Jake Chung
 *	12/20/2019
 *	Update Connor's code to use encoder to measure the angle of the platform.
 */
/*----------------------- Included Libraries  --------------------------------*/
#include <SPI.h>          //For communicating with DAC
#include <Encoder.h>	

//Dac Library information at http://arduino.alhin.de/index.php?n=8
#include <AH_MCP4921.h>   //For easy DAC functions

//Define encoder pins. Encoder pins have to be interupt pins
#define encA 19
#define encB 20

/*----------------------- Definitions  --------------------------------*/
AH_MCP4921 AnalogOutput(51, 52, 53);          //SPI communication is on Arduino MEGA Pins 51, 52, 53


/*----------------------- Pin Declarations --------------------------------*/
int InputPin = A0;                      //Pin that takes in values from a function generator or potentiometer
    
volatile uint8_t VoltageMode = 0;		//Variable that keeps track of voltage mode, 0 for hold, 1 for manual control, 2 for PRTS

uint8_t HoldPin = 2;                    //Pin connected to a button that switches voltage from 2.5V to variable   
uint8_t HoldLED = 7;                    //Pin connected to an LED that is dim when voltage supplied to the Hbridge is 2.5V and lit when the voltage is variable

uint8_t ManualControlPin = 18;           //Pin connected to a button that switches control voltage to be based on the position of a manually controlled potentiometer
uint8_t ManualControlLED = 6;           //Pin connected to an LED that is lit when the platform is in manual control

uint8_t PRTSPin = 3;                    //Pin connected to a button that switches control voltage to be generated by the PRTS
uint8_t PRTSLED = 9;                    //Pin connected to an LED that is lit when the platform is controlled by PRTS signal

/*------------------------ Control Parameters -----------------------------*/
float KP = 1.0;
int DACoffset = 4096.0/2.0;
int SetPoint = 366.0;


/*------------------------PRTS Parameters ---------------------------------*/
float PRTSAngle = 0.0;
float VelocityGain = 0.3;
int counter = 0;
boolean test = 0;

//Encoder set up
Encoder myEnc(encA, encB);

void setup() {
  //Serial.begin(9600);
  pinMode(InputPin, INPUT);
  pinMode(HoldPin, INPUT);
  pinMode(HoldLED, OUTPUT);
  pinMode(ManualControlPin, INPUT);
  pinMode(ManualControlLED, OUTPUT);
  pinMode(PRTSPin, INPUT);
  pinMode(PRTSLED, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(HoldPin), HoldVoltage, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ManualControlPin), ManualVoltage, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PRTSPin), PRTSVoltage, CHANGE);
  
  //Set PWM on pin 8 to have a frequency of 4kHz
  int prescaler = 2;
  int myEraser = 7;
  TCCR4B &= ~myEraser;         //Erases the last three bits of the register so that we can replace them with our scaler
  TCCR4B |= 2;                 //Sets timer 4 PWM frequency to 4kHz

  InterruptSetup(); 
}

void loop() {
  if(VoltageMode == 0){
    digitalWrite(HoldLED, HIGH);
    digitalWrite(ManualControlLED, LOW);
    digitalWrite(PRTSLED, LOW);
  }
  else if(VoltageMode == 1){
    digitalWrite(HoldLED, LOW);  
    digitalWrite(ManualControlLED, HIGH);
    digitalWrite(PRTSLED, LOW);
  }
  else if(VoltageMode == 2){
    digitalWrite(HoldLED, LOW);  
    digitalWrite(ManualControlLED, LOW);
    digitalWrite(PRTSLED, HIGH);
  }
}

void HoldVoltage(){
  VoltageMode = 0;
}

void ManualVoltage(){
  VoltageMode = 1;
}

void PRTSVoltage(){
  VoltageMode = 2;
}

void InterruptSetup(){
  cli();//stop interrupts
  
  //set timer0 interrupt at 50 Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 1249;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10, CS11, and CS12 bits for 256 prescaler
  TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
}

ISR(TIMER1_COMPA_vect){
  if(VoltageMode == 0){
	  AnalogOutput.setValue(4096/2);
  }
  else if(VoltageMode == 1){
		//Read the encoder
	  float pos = myEnc.read();
	  float encAngle = 10.0/226.0*pos;
	  
	  float PosValue = analogRead(InputPin);                            //Reads position value from a manually controller potentiometer
	  float PosAngle = 10.0/256.0*PosValue - 20.0;
    
	  float error = encAngle - PosAngle;                               //Value - setpoint, so that the correct motor polarity is achieved
	  float DACerror = error*4096.0/20.0;
	  float DACsignal = KP*DACerror + DACoffset;              // I think the increased polarity only happens when the arduino is plugged into the computer.
    if(DACsignal > 4095.0){
      DACsignal = 4095.0;
    }
    else if(DACsignal < 0.0){
      DACsignal = 0.0;
    }
    AnalogOutput.setValue((int) DACsignal);
    //Serial.print("Position Voltage:  "); Serial.print(PosValue); Serial.print("  Pos Angle:  "); Serial.print(PosAngle); Serial.print("   Potentiometer Reading:  "); Serial.print(PotValue); Serial.print("  Potentiometer Angle:  "); Serial.print(PotAngle); Serial.print("  Error:  "); Serial.print(error); Serial.print("    DAC Signal:  "); Serial.println(DACsignal);
    }
    else if(VoltageMode == 2){

    /* PRTS Control */
    //Example code first
    float pos = myEnc.read();
    float encAngle = 10.0/226.0*pos;
    //Test code generating a triangle wave
    
    //Calculate current time in the period of the triangle wave
    counter += 1;
    if(counter == 501){
      counter = 1;
    }

    //Calculate output 
    if(counter <= 125){
      PRTSAngle = 5.0/125.0*((float) counter);
    }
    else if(counter <= 375){
      PRTSAngle = 5.0-10.0/250.0*((float) counter - 125.0);
    }
    else if(counter <= 500){
      PRTSAngle = -5+5.0/125.0*((float) counter - 375.0);
    }
    else {
      PRTSAngle = 0.0;
    }

    float error = pos - PRTSAngle;                               //Value - setpoint, so that the correct motor polarity is achieved
    float DACerror = error*4096.0/20.0;
    float DACsignal = KP*DACerror + DACoffset;              // I think the increased polarity only happens when the arduino is plugged into the computer.
    if(DACsignal > 4095.0){
      DACsignal = 4095.0;
    }
    else if(DACsignal < 0.0){
      DACsignal = 0.0;
    }
    AnalogOutput.setValue((int) DACsignal);
  }
}
