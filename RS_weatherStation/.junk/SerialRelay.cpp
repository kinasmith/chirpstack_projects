#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <RTCZero.h>
#include <SerialFlash.h>

void setup()
{
    SerialUSB.begin(115200);
    // Open serial communications and wait for port to open:
        while (!SerialUSB) {
        ; // wait for serial port to connect. Needed for Native USB only
    }
    SerialUSB.println("Goodnight moon!");

    // set the data rate for the SoftwareSerial port
    Serial.begin(9600);
    Serial.println("0R0");
}

void loop() // run over and over
{
  if (SerialUSB.available())
    Serial.write(SerialUSB.read());
  if (Serial.available())
    SerialUSB.write(Serial.read());
}


void sendConfig() {
    //Sets weather station config
    //See manual for details.
    //This is the response string.
    
    //0R0,Dn=270D,Dm=302D,Dx=339D,Sn=0.1M,Sm=0.1M,Sx=0.2M,Ta=19.1C,Tp=19.4C,Ua=31.0P,Pa=837.5H,Rc=0.00M,Rd=0s,Ri=0.0M,Hc=0.0M,Hd=0s,Hi=0.0M,Rp=0.0M,Hp=0.0M,Th=18.8C,Vr=3.533V,Id=Hel␍␊
    Serial.println("0WU,R=11111100&11111100");
    delay(500);
    Serial.println("0WU,I=60,A=60,G=3,U=M");
    delay(500);
    Serial.println("0WU,D=0,N=M,F=1");
    delay(500);
    Serial.println("0TU,R=11110000&11110000");
    delay(500);
    Serial.println("0TU,I=60,P=H,T=C");
    delay(500);
    Serial.println("0RU,R=11111111&11111111");
    delay(500);
    Serial.println("0RU,I=60,U=M,S=M,M=T");
    delay(500);
    Serial.println("0RU,Z=A,X=65525,Y=65535");
    delay(500);
    Serial.println("0SU,R=00110000&00110000");
    delay(500);
    Serial.println("0SU,I=60,S=N,H=N");
    delay(500);
}