/*code by Vineeth Rao
finished on 21/09/2023 3 days before leaving to my college.
A device to feed my dog,Kuku, when were away.
compatible with an app on mit app inventor*/

//Uses I2C communication to get information from Real time cloack
#include<Wire.h>

//Stepper pins 
#define stepMove 3
#define stepDir 2
#define stepSleep 6
#define stepReset 7
//A button thats normally closed attatched to pin 4 and 5Bbutton opens when the feeder is compoletely closed.
#define stepLow 4
#define stepCheck 5
//buzzer attatched to pin 8 and 11. negative to 8 and posistive to 11. Purpose is to condition kukku to respond to frequency of about 2300 hz
#define buzzerNeg 8
#define buzzerPos 11
//led to assist with bluetooth communication
#define ledPos A0
#define ledNeg A2
//global variables used by functions to store time and avoid accessing RTC many times.
int year,month,day,hour,minute,second;
//global variable used to store incoming alarm timings
char time[21]={'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-'};
//bluetooth algorithm variables
bool bluetooth=false;
void setup(){
  //initialise communication
  Serial.begin(9600);
  Wire.begin();
  //Stepper motor only turns when stepSleep and stepReset are high. initially we dont want it to move. direction of motor determined by stepDir. 
  //stepDir to 1, feeder starts closing,0 and it starts opening. a pulse of 5v moves the motor 1 step. each step is 1.8 degrees.
  pinMode(stepMove,OUTPUT);
  pinMode(stepDir,OUTPUT);
  pinMode(stepSleep,OUTPUT);
  digitalWrite(stepSleep,LOW);
  pinMode(stepReset,OUTPUT);
  digitalWrite(stepMove,LOW);
  digitalWrite(stepReset,LOW);
  //stepcheck is pullledmto high. stepLow is written 0
  pinMode(stepCheck,INPUT_PULLUP);
  pinMode(stepLow,OUTPUT);
  digitalWrite(stepLow,LOW);
  //buzzer pins
  pinMode(buzzerNeg,OUTPUT);
  pinMode(buzzerPos,OUTPUT);
  digitalWrite(buzzerNeg,LOW);
  digitalWrite(buzzerPos,LOW);
  //led pins
  pinMode(ledPos,OUTPUT);
  pinMode(ledNeg,OUTPUT);
  digitalWrite(ledPos,LOW);
  digitalWrite(ledNeg,LOW);
  //first step is to check if feeder is closed. otherwise it may try to open more than its supposed to and break.
  checkIfClosed();
   //checking for alarms. check alarm returns 0 if there are no alarms and will return position of alarm in the memory of RTC if there is one.
  if(checkAlarm()!=0){
    feed();
    deleteAlarm(checkAlarm());
  }
  //wait 2 mins for a ping to occur
  digitalWrite(ledPos,HIGH);
  Serial.setTimeout(120000);
  Serial.readBytes(time,1);
  if(time[0]=='t'){
    bluetooth=true;
  }

  while(bluetooth==true){
    char state[1];
    Serial.setTimeout(60000);
    if(Serial.readBytes(state,1)==1){
      if(state[0]=='a'){
        Serial.setTimeout(1000);
        Serial.readBytes(time,12);
        Serial.println("alarm");
        store(epoch(int(time[10]-'0')*10+int(time[11]-'0'),int(time[6]-'0')*10+int(time[7]-'0'),int(time[4]-'0')*10+int(time[5]-'0'),int(time[2]-'0')*10+int(time[3]-'0'),int(time[0]-'0')*10+int(time[1]-'0'),0)/1800);
        digitalWrite(ledPos,0);
        delay(500);
        digitalWrite(ledPos,1);
        delay(500);
        digitalWrite(ledPos,0);
        delay(500);
        digitalWrite(ledPos,1);
        delay(500);
        digitalWrite(ledPos,0);
        delay(500);
        digitalWrite(ledPos,1); 
      }
      else if(state[0]=='f'){
        Serial.println("feeding");
        feed();
      }
      else if(state[0]=='c'){
        Serial.println("clearing");
        clear();
      }
      else if(state[0]=='s'){
        Serial.println("Stopping");
        bluetooth=false;
      }
    }
    else{
      bluetooth=false;
    }
  }
  Serial.println("out of loop");
  digitalWrite(ledPos,LOW);
}

void loop(){
  delay(900000);
  if(checkAlarm()!=0){
    feed();
    deleteAlarm(checkAlarm());
  }
}
void getRTCtime(){
  Wire.beginTransmission(0x68);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(0x68,7);
  if(Wire.available()>=7){
    second=bcdtoint(Wire.read());
    minute=bcdtoint(Wire.read());
    hour=bcdtoint(Wire.read());
    Wire.read();
    day=bcdtoint(Wire.read());
    month=bcdtoint(Wire.read());
    year=bcdtoint(Wire.read());
  }
}

void checkIfClosed(){
  digitalWrite(stepReset,1);
  digitalWrite(stepSleep,1);
  digitalWrite(stepDir,1);
  int i=2300;
  while(i>0 && (digitalRead(stepCheck)==0)){
    i--;
    digitalWrite(stepMove,1);
    delay(1);
    digitalWrite(stepMove,0);
    delay(1);
  } 
  digitalWrite(stepSleep,0);
  digitalWrite(stepReset,0);
}

void feed(){
  digitalWrite(stepReset,1);
  digitalWrite(stepSleep,1);
  digitalWrite(stepDir,1);
  int i=2300;
  while(i>0 && (digitalRead(stepCheck)==0)){
    i--;
    digitalWrite(stepMove,1);
    delay(1);
    digitalWrite(stepMove,0);
    delay(1);
  }
  digitalWrite(stepDir,0);
  i=0;
  digitalWrite(buzzerPos,HIGH);
  while(i<2300 ){
    i++;
    digitalWrite(stepMove,1);
    delay(1);
    digitalWrite(stepMove,0);
    delay(1);
  }
  digitalWrite(stepDir,1);
  while(i>0 && (digitalRead(stepCheck)==0)){
    i--;
    digitalWrite(stepMove,1);
    delay(1);
    digitalWrite(stepMove,0);
    delay(1);
  }
  digitalWrite(stepSleep,0);
  digitalWrite(stepReset,0);
  digitalWrite(buzzerPos,0);
}

long epoch(int year,int month,int day, int hour,int minute, int seconds){
  long calculatedseconds=0;
  for(int i=(1970);i<year;i++){
    if(i%4==0){
      calculatedseconds+=31622400;
    }else{
      calculatedseconds+=31536000;
    }
  }
  for(int i=1;i<month;i++){
    if(i<=7){
      if(i%2==1){
        calculatedseconds+=2678400;
      }
      else{
        if(i==2){
          if(year%4==0){
            calculatedseconds+=2505600;
          }
          else{
            calculatedseconds+=2419200;
          }
        }
        else{
          calculatedseconds+=2592000;
        }
      }
    }
    else{
      if(i%2==0){
        calculatedseconds+=2678400;
      }
      else{
        calculatedseconds+=2592000;
      }
    }
  }
  calculatedseconds+=((day-1)*86400+hour*3600+minute*60+seconds);
  return calculatedseconds;
}

void store(unsigned int var){
  int numberOfAlarms=0;
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.endTransmission();
  Wire.requestFrom(0x68,1);
  numberOfAlarms=Wire.read();
  int locToStore=numberOfAlarms*2+9;
  byte secondhalf=(var>>8);
  byte firsthalf=var-(long(secondhalf)<<8);
  Wire.beginTransmission(0x68);
  Wire.write(locToStore);
  Wire.write(secondhalf);
  Wire.write(firsthalf);
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.write(numberOfAlarms+1);
  Wire.endTransmission();
}
void clear(){
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.write(0);
  for(int i=0;i<55;i++){
    Wire.write(-1);
  }
  Wire.endTransmission();
}
void log(){
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.endTransmission();
  Wire.requestFrom(0x68,23);
  if(Wire.available()>=23){
  for(int i=0;i<23;i++)
  Serial.println(Wire.read());}
}

unsigned int getAlarm(int alarmno){
  Wire.beginTransmission(0x68);
  Wire.write(7+alarmno*2);
  Wire.endTransmission();
  Wire.requestFrom(0x68,2);
  unsigned int alarm=((Wire.read())<<8);
  alarm+=Wire.read();
  return(alarm);
}

void deleteAlarm(int pos){
  int i=7+pos*2;
  byte a=0;
  byte b=0;
  while(i<=31){
    Wire.beginTransmission(0x68);
    Wire.write(i+2);
    Wire.endTransmission();
    Wire.requestFrom(0x68,2);
    a=Wire.read();
    b=Wire.read();
    Wire.beginTransmission(0x68);
    Wire.write(i);
    Wire.write(a);
    Wire.write(b);
    Wire.endTransmission();
    i+=2;    

  }
  //decrement number of alarms by one
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.endTransmission();
  Wire.requestFrom(0x68,1);
  int numberOfAlarms=Wire.read();
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.write(numberOfAlarms-1);
  Wire.endTransmission();
}
int checkAlarm(){
  getRTCtime();
  Wire.beginTransmission(0x68);
  Wire.write(8);
  Wire.endTransmission();
  Wire.requestFrom(0x68,1);
  int no=Wire.read();
  int position=-1;
  for(int i=1;i<=no;i++){
    int alarm=getAlarm(i);
    long currentTime=epoch(year,month,day,hour,minute,second);
    currentTime=currentTime/1800;
      if(alarm<=currentTime){
        position=i;
      }
    }
    if(position>0){
      return position;
    }
    else{
      return 0;
    }
}
byte inttobcd(int k){
  int a,b;
  byte res;
  a=k%10;
  b=k/10;
  res=(byte(b)<<4)+byte(a);
  return res;
}

int bcdtoint(byte k){
  byte a,b;
  int res;
  a=k>>4;
  b=k-(a<<4);
  res=int(a)*10+int(b);
  return res;
}
