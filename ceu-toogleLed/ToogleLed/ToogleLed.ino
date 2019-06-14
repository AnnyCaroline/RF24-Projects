#include <SPI.h>
#include "RF24.h"

//Receivers
const int r_ledPin = 3;      // the number of the LED pin
int r_ledState = LOW;


//Sender
const int s_btnPin = 4;
int s_btnState;
int s_btnControl = 0;
const int s_ledErrorPin = 5;

//Normaly uses radio(7,8), but in Céu we're using 7,8.
RF24 radio(8,7);

byte addresses[][6] = {"1Node","2Node"};

// Used to control whether this node is sending (1) or receiving (0)
bool role = 0;
bool radioNumber = role;

void setup() {
  Serial.begin(115200);
  Serial.print("Role: ");
  Serial.println(role);

  pinMode(r_ledPin, OUTPUT);
  pinMode(s_ledErrorPin, OUTPUT);
  
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  
  // Start the radio listening for data
  radio.startListening();
}

void loop() {
  

  //////////////////
  // Sender
  //////////////////
  if (role == 1)  {

    // Send a message after button press and release
    s_btnState = digitalRead(s_btnPin);
    delay(25);
    
    if (s_btnState == HIGH && s_btnControl == 0) {
      s_btnControl = 1;
    }
    else if (s_btnState == LOW  && s_btnControl == 1){
      s_btnControl = 2;
    }
    else if (s_btnControl == 2){
      // The button was pressed and realeased, so send a message
  
      radio.stopListening();
        
      Serial.println("Now sending");

      // Send a message with the current time
      digitalWrite(s_ledErrorPin, LOW);
      unsigned long start_time = micros();
      if (!radio.write( &start_time, sizeof(unsigned long) )){
        Serial.println("failed");
        digitalWrite(s_ledErrorPin, HIGH);
      }
            
      radio.startListening();
        
      unsigned long started_waiting_at = micros();
      boolean timeout = false;

      // While nothing is received
      while ( ! radio.available() ){                             
        if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
        }      
      }
            
      if ( timeout ){ 
        Serial.println("Failed, response timed out.");
        digitalWrite(s_ledErrorPin, HIGH);
      }else{
        unsigned long got_time;                                 // Grab the response, compare, and send to debugging spew
        radio.read( &got_time, sizeof(unsigned long) );
        unsigned long end_time = micros();
        
        // Spew it
        Serial.print("Sent ");
        Serial.print(start_time);
        Serial.print(", Got response ");
        Serial.print(got_time);
        Serial.print(", Round-trip delay ");
        Serial.print(end_time-start_time);
        Serial.println(" microseconds");
      }
  
      s_btnControl = 0;
    }
      
  
  
  }




  //////////////////
  // Receiver
  //////////////////

  if ( role == 0 )
  {
    unsigned long got_time;
    
    if( radio.available()){
                                                                    // Variable for the received timestamp
      while (radio.available()) {                                   // While there is data ready
        radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
      }

      r_ledState = !r_ledState;
      digitalWrite(r_ledPin, r_ledState);
     
      radio.stopListening();                                        // First, stop listening so we can talk   
      radio.write( &got_time, sizeof(unsigned long) );              // Send the final one back.      
      radio.startListening();                                       // Now, resume listening so we catch the next packets.     
      Serial.print("Sent response ");
      Serial.println(got_time);
   }
 }


} // Loop
