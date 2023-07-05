#include "Arduino.h"
#include "LoRaWan_APP.h"

/* ABP para*/
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00 };
uint32_t devAddr =  ( uint32_t )0x00000000;
/* OTAA para*/
// 0007072E0B2E1151
uint8_t devEui[] = { 0x00, 0x07, 0x07, 0x2E, 0x0B, 0x2E, 0x11, 0x51 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//08D359CEEF92C546C885274894443875
uint8_t appKey[] = { 0x08, 0xD3, 0x59, 0xCE, 0xEF, 0x92, 0xC5, 0x46, 0xC8, 0x85, 0x27, 0x48, 0x94, 0x44, 0x38, 0x75 };

uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 }; //for TTN
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_US915;
DeviceClass_t  loraWanClass = CLASS_C;
// uint32_t appTxDutyCycle = 60000*1;
uint32_t appTxDutyCycle = 5000*1;

bool overTheAirActivation = true;
bool loraWanAdr = false;
bool keepNet = false;
bool isTxConfirmed = false;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

float d;
uint16_t b;
char valveStateEncoded;
uint32_t valvecmd;
const int ARRAY_SIZE = 4;  // Size of the boolean array

int valvePins[] = {2, 3, 7, 8};
const int readPins[] = {5, 6, 9, 10};
int inputArray[4];

//this is an example, but the battery code is real!
void readSensor() {
  // for (int i = 0; i < 4; i++) {
  //   inputArray[i] = digitalRead(readPins[i]);
  // }
  // // Serial.println(inputArray);
  // byte inputHex = byte(inputArray[0] << 3 | inputArray[1] << 2 | inputArray[2] << 1 | inputArray[3]);
  // String inputHexString = String(inputHex, HEX);
  // Serial.println(inputHexString);
  // valveStateEncoded = inputHexString.charAt(0);

  b = getBatteryVoltage();
  digitalWrite(GPIO0, LOW);
  digitalWrite(Vext, LOW);
  delay(100);
  // d = random(0,1000)/1000.;
  // Serial.print("Battery: ");
  // Serial.print(b/1000.);
  // Serial.print(" Data: ");
  // Serial.println(d, 2);
  // Vext OFF
  digitalWrite(Vext, HIGH);
}

static void prepareTxFrame( uint8_t port )
{
  readSensor(); 
  appDataSize = 3;
  unsigned char *puc;
  puc = (unsigned char *)(&d);
  // appData[0] = puc[0];
  // appData[1] = puc[1];
  // appData[2] = puc[2];
  // appData[3] = puc[3];
  appData[0] = (uint8_t)(b >> 8);
  appData[1] = (uint8_t)b;
  valveStateEncoded = static_cast<byte>(valvecmd); // Convert uint32_t to byte
  appData[2] = valveStateEncoded;
  // appData[2] = 13;
}

void hexToBoolArray(unsigned long hexNumber, bool* boolArray, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    boolArray[i] = (hexNumber & (1 << i)) != 0;
  }
}

//downlink data handle function example
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  uint32_t down = mcpsIndication->Buffer[0];
  if(mcpsIndication->Port == 1) {
    // unsigned long hexNumber = down;  // Example hexadecimal number
    bool boolArray[ARRAY_SIZE];
    hexToBoolArray(down, boolArray, ARRAY_SIZE);
    Serial.print("port 1, ");
    // Print the boolean array
    for (int i = 0; i < ARRAY_SIZE; i++) {
      Serial.print(boolArray[i]);
      Serial.print(" ");
    }
    Serial.println();

    //write recieved values to digital pins
    for (int i = 0; i < sizeof(valvePins)/sizeof(valvePins[0]); i++) {
      digitalWrite(valvePins[i], boolArray[i]);
    }
  }
  Serial.println(down);
  valvecmd = down;
}

void setup()
{
  boardInitMcu();
  Serial.begin(115200);
#if(AT_SUPPORT)
  enableAt();
#endif
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();
  //set mode for digital output pins to control valves
  for (int i = 0; i < sizeof(valvePins)/sizeof(valvePins[0]); i++) {
    pinMode(valvePins[i], OUTPUT);
  }
  //set mode of input pins to read valve state
  for (int i = 0; i < 4; i++) {
    pinMode(readPins[i], INPUT);
  }
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


/*
//Payload formatter

function decodeUplink(input) {
  var data = {};
  var valveState = (input.bytes[2]);
  var valveStates = decimalToBinary(valveState);
  var [valve1, valve2, valve3, valve4] = valveStates;
  data.valve_state_01 = valve1;
  data.valve_state_02 = valve2;
  data.valve_state_03 = valve3;
  data.valve_state_04 = valve4;

  data.battery = ((input.bytes[0] << 8) + input.bytes[1])/1000.;
  var warnings = [];
  if (data.battery < 3.6) {
    warnings.push("LOW BATTERY");
  }
  return {
    data: data,
    warnings: warnings
  };
}

function decimalToBinary(decimalNumber) {
  // Convert decimal number to binary string
  const binaryString = decimalNumber.toString(2);
  // Split the binary string into an array of characters
  const binaryArray = binaryString.split('');
  // Convert each binary digit to a boolean value
  const booleanVariables = binaryArray.map((digit) => digit === '1');
  // Return the array of boolean variables
  return booleanVariables;
}
*/
