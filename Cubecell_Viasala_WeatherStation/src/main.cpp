#include <Arduino.h>
#include "LoRaWan_APP.h"
// #include "HT_SH1107Wire.h"
#include "Wire.h"

// SH1107Wire  oled(0x3c, 500000, SDA, SCL ,GEOMETRY_128_64,GPIO10); // addr, freq, sda, scl, resolution, rst

#define MAX_DATA_SIZE 42
#define MEAS_ARRAY_SIZE 23
#define DATA_ARRAY_SIZE 21


static void prepareTxFrame( uint8_t ) ;
void printHex2(unsigned);
void setupSensor();
void listenForWeather();
String split(String, char, int);

/* OTAA para*/
// devEUI, MSB:  0d 14 ea 46 86 20 52 c8
// App Key, MSB: 22 11 2b 72 e4 e1 7f 6c ad 43 7c 9a 3a 58 43 e2
uint8_t devEui[] = { 0x0d, 0x14, 0xea, 0x46, 0x86, 0x20, 0x52, 0xc8 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x22, 0x11, 0x2b, 0x72, 0xe4, 0xe1, 0x7f, 0x6c, 0xad, 0x43, 0x7c, 0x9a, 0x3a, 0x58, 0x43, 0xe2 };

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t  loraWanClass = LORAWAN_CLASS;
uint32_t appTxDutyCycle = 60000 * 5;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = LORAWAN_ADR;
bool keepNet = LORAWAN_NET_RESERVE;
bool isTxConfirmed = LORAWAN_UPLINKMODE;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

float h,t;
uint16_t b;

String input = "";
String meas[MEAS_ARRAY_SIZE];
int measINT[MEAS_ARRAY_SIZE];
float measFLOAT[MEAS_ARRAY_SIZE];
// static uint8_t data[MAX_DATA_SIZE];

void printHex2(unsigned v) {
  v &= 0xff;
  if (v < 16)
      Serial.print('0');
  Serial.print(v, HEX);
}

static void prepareTxFrame( uint8_t port )
{
  Serial.println("Checking the Weather");
  // listenForWeather();
  Serial.println("data Collected, Attempting to Send");
  appDataSize = MAX_DATA_SIZE;

  Serial1.println("0R0"); //request measurements
  delay(40);
    //Parse Measurements
  if(Serial1.available()) {
    input = Serial1.readStringUntil('\n'); //pulls whole string into variable
    for(int i = 0; i < MEAS_ARRAY_SIZE; i++) //splits the string at the comma into an array of strings
      meas[i] = split(input, ',',i);
    if(meas[0]=="0R0") { //check for correct return
      for(int i = 0; i < MEAS_ARRAY_SIZE; i++) {     //trims non-number values off each return
        meas[i].remove(0,3); //remove first three values
        meas[i].remove(meas[i].length()-1,1); //remove the last value
      }
      for(int i = 0; i < MEAS_ARRAY_SIZE-3; i++) {//converts values to floats, also trims first and last values from String Array, which hold junk values
        measFLOAT[i] = meas[i+1].toFloat();
        // Serial.print(i);
        // Serial.print(", ");
        // Serial.println(measFLOAT[i]);
      }
      //scale temperatures to avoid negative values
      measFLOAT[6] += 50; //air temp
      measFLOAT[7] += 50; //air temp internal
      measFLOAT[9] /= 10; //air pressure (div by 10, or it will roll over when upscaled to an INT)
      measFLOAT[18] += 50;  //heating temp
      measFLOAT[20] = getBatteryVoltage(); //battery voltage
      measFLOAT[20] /= 1000; //battery voltage (scale for CubeCell)

      for(int i = 0; i < DATA_ARRAY_SIZE; i++) {//print scaled values as a check
        Serial.print(i);
        Serial.print(", ");
        Serial.println(measFLOAT[i]);
      }
      //converts floats into char pairs
      for(int i = 0; i < DATA_ARRAY_SIZE; i++) {
        measINT[i] = measFLOAT[i] * 100; //upscale data and convert floats to ints
        appData[i*2] = measINT[i] >> 8;
        appData[(i*2)+1] = measINT[i];
      }
    }
  }
}

void setup()
{
  boardInitMcu();
  // oled.init();
  // oled.screenRotate(ANGLE_180_DEGREE);
  // oled.clear();
  // oled.setFont(ArialMT_Plain_10);
  // oled.setTextAlignment(TEXT_ALIGN_LEFT);
  // oled.setFont(ArialMT_Plain_10);
  // oled.drawString(0, 0, "Hello world");
  // oled.setFont(ArialMT_Plain_16);
  // oled.drawString(0, 10, "Hello world");
  // oled.setFont(ArialMT_Plain_24);
  // oled.drawString(0, 26, "Hello world");
  // oled.display();
  // delay(5000);
  // oled.clear();
  Serial.begin(115200);
  Serial1.begin(9600);
  // while(1) {
  //   listenForWeather();
  //   // Serial1.println("hello");
  //   delay(1000);
  // }
#if(AT_SUPPORT)
  enableAt();
#endif
  LoRaWAN.displayMcuInit(); // 	display.screenRotate(ANGLE_180_DEGREE);
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();
  appDataSize = MAX_DATA_SIZE;
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(AT_SUPPORT)
      getDevParam();
#endif
      printDevParam();
      LoRaWAN.init(loraWanClass,loraWanRegion);
      deviceState = DEVICE_STATE_JOIN;
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.displayAck();
      LoRaWAN.sleep();
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}

void listenForWeather() {
  /**
   *    0R0,Dn=209D,Dm=281D,Dx=035D,Sn=0.0M,Sm=0.1M,Sx=0.3M,Ta=19.7C,Tp=20.0C,Ua=31.0P,Pa=837.3H,Rc=0.00M,Rd=0s,Ri=0.0M,Hc=0.0M,Hd=0s,Hi=0.0M,Rp=0.0M,Hp=0.0M,Th=19.2C,Vr=3.542V,Id=Hel
   * 
   * 0R0,
   * Dn=209D,
   * Dm=281D,
   * Dx=035D,
   * Sn=0.0M,
   * Sm=0.1M,
   * Sx=0.3M,
   * Ta=19.7C,
   * Tp=20.0C,
   * Ua=31.0P,
   * Pa=837.3H,
   * Rc=0.00M,
   * Rd=0s,
   * Ri=0.0M,
   * Hc=0.0M,
   * Hd=0s,
   * Hi=0.0M,
   * Rp=0.0M,
   * Hp=0.0M,
   * Th=19.2C,
   * Vr=3.542V,
   * Id=Hel
   */

  /*
0, 106.00   DIR_MIN
1, 278.00   DIR_AVG
2, 27.00    DIR_MAX
3, 0.10     SPD_LUL
4, 0.10     SPD_AVG
5, 0.20     SPD_GST
6, 19.70    AIR_TEM
7, 19.90    AIR_INT
8, 32.00    AIR_HUM
9, 837.30   AIR_PRS
10, 0.00    RAI_AMT
11, 0.00    RAI_DUR
12, 0.00    RAI_INT
13, 0.00    HAI_AMT
14, 0.00    HAI_DUR
15, 0.00    HAI_INT
16, 0.00    RAI_PEK
17, 0.00    HAI_PEK
18, 19.00   HEA_TEM
19, 3.54    REF_VOL
20, x.xx    BAT_VOL
*/

  Serial1.println("0R0"); //request measurements
  delay(40);
    //Parse Measurements
  if(Serial1.available()) {
    input = Serial1.readStringUntil('\n'); //pulls whole string into variable
    for(int i = 0; i < MEAS_ARRAY_SIZE; i++) //splits the string at the comma into an array of strings
      meas[i] = split(input, ',',i);
    if(meas[0]=="0R0") { //check for correct return
      for(int i = 0; i < MEAS_ARRAY_SIZE; i++) {     //trims non-number values off each return
        meas[i].remove(0,3); //remove first three values
        meas[i].remove(meas[i].length()-1,1); //remove the last value
      }
      for(int i = 0; i < MEAS_ARRAY_SIZE-3; i++) {//converts values to floats, also trims first and last values from String Array, which hold junk values
        measFLOAT[i] = meas[i+1].toFloat();
        // Serial.print(i);
        // Serial.print(", ");
        // Serial.println(measFLOAT[i]);
      }
      //scale temperatures to avoid negative values
      measFLOAT[6] += 50; //air temp
      measFLOAT[7] += 50; //air temp internal
      measFLOAT[9] /= 10; //air pressure (div by 10, or it will roll over when upscaled to an INT)
      measFLOAT[18] += 50;  //heating temp
      measFLOAT[20] = getBatteryVoltage(); //battery voltage
      measFLOAT[20] /= 1000; //battery voltage (scale for CubeCell)

      for(int i = 0; i < DATA_ARRAY_SIZE; i++) {//print scaled values as a check
        Serial.print(i);
        Serial.print(", ");
        Serial.println(measFLOAT[i]);
      }
      //converts floats into char pairs
      for(int i = 0; i < DATA_ARRAY_SIZE; i++) {
        measINT[i] = measFLOAT[i] * 100; //upscale data and convert floats to ints
        appData[i*2] = measINT[i] >> 8;
        appData[(i*2)+1] = measINT[i];
      }
    }
  }
}


String split(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++) {
    if(data.charAt(i)==separator || i==maxIndex) {
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setupSensor() {
  //Sets weather station config
  //See manual for details.
  //This is the response string.
  
  //0R0,Dn=270D,Dm=302D,Dx=339D,Sn=0.1M,Sm=0.1M,Sx=0.2M,Ta=19.1C,Tp=19.4C,Ua=31.0P,Pa=837.5H,Rc=0.00M,Rd=0s,Ri=0.0M,Hc=0.0M,Hd=0s,Hi=0.0M,Rp=0.0M,Hp=0.0M,Th=18.8C,Vr=3.533V,Id=Hel␍␊
  delay(100);

  //Set Wind Settings
  Serial1.println("0WU,R=11111100&11111100");
  delay(100);
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0WU,I=60,A=60,G=3,U=M,D=0");
  delay(100);
  Serial1.println(input);
  Serial1.println("0WU,N=W,F=4");
  delay(100);

  //Set Pressure/Temperature/Humidity Settings
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0TU,R=11110000&11110000");
  delay(100);
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0TU,I=60,P=H,T=C");
  delay(100);

  //Set Rain/Hail Settings
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0RU,R=11111111&11111111");
  delay(100);
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0RU,I=60,U=M,S=M,M=T");
  delay(100);
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0RU,Z=A,X=65535,Y=65535");
  delay(100);

  //Set System Settings
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0SU,R=00110000&00110000");
  delay(100);
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.println("0SU,I=60,S=N,H=N");
  delay(100);
  if(Serial1.available()) { input = Serial1.readStringUntil('\n'); }
  Serial1.println(input);
  Serial1.flush();
  Serial1.println("0R0");
  delay(40);
  if(Serial1.available()) {
    input = Serial1.readStringUntil('\n');
  }
  Serial1.println(input);
}