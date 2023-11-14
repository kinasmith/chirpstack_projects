#include "LoRaWan_APP.h"
#include <Wire.h>
#include <SHT2x.h>
///* ABP para -- This isn't used, but needs to be included 
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

// 1 pulse per 1/10 cubic ft
uint16_t wm_pulse_cnt = 0;
uint32_t dblast = 0; //last debounce time
uint8_t dbdelay = 100; //debounce delay

// OTAA
// 1328c3aadcb9eb46
uint8_t devEui[] = { 0x13, 0x28, 0xc3, 0xaa, 0xdc, 0xb9, 0xeb, 0x46 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// 09d340eed4bbf771976997ca2a7dd9fe
uint8_t appKey[] = { 0x09, 0xd3, 0x40, 0xee, 0xd4, 0xbb, 0xf7, 0x71, 0x97, 0x69, 0x97, 0xca, 0x2a, 0x7d, 0xd9, 0xfe };


uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_US915;
DeviceClass_t  loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 60000*15;
// uint32_t appTxDutyCycle = 15000;
bool overTheAirActivation = true;
bool loraWanAdr = false;
bool keepNet = false;
bool isTxConfirmed = false;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

float h,t; //humidity, temperature
uint16_t b; //battery

void setup()
{
  boardInitMcu();
  Serial.begin(115200);
  PINMODE_INPUT_PULLUP(GPIO0);
  attachInterrupt(GPIO0,wmPulse,FALLING);
#if(AT_SUPPORT)
  enableAt();
#endif
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();
}

void loop()
{
  run_comms();
}

void wmPulse() 
{
  if((millis() - dblast) > dbdelay) {//debounce the input
    wm_pulse_cnt++;
    dblast = millis();
  }
}

void readRHT() 
{
  b = getBatteryVoltage(); //built-in function
  digitalWrite(Vext, LOW); //power on RHT sensor
  delay(50); //wait for sensor to stabalize
  
  Wire.begin();
  h = SHT2x.GetHumidity();
  t = SHT2x.GetTemperature();
  Serial.print("Bat: ");
  Serial.print(b/1000.);
  Serial.print(" Humidity:    ");
  Serial.print(h, 2);
  Serial.print("\tTemperature: ");
  Serial.print(t, 2);
  Serial.print("\tWM Pulses: ");
  Serial.println(wm_pulse_cnt);

  Wire.end(); //turns off i2c
  digitalWrite(Vext, HIGH); //powers off sensor
}

static void prepareTxFrame( uint8_t port )
{
  readRHT(); //temp, humidity as float

  appDataSize = 12;
  unsigned char *puc;
  puc = (unsigned char *)(&t); //temperature
  appData[0] = puc[0];
  appData[1] = puc[1];
  appData[2] = puc[2];
  appData[3] = puc[3];
  puc = (unsigned char *)(&h); //humidity
  appData[4] = puc[0];
  appData[5] = puc[1];
  appData[6] = puc[2];
  appData[7] = puc[3];

  appData[8] = (uint8_t)(b >> 8);  //battery
  appData[9] = (uint8_t)b;

  appData[10] = (uint8_t)(wm_pulse_cnt >> 8);  //water meter
  appData[11] = (uint8_t)wm_pulse_cnt;
}

void run_comms() {
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
      wm_pulse_cnt = 0; //reset water meter pulse count
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
 function decodeUplink(input) {
  var data = {};
  data.temperature = bytesToFloat(input.bytes.slice(0,4));
  data.humidity = bytesToFloat(input.bytes.slice(4,8));
  data.battery = ((input.bytes[8] << 8)+input.bytes[9])/1000.;
  data.wm_count = ((input.bytes[10] << 8) + input.bytes[11]);
  data.wm_volume = data.wm_count * 0.7480519; // 1 pulse = 1/10 cubic ft = 0.7480519 gal

  warnings = [];
  if (data.battery < 3.6) {
    warnings.push("LOW BATTERY");
  }
  return {
    data: data,
    warnings: warnings
  };
}

function normalizeUplink(input) {
  return {
    data: {
      air: {
        temperature: input.data.temperature,
        relativeHumidity: input.data.humidity,
      }
    },
    warnings: input.warnings
  }
}

function bytesToFloat(by) {
  var bits = by[3]<<24 | by[2]<<16 | by[1]<<8 | by[0];
  var sign = (bits>>>31 === 0) ? 1.0 : -1.0;
  var e = bits>>>23 & 0xff;
  var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000;
  var f = sign * m * Math.pow(2, e - 150);
  return f;
}*/