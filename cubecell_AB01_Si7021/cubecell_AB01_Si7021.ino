#include "LoRaWan_APP.h"
#include "Adafruit_Si7021.h"

Adafruit_Si7021 sensor = Adafruit_Si7021();

// devEUI, MSB:  65 dd 65 5e 4b df ad a3
// App Key, MSB: 0xC2, 0xC7, 0x7A, 0x56, 0xBA, 0x59, 0x6E, 0x13, 0xC6, 0xD9, 0xF8, 0xB4, 0xCF, 0x9F, 0xD3, 0xD5
///* OTAA para*/
//uint8_t devEui[] = { 0x65, 0xdd, 0x65, 0x5e, 0x4b, 0xdf, 0xad, 0xa3 };
//uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//uint8_t appKey[] = { 0xC2, 0xC7, 0x7A, 0x56, 0xBA, 0x59, 0x6E, 0x13, 0xC6, 0xD9, 0xF8, 0xB4, 0xCF, 0x9F, 0xD3, 0xD5 };

///* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;
//================the99_heat=================================//
// 1328c3aadcb9eb45
uint8_t devEui[] = { 0x13, 0x28, 0xc3, 0xaa, 0xdc, 0xb9, 0xeb, 0x45 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// 09d340eed4bbf771976997ca2a7dd9fe
uint8_t appKey[] = { 0x09, 0xd3, 0x40, 0xee, 0xd4, 0xbb, 0xf7, 0x71, 0x97, 0x69, 0x97, 0xca, 0x2a, 0x7d, 0xd9, 0xfe };

//================office_heat=================================//
// 70b3d57ed0059b19
// uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x05, 0x9B, 0x19 };
// uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// c7b1e1dd7272e8fd719675f707ee27ea
// uint8_t appKey[] = { 0xC7, 0xB1, 0xE1, 0xDD, 0x72, 0x72, 0xE8, 0xFD, 0x71, 0x96, 0x75, 0xF7, 0x07, 0xEE, 0x27, 0xEA };

uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };
// LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_US915;
// DeviceClass_t  loraWanClass = LORAWAN_CLASS;
DeviceClass_t  loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 60000*1;
// bool overTheAirActivation = LORAWAN_NETMODE;
bool overTheAirActivation = true;
// bool loraWanAdr = LORAWAN_ADR;
bool loraWanAdr = false;
// bool keepNet = LORAWAN_NET_RESERVE;
bool keepNet = false;
// bool isTxConfirmed = LORAWAN_UPLINKMODE;
bool isTxConfirmed = false;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

float h,t;
uint16_t b;

static void prepareTxFrame( uint8_t port )
{
  readSensor(); //temp, humidity as float

  appDataSize = 10;
  unsigned char *puc;
  puc = (unsigned char *)(&t);
  appData[0] = puc[0];
  appData[1] = puc[1];
  appData[2] = puc[2];
  appData[3] = puc[3];
  puc = (unsigned char *)(&h);
  appData[4] = puc[0];
  appData[5] = puc[1];
  appData[6] = puc[2];
  appData[7] = puc[3];

  appData[8] = (uint8_t)(b >> 8);
  appData[9] = (uint8_t)b;
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

void readSensor() {
    b = getBatteryVoltage();
    digitalWrite(GPIO0, LOW);
    digitalWrite(Vext, LOW);
    delay(100);
    if (!sensor.begin())
    {
        Serial.println("Did not find Si7021 sensor!");
        digitalWrite(Vext, HIGH);
        delay(1000);
        return;
    }
    h = sensor.readHumidity();
    t = sensor.readTemperature();
    Serial.print("Bat: ");
    Serial.print(b/1000.);
    Serial.print(" Humidity:    ");
    Serial.print(h, 2);
    Serial.print("\tTemperature: ");
    Serial.println(t, 2);
    Wire.end();
    // Vext OFF
    digitalWrite(Vext, HIGH);
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
