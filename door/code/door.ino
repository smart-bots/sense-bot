#include <EEPROM.h>
#include <SPI.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

RF24 radio(7,8);

#define nrfIrqPin 2
#define reedSwchPin 3
#define buzzerPin 6

const char token[] = "abcabcabc0"; // token

const uint64_t pipes[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
// pipe.1: read | pipe.2: write -> oposite with hub

bool buzzerVal;

unsigned long time1;

bool timeToSleep = false;

struct recei {
  byte type;
  char token[11];
  short state;
} recei;

struct trans {
  byte type;
  char token[11];
  short state;
  float data;
} trans;

//----------------------------------------------------------------------------------------

void go_to_sleep() {
//  Serial.println("Sleeping...");
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}

//----------------------------------------------------------------------------------------

void send_data(float data) {

  trans.type = 4;
  strncpy(trans.token,token,10);
  trans.state = 0;
  trans.data = data;

  radio.stopListening();
  radio.write(&trans,sizeof(trans));
  radio.startListening();

}

//----------------------------------------------------------------------------------------

void check_radio(void) {
  
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);
  
  if (rx || radio.available()) {

    radio.read(&recei,sizeof(recei));

    Serial.println("Got -----------------");
    Serial.println(recei.type);
    Serial.println(recei.token);
    Serial.println(recei.state);
    Serial.println("---------------------");
 
    if (strcmp(recei.token,token) == 0  && recei.type == 2) {
      // Ack check online
    }
  }
}

//----------------------------------------------------------------------------------------

void data_changed() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time >= 100) 
  {
    timeToSleep = false;
    buzzerVal = 1;
    
    Serial.print("DATA CHANGED: ");
    Serial.println(digitalRead(reedSwchPin));
    
    send_data(digitalRead(reedSwchPin));
    
    last_interrupt_time = interrupt_time;
  }
}

//----------------------------------------------------------------------------------------

void setup()
{
  Serial.begin(9600);

  radio.begin();
//  printf_begin();
//  radio.printDetails();
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.openReadingPipe(1,pipes[0]);
  radio.openWritingPipe(pipes[1]);
  radio.startListening();

  pinMode(buzzerPin, OUTPUT);
  pinMode(reedSwchPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(nrfIrqPin), check_radio, LOW);
  attachInterrupt(digitalPinToInterrupt(reedSwchPin), data_changed, CHANGE);

  timeToSleep = true;
}


//----------------------------------------------------------------------------------------

void handle_buzzer() {
  if (buzzerVal == 1)
  {
    analogWrite(buzzerPin, 255);
    buzzerVal = 0;
//    Serial.println("BEEP");
  } else {
    analogWrite(buzzerPin, 0);
    timeToSleep = true;
  }
}

void loop() {
  //----------------------------------------------------------------------------------------
  if ((unsigned long)(millis()-time1) > 100)
  {
    handle_buzzer();
    time1 = millis();
  }
  //----------------------------------------------------------------------------------------
  if (timeToSleep) {
    delay(200);
    go_to_sleep();
  }
}

