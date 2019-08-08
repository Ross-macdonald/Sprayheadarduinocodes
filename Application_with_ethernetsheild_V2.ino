#include "max6675.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3,POSITIVE);

//ethernet
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

///////////////////////////////////////
///////////////PINS////////////////////
///////////////////////////////////////
//thermocouple
int thermoDO = 7;
int thermoCS = 5;
int thermoCLK = 6;
//heater
int heatpin = A2;
//Motor
const int stepPin = A1; 
const int dirPin = 3; 
const int ms3Pin = 8;
const int ms2Pin = 9;
const int ms1Pin = 10;
const int PosPin = A0;
int MOTpwr = 2;
///////////////////////////////////////
////////////////Commands///////////////
///////////////////////////////////////
//String LVcommand;
char LVcommandarray[10];
char c;
String Heater_command; //These have a seperate command as the LVcommand is reset each loop while the heater and termocouple must stay on
String Thermo_command;
char Type;
//Heater and thermocouple
char Heater_on = '0';
char Heater_temp_array[4];
String Heater_temp;
char thermo_enabled;
char thermo_unit;
char thermoyesno;
//Motor
char steps[6];
String stepstring;
int No_steps = 0;
char direc;
char steptype;
char stepping_mode;
char reset_command;
int pos = 0;
int step_no = 0;
///////////////////////////////////////
///////////////PID/////////////////////
///////////////////////////////////////
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

// Enable thermocouple
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup() {
  //enables a serial connection(usb)
  Serial.begin(9600);
  //Settup pin modes
  pinMode(MOTpwr,OUTPUT);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(ms1Pin,OUTPUT);
  pinMode(ms2Pin,OUTPUT);
  pinMode(PosPin,INPUT);
  pinMode(heatpin, OUTPUT);
  //Start the ethernet server
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP()); 
  //Allow thermocouple to config
  delay(500);
  //Start time for PID 
  Time = millis();
  
}

void loop() {
  EthernetClient client = server.available();
  if (client) { //if data is detected at the ethernet connection
      for(int x = 0; x < 10; x++){
      c = client.read();
      if(c == 'E'){
        for(int y = x; y<10; y++){
          LVcommandarray[y] = '\0'; //The E character represents the end of the command, if it is detected the rest of the command line is filled with null
        }
      }
      else{
        LVcommandarray[x] = c; //otherwise the command character is added to this charstar pointer
      }
     }
    LVcommandarray[10] == '\0'; //ensures that the charstar is null terminated
    Type = LVcommandarray[0]; //The type of command i.e. what device is classified by the 1st character sent. This determines how the rest of the command is treated

    //Heater command
    if(Type == 'H'){//H for heater
      Heater_on = LVcommandarray[1];
      Heater_temp_array[0] = LVcommandarray[2];
      Heater_temp_array[1] = LVcommandarray[3];
      Heater_temp_array[2] = LVcommandarray[4];
      Heater_temp_array[4] = 0;
      Heater_temp = String(Heater_temp_array);
      set_temperature = Heater_temp.toInt();
    }
    else if(Type == 'T'){
      thermoyesno = LVcommandarray[1];
      thermo_unit = LVcommandarray[2];    
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
      }
    }
      else if(Type == 'R'){
      reset_command = LVcommandarray[1];
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
    direc = LVcommandarray[1];
    steps[0] = LVcommandarray[3];
    steps[1] = LVcommandarray[4];
    steps[2] = LVcommandarray[5];
    steps[3] = LVcommandarray[6]; 
    steps[4] = LVcommandarray[7];
    steps[5] = 0;//null terminator
    stepstring = String(steps);
    Serial.println(stepstring);
    No_steps = stepstring.toInt();
    stepping_mode = LVcommandarray[2];
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

    for(int z = 0; z<11; z++){
      LVcommandarray[z] = '\0'; //The E character represents the end of the command, if it is detected the rest of the command line is filled with null
    }
  }
    
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
