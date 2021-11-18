#include <OneWire.h>
#include <DallasTemperature.h>

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <RTCZero.h>
#include <SerialFlash.h>

#define EUI64_CHIP_ADDRESS 0x50
#define EUI64_MAC_ADDRESS 0xF8
#define EUI64_MAC_LENGTH 0x08
#define MAX_DATA_SIZE 2
// Data wire is plugged into port 2 on the Arduino
#define TEMP_PIN 8


void onEvent(ev_t);
void do_send(osjob_t*);
void setDevEui(unsigned char*);
void alarmMatch();
void printHex2(unsigned);
float do_sensor();
String split(String, char, int);

OneWire oneWire(TEMP_PIN);
DallasTemperature t(&oneWire);

RTCZero rtc;

/*
// End-device Address (u4_t) in uint32_t format. 
// Note: The value must start with 0x (current version of TTN Console does not provide this).
#define ABP_DEVADDR 0x01CFD606

// Network Session Key (u1_t[16]) in msb format
#define ABP_NWKSKEY 0x4C, 0x62, 0x1F, 0x85, 0x3D, 0xE8, 0x33, 0xA0, 0xE2, 0xDD, 0x6D, 0x30, 0x3D, 0xCA, 0x95, 0x0C

// Application Session K (u1_t[16]) in msb format
#define ABP_APPSKEY 0x0A, 0xED, 0xD5, 0x40, 0x0F, 0x21, 0xCF, 0xF9, 0x8B, 0x44, 0xCC, 0xC9, 0xF4, 0x13, 0x8F, 0xA2
*/

// LoRaWAN NwkSKey, network session key
// This should be in big-endian (aka msb).
static const PROGMEM u1_t NWKSKEY[16] = { 0xA0, 0xA8, 0x37, 0x54, 0x38, 0x78, 0x6D, 0xAA, 0x05, 0x47, 0x97, 0x13, 0xAB, 0x8F, 0xE2, 0x34 };

// LoRaWAN AppSKey, application session key
// This should also be in big-endian (aka msb).
static const u1_t PROGMEM APPSKEY[16] = { 0x58, 0xBE, 0xD3, 0x74, 0x52, 0x4C, 0x27, 0x14, 0x29, 0xDE, 0x83, 0x1C, 0x75, 0x32, 0x13, 0x42 };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
// The library converts the address to network byte order as needed, so this should be in big-endian (aka msb) too.
static const u4_t DEVADDR = 0x00C2706E ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmic/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }


static uint8_t data[MAX_DATA_SIZE];
static osjob_t sendjob;
const unsigned TX_INTERVAL = 60*1;

const lmic_pinmap lmic_pins = {
  .nss = 5,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 3,
  .dio = {2, 6, LMIC_UNUSED_PIN},
};

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        SerialUSB.print('0');
    SerialUSB.print(v, HEX);
}

void onEvent (ev_t ev) {
  SerialUSB.print(os_getTime());
  SerialUSB.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      SerialUSB.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      SerialUSB.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      SerialUSB.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      SerialUSB.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      SerialUSB.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      SerialUSB.println(F("EV_JOINED"));
      {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        SerialUSB.print("netid: ");
        SerialUSB.println(netid, DEC);
        SerialUSB.print("devaddr: ");
        SerialUSB.println(devaddr, HEX);
        SerialUSB.print("AppSKey: ");
        for (size_t i=0; i<sizeof(artKey); ++i) \
        {
          if (i != 0)
            SerialUSB.print("-");
          printHex2(artKey[i]);
        }
        SerialUSB.println("");
        SerialUSB.print("NwkSKey: ");
        for (size_t i=0; i<sizeof(nwkKey); ++i) 
        {
          if (i != 0)
            SerialUSB.print("-");
          printHex2(nwkKey[i]);
        }
        SerialUSB.println();
      }
      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      break;
    case EV_JOIN_FAILED:
      SerialUSB.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      SerialUSB.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      SerialUSB.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        SerialUSB.println(F("Received ack"));
      if (LMIC.dataLen) {
        SerialUSB.println(F("Received "));
        SerialUSB.println(LMIC.dataLen);
        SerialUSB.println(F(" bytes of payload"));
      }
      // Ensure all debugging messages are sent before sleep
      SerialUSB.flush();
      
      //Run without sleeping!
      // os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);

      // Sleep for a period of TX_INTERVAL using single shot alarm
      rtc.setAlarmEpoch(rtc.getEpoch() + TX_INTERVAL);
      rtc.enableAlarm(rtc.MATCH_YYMMDDHHMMSS);
      rtc.attachInterrupt(alarmMatch);
      // USB port consumes extra current
      USBDevice.detach();
      // Enter sleep mode
      rtc.standbyMode();
      // Reinitialize USB for debugging
      USBDevice.init();
      USBDevice.attach();
      // Schedule next transmission to be immediately after this
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(1), do_send);
      break;
    case EV_LOST_TSYNC:
      SerialUSB.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      SerialUSB.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      SerialUSB.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      SerialUSB.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      SerialUSB.println(F("EV_LINK_ALIVE"));
      break;
    case EV_TXSTART:
      SerialUSB.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      SerialUSB.println(F("EV_TXCANCELED"));
      break;
    case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
    case EV_JOIN_TXCOMPLETE:
      SerialUSB.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;
    default:
      SerialUSB.print(F("Unknown event: "));
      SerialUSB.println((unsigned) ev);
      break;
  }
}

void do_send(osjob_t* j) {
  float tempC = do_sensor();
  int temp_int = (tempC+50) * 100;    
  data[0] = temp_int >> 8;
  data[1] = temp_int;
  digitalWrite(LED_BUILTIN, HIGH);
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    SerialUSB.println(F("OP_TXRXPEND, not sending"));
  }
  else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, data, sizeof(data), 0);
    SerialUSB.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
  digitalWrite(LED_BUILTIN, LOW);
}


void setDevEui(unsigned char* buf)
{
  Wire.begin();
  Wire.beginTransmission(EUI64_CHIP_ADDRESS);
  Wire.write(EUI64_MAC_ADDRESS);
  Wire.endTransmission();
  Wire.requestFrom(EUI64_CHIP_ADDRESS, EUI64_MAC_LENGTH);

  // Format needs to be little endian (LSB...MSB)
  while (Wire.available())
  {
    *buf-- = Wire.read();
  }
}

void setup() {
  int count;
  unsigned char pinNumber;
  // ***** Put unused pins into known state *****
  pinMode(0, INPUT_PULLUP);  
  pinMode(1, INPUT_PULLUP);
  for (pinNumber = 7; pinNumber <= 22; pinNumber++)  // D7-D13, A0(D14)-A5(D19), SDA(D20), SCL(D21), MISO(D22)
  {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  pinMode(25, INPUT_PULLUP);  // RX_LED (D25) & TX_LED (D26) (both LED not mounted on Mini Ultra Pro)
  pinMode(26, INPUT_PULLUP);
  pinMode(30, INPUT_PULLUP);  // D30 (RX) & D31 (TX) of Serial
  pinMode(31, INPUT_PULLUP);
  for (pinNumber = 34; pinNumber <= 38; pinNumber++)  // D34-D38 (EBDG Interface)
  {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  // ***** End of unused pins state initialization *****

  pinMode(LED_BUILTIN, OUTPUT);
  // setDevEui(&DEVEUI[EUI64_MAC_LENGTH - 1]);
  while (!SerialUSB && millis() < 10000);
  SerialUSB.begin(115200);
  SerialUSB.println(F("Starting"));
  // SerialUSB.print(F("DEVEUI: "));  

  // for (count = EUI64_MAC_LENGTH; count > 0; count--) {
  //   SerialUSB.print("0x");
  //   if (DEVEUI[count - 1] <= 0x0F) SerialUSB.print("0");
  //   SerialUSB.print(DEVEUI[count - 1], HEX);
  //   SerialUSB.print(" ");
  // }
  // SerialUSB.println();

  SerialFlash.begin(4);  // Initialize serial flash
  SerialFlash.sleep(); // Put serial flash in sleep
  rtc.begin();  // Initialize RTC
  rtc.setEpoch(0);  // Use RTC as a second timer instead of calendar
  t.begin(); //setup temperature sensor

  os_init();  // LMIC init
  LMIC_reset();  // Reset the MAC state. Session and pending data transfers will be discarded.
  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  #ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);
  #else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x13, DEVADDR, NWKSKEY, APPSKEY);
  #endif  
  LMIC_selectSubBand(1);
  LMIC.dn2Dr = DR_SF9;
  LMIC_setDrTxpow(DR_SF7,14);

  // LMIC_setLinkCheckMode(0);
  do_send(&sendjob);  // Start job (sending automatically starts OTAA too)
}

void loop() {
  os_runloop_once();
}

void alarmMatch(){ }


float do_sensor() {
  t.requestTemperatures(); // Send the command to get temperatures

  float tempC = t.getTempCByIndex(0);

  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    SerialUSB.println(tempC);
  } 
  else
  {
    SerialUSB.println("Error: Could not read temperature data");
  }
  return tempC;
}
