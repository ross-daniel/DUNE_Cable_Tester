//-------------------------------------------------------
//   Cable Tester Code for the DUNE Experiment
//            created by: Ross Stauder
//                   rev 2024
//-------------------------------------------------------
//
//PINOUT: (For ATMEGA)
//
//Pin 2 -- Push Button negative terminal
//+5V -- Push Button positive terminal
//------------------OUTPUT PINS---------------------------
//Pins 22-29 (PORTA) -- first 8 connextions in A_T2 in order
//Pins 30-33 (PORTC7-4) -- last 4 connections in A_T2 in order
//------------------INPUT PINS----------------------------
//Pins A0-A7 (PORTF) -- first 8 connections of A_T1 in order
//Pins A8-A11 (PORTK) -- last 4 connections of A_T1 in order
//--------------------------------------------------------
//
//+5v -> PCB 5V
//GND -> PCB 5GND
//
// See PCB Documentation for more info
//--------------------------------------------------------

#define button_pin 2

//bool buttonPress = false;
int cable = 0;
unsigned int input;
unsigned int output;
int current;
int greenLED = 8;
int redLED = 7;

void setup() {
  // put your setup code here, to run once:
  DDRF = 0b00000000; //set A0-7 to input
  DDRK = 0b00000000; //set A8-11 to input

  DDRA = 0b11111111; //set pin 22-29 to output
  DDRC = 0b11111111; //set pin 30-33 to output
  
  //attachInterrupt(digitalPinToInterrupt(2), buttonPressed, RISING);
  Serial.begin(9600);
}
void loop() {
  // -----Main Data Stream
  if(digitalRead(button_pin)){
    //wait for button to debounce
    delay(100);
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, LOW);
    //set up inputs
    Serial.println("now testing.....");
    Serial.println("******************************************");
    cable = 0;
    input = 0b000000000001; //start by testing the first input
    updatePort(input);    //since input is 12-bits, we use 2 Ports (A and C) and need to save our input data to them to actually change the input
    delay(100);
    //begin checking the outputs for every input
    for(int i = 1; i<13; i++){
      //wait for stabilization
      delay(100);
      //recieve the current outputs as a 12-bit unsigned int
      output = grabOutput();
      for(int j=0;j<1000;j++){
        if(grabOutput() != output){
          output = grabOutput();
          Serial.println("Output Unstable");    //verify that the output is stable
          Serial.print("New Output: ");
          Serial.println(output);
        }
      }
      //Serial.print("input: ");
      //Serial.println(input);
      //Serial.print("output: ");
      //Serial.println(output);

      //Specify the current connection
      Serial.print("Connection ");
      Serial.print(i);
      Serial.print(": ");
      delay(100);

      //check input against output and see if connection functions properly
      if(input == output){
        //connection passes
        Serial.println("PASS");
        Serial.println("----------------------------");
        //stores the number of connections that pass
        cable++;
      }else{
        //connection fails
        Serial.println("FAIL");
        //determine whether there is a short or a no connect
        checkError(input, output);
        Serial.println("----------------------------");
      }
      input = input << 1;
      updatePort(input);
    }
    //if all connections pass
    if(cable >= 12){
      Serial.println();
      Serial.println("CABLE PASSES");
      Serial.println();
      Serial.println("******************************************");
      Serial.println();
      cable = 0;
      digitalWrite(greenLED, HIGH);
      
    }else{
      Serial.println();
      Serial.println("CABLE FAILS");
      Serial.println();
      Serial.println("******************************************");
      Serial.println();
      cable = 0;
      digitalWrite(redLED, HIGH);
    }
    //avoid infinite loop
    //buttonPress = false;
    delay(500);
  }
  //button hasn't been pressed
  delay(500);
}

//---------HELPER METHODS----------

//ISR attached to the button input pin
//void buttonPressed(){
  //Serial.println("button pressed");
  //buttonPress = true;
//}
//Translates a 12bit input into 2 seperate ports
void updatePort(int input){
  unsigned int A = input | 0b00000000;
  A &= 0b000011111111; 
  unsigned int C = input >> 8; //most significant 4 digits
  updatePortC(C);
  PORTA = A;
}
//helper method necessary because port C is in opposite direction as pin labels
void updatePortC(unsigned int C){
    if(C==0){
      //PORTC = 0b00000000;
      digitalWrite(33, LOW);
      digitalWrite(32, LOW);
      digitalWrite(31, LOW);
      digitalWrite(30, LOW);
    }
    if(C==1){
      //PORTC = 0b10000000;
      digitalWrite(33, LOW);
      digitalWrite(32, LOW);
      digitalWrite(31, LOW);
      digitalWrite(30, HIGH);
    }
    if(C==2){
      //PORTC = 0b01000000;
      digitalWrite(33, LOW);
      digitalWrite(32, LOW);
      digitalWrite(31, HIGH);
      digitalWrite(30, LOW);
    }
    if(C==4){
      //PORTC = 0b00100000;
      digitalWrite(33, LOW);
      digitalWrite(32, HIGH);
      digitalWrite(31, LOW);
      digitalWrite(30, LOW);
    }
    if(C==8){
      //PORTC = 0b00010000;
      digitalWrite(33, HIGH);
      digitalWrite(32, LOW);
      digitalWrite(31, LOW);
      digitalWrite(30, LOW);
    }
}
//returns the output from both o/p ports as a single 12 bit number
int grabOutput(){
  unsigned int first = PINF;
  unsigned int second = PINK << 8;
  unsigned int output = first | second;
  output &= 0b0000111111111111;
  //Serial.print("output: ");
  //Serial.println(output);
  return output;
}
//checks for a difference between the output and input and identifies whether theres a short or no connect or both
//also prints this information to the serial monitor
void checkError(int input, int output){
  checkNoConnect(input, output);
  unsigned int checkShort = output ^ input;  //XOR -- finds all 1's corresponding to an incorrect connection
  unsigned int index = findIndex(input);
  //find all the 1's in checkShort and their indexes
  for(int i=0;i<12;i++){
    if((checkShort & 0b00000001) && (index!=(i+1))){  //check if current first bit is high
      Serial.print("\t");
      Serial.print(getColor((int)index)); //connection being checked
      Serial.print(" shorts to ");
      Serial.println(getColor(i+1));            //connection it is shorted to
      checkShort = checkShort >> 1; //shift right to check next bit
    }else{
      checkShort = checkShort >> 1; //no short detected, check next connection
    }
  }
  
}
void checkNoConnect(unsigned int input, unsigned int output){
  if(input&output){  //checks if given input yields a HIGH output for its corresponding connection
    return;
  }else{
    Serial.print("\tConnection ");
    Serial.print(findIndex(input));
    Serial.print(": No Connect (");
    Serial.print(getColor(findIndex(input)));
    Serial.println(")");
  }
}
//finds the index of the '1' in a given input (which should only include a single bit set to 1)
//essentially the base 2 log of a 112 bit number with on a single bit set
int findIndex(int input){
  int currConnect = 1;
  for(int i = 1;i < 13; i++){
    if(input == 1){
      return currConnect;
    }
    else{
      input = input >> 1;
      currConnect++;
    }
  }
  return -1;
}
//maps the index of a given connection to the physical color of the wire
String getColor(int index){
  switch(index){
    case 1:
      return "Drain";
    case 2:
      return "Yellow(VEE)";
    case 3:
      return "White (GND)";
    case 4:
      return "Red (VCC)";
    case 5:
      return "Light Orange";
    case 6:
      return "Orange";
    case 7:
      return "Light Green";
    case 8:
      return "Green";
    case 9:
      return "Light Brown";
    case 10:
      return "Brown";
    case 11:
      return "Light Blue";
    case 12:
      return "Blue";
  }
  return "none";
}
