//Setup thermocouple
#include "max6675.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3,POSITIVE);
int thermoDO = 7;
int thermoCS = 5;
int thermoCLK = 6;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
//int vccPin = 3;
int MOTpwr = 2;
int heatpin = 11;
int step_no = 0;

//setup motor
const int stepPin = 4; 
const int dirPin = 3; 
const int ms3Pin = 8;
const int ms2Pin = 9;
const int ms1Pin = 10;
const int PosPin = A0;


//Command Strings
String LVcommand;
String Heater_command;
String Thermo_command;
char Type;
char Heater_on = '0';
char Heater_temp_array[4];
String Heater_temp;
char thermo_enabled;
char thermo_unit;
char thermoyesno;
char steps[6];
String stepstring;
int No_steps = 0;
char direc;
char steptype;
char stepping_mode;
char reset_command;
int pos = 0;

//Heater variables
int ontime;
int offtime;
float set_temperature = 0;
float temperature_read = 0.0;
float PID_error = 0;
float previous_error = 0;
float elapsedTime, Time, TimePrev;
float PID_value = 0;
//PID constants
///////////////////////////////////////
int kp = 10; int ki = 5; int kd = 160;
///////////////////////////////////////
int PID_p = 0; int PID_i = 0; int PID_d = 0;
//////////////////////////////////////

void setup() {
  Serial.begin(9600);
  pinMode(MOTpwr,OUTPUT);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(ms1Pin,OUTPUT);
  pinMode(ms2Pin,OUTPUT);
  pinMode(PosPin,INPUT);
  
  //pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  //pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);
  pinMode(heatpin, OUTPUT);
  delay(500);
  Time = millis();

}

void loop() {
  //will constantly search the serial port for incoming strings 
  while(Serial.available()){
    LVcommand = Serial.readString();
  }
  // Command can be any length that isn't 1 or 0, 0 is ovious and not 1 as a header is sent of length 1 to confirm a transfer
   if((LVcommand.length() != 0) && (LVcommand.length() != 1)){
    Type = LVcommand.charAt(0); // clasifies the command
    if(Type == 'H'){ // H for heater
      Heater_command = LVcommand; 
      Heater_on = LVcommand.charAt(1);
      Heater_temp_array[0] = LVcommand.charAt(2);
      Heater_temp_array[1] = LVcommand.charAt(3);
      Heater_temp_array[2] = LVcommand.charAt(4);
      Heater_temp_array[4] = 0;
      Heater_temp = String(Heater_temp_array);
      set_temperature = Heater_temp.toInt(); // keeps the set temp around after LVcommand is cleared
      //Serial.println(set_temperature);
    }
    else if(Type == 'T'){
      Thermo_command = LVcommand;
      thermoyesno = Thermo_command.charAt(1);
      thermo_unit = Thermo_command.charAt(2);
      if (thermoyesno == '1'){
      //C = 1 F = 0
      if (thermo_unit == '1'){
        //Serial.println("C = ");
        Serial.println(thermocouple.readCelsius());
      }
      else if (thermo_unit == '0'){ //Why did i bother with Fahrenheit its a unit for people who use pounds per square inch (i.e. idiots)
        //Serial.println("F = ");
        Serial.println(thermocouple.readFahrenheit());
      }
     // else{
      //  Serial.println("Give a valid unit you wally");
      //}
    }
    }
    else if(Type == 'R'){
      reset_command = LVcommand.charAt(1);
      if(reset_command == 'C'){
        pos = digitalRead(PosPin);
        Serial.println(pos);
      }
      if(reset_command == 'M'){
        digitalWrite(dirPin,HIGH);
        pos = digitalRead(PosPin);
        digitalWrite(MOTpwr,HIGH);
        while(pos == LOW){
          digitalWrite(stepPin,HIGH);
          delayMicroseconds(1);
          digitalWrite(stepPin,LOW);
          delayMicroseconds(1);
          delay(3);
          pos = digitalRead(PosPin);
          Serial.println(pos);
      }
      digitalWrite(MOTpwr,LOW);
      
    }
    }
    
    else if(Type == 'M'){
    digitalWrite(MOTpwr,HIGH);
    direc = LVcommand.charAt(1);
    steps[0] = LVcommand.charAt(3);
    steps[1] = LVcommand.charAt(4);
    steps[2] = LVcommand.charAt(5);
    steps[3] = LVcommand.charAt(6); 
    steps[4] = LVcommand.charAt(7);
    steps[5] = 0;//null terminator
    stepstring = String(steps);
    Serial.println(stepstring);
    No_steps = stepstring.toInt();
    stepping_mode = LVcommand.charAt(2);
    //Serial.println(Type);
    //Serial.println(direc);
    //Serial.println(stepping_mode);
    //Serial.println(No_steps);

    if(stepping_mode == '0'){//Sixteenth
      digitalWrite(ms1Pin,HIGH);
      digitalWrite(ms2Pin,HIGH);
      digitalWrite(ms3Pin,HIGH);
      //Serial.println("Sixteenth");
    }
    else if(stepping_mode == '1'){//Eighth
      digitalWrite(ms1Pin,HIGH);
      digitalWrite(ms2Pin,HIGH);
      digitalWrite(ms3Pin,LOW);
      //Serial.println("Eighth");
    }
    else if(stepping_mode == '2'){//Quater
      digitalWrite(ms1Pin,LOW);
      digitalWrite(ms2Pin,HIGH);
      digitalWrite(ms3Pin,LOW);
      //Serial.println("Quater");
    }
    else if(stepping_mode == '3'){//Half
      digitalWrite(ms1Pin,HIGH);
      digitalWrite(ms2Pin,LOW);
      digitalWrite(ms3Pin,LOW);
      //Serial.println("Half");
    }
    else if(stepping_mode == '4'){//full
      digitalWrite(ms1Pin,LOW);
      digitalWrite(ms2Pin,LOW);
      digitalWrite(ms3Pin,LOW);
      //Serial.println("Full");
    }
    delay(10);
    if(direc == 'F'){
      digitalWrite(dirPin,LOW);
      for(int x = 0; x < No_steps; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(1);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(1);
    delay(3);
    //Serial.println("Fwd");
  }
    }
    else if(direc == 'B'){
      digitalWrite(dirPin,HIGH);
      //step_no = 0;
      for(int x = 0; x < No_steps; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(1);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(1);
    delay(3);
    //step_no = step_no + 1;
    //Serial.println(step_no);
  }
    }
    digitalWrite(MOTpwr,LOW);

    }
  
  
  LVcommand = "";
  }
  //P and I
  if(Heater_on == '1'){
  //Serial.println(thermocouple.readCelsius());
  temperature_read = float(thermocouple.readCelsius());
  PID_error = set_temperature - temperature_read;
  PID_p = 0.01*kp*PID_error;
  PID_i = 0.01*PID_i +(ki*PID_error);
  //D
  TimePrev = Time;
  Time = millis();
  elapsedTime = (Time - TimePrev)/1000;
  PID_d = 0.01*kd*((PID_error - previous_error)/elapsedTime);
  PID_value = PID_p + PID_i + PID_d;
  previous_error = PID_error;
  lcd.begin(16,2);
  //lcd.clear();
  lcd.print("Temp = ");
  lcd.print(thermocouple.readCelsius());
  //Serial.println("here");
  
  //converting to power
    if(PID_value < 0){
    PID_value = 0;
    
  }
  else if (PID_value > 100){
    PID_value = 100;
  }
  //generating power
  //Serial.println(PID_value);
  ontime = 5*PID_value;
  if(ontime > 1000){
    ontime = 1000;
  }
  offtime = 1000 - ontime;
  //Serial.println(ontime);
  //Serial.println(offtime);
  digitalWrite(heatpin,HIGH);
  delay(ontime);
  digitalWrite(heatpin,LOW);
  delay(offtime);
  }
  else{
    //analogwrite(PIN,0)
    set_temperature = 0;
    PID_p = 0;
    PID_d = 0;
    PID_i = 0;
    PID_value = 0;
  }
}
