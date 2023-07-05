#include "Arduino.h"
#include "LoRaWan_APP.h"

/* ABP para*/
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00 };
uint32_t devAddr =  ( uint32_t )0x00000000;
/* OTAA para*/
uint8_t devEui[] = { 0x00, 0x07, 0x07, 0x2E, 0x0B, 0x2E, 0x11, 0x51 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x08, 0xD3, 0x59, 0xCE, 0xEF, 0x92, 0xC5, 0x46, 0xC8, 0x85, 0x27, 0x48, 0x94, 0x44, 0x38, 0x75 };

uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 }; //for TTN
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_US915;
DeviceClass_t  loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 60000*1;
bool overTheAirActivation = true;
bool loraWanAdr = false;
bool keepNet = false;
bool isTxConfirmed = false;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

float d;
uint16_t b;

//this is an example, but the battery code is real!
void readSensor() {
    b = getBatteryVoltage();
    digitalWrite(GPIO0, LOW);
    digitalWrite(Vext, LOW);
    delay(100);
    d = random(0,1000)/1000.;
    Serial.print("Battery: ");
    Serial.print(b/1000.);
    Serial.print(" Data: ");
    Serial.println(d, 2);
    // Vext OFF
    digitalWrite(Vext, HIGH);
}

static void prepareTxFrame( uint8_t port )
{
  readSensor(); 
  appDataSize = 6;
  unsigned char *puc;
  puc = (unsigned char *)(&d);
  appData[0] = puc[0];
  appData[1] = puc[1];
  appData[2] = puc[2];
  appData[3] = puc[3];

  appData[4] = (uint8_t)(b >> 8);
  appData[5] = (uint8_t)b;
}

//downlink data handle function example
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  Serial.printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  Serial.print("+REV DATA:");
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
  {
    Serial.printf("%02X",mcpsIndication->Buffer[i]);
  }
  Serial.println();
  uint32_t color = mcpsIndication->Buffer[0]<<16|mcpsIndication->Buffer[1]<<8|mcpsIndication->Buffer[2];
  uint32_t delay = (mcpsIndication->Buffer[3]) * 10;
  Serial.println(delay);
// #if(LoraWan_RGB==1)
  turnOnRGB(color,delay);
  turnOffRGB();
// #endif
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




/* --- DECODER for Chirpstack --- */
/* http://community.heltec.cn/t/please-help-heltec-cubecell-htcc-ab01-with-ds18b20-on-ttn-lorawan/2678/3 */
/*
 function Decode(fPort, bytes, variables) {
  var decoded = {
    "temperature":{},
    "humidity":{},
    "battery":{},
  };
  t = 0
  h = 4;
  decoded.humidity = bytesToFloat(bytes.slice(h,h+=4));
  decoded.temperature = bytesToFloat(bytes.slice(t,t+=4));
  decoded.battery = ((bytes[8] << 8) + bytes[9])/1000.;
  return decoded;
}

function bytesToInt(by) {
  f = by[0] | by[1]<<8 | by[2]<<16 | by[3]<<24;
  return f;
}

function bytesToFloat(by) {
  var bits = by[3]<<24 | by[2]<<16 | by[1]<<8 | by[0];
  var sign = (bits>>>31 === 0) ? 1.0 : -1.0;
  var e = bits>>>23 & 0xff;
  var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000;
  var f = sign * m * Math.pow(2, e - 150);
  return f;
}

*/
