/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/


/**
 * Copy the APPKEY and DEVEUI Straight across as MSB!!! 
 * The DEVEUI is switched as it's read from the chip 
 * 
 */
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <RTCZero.h>
#include <SerialFlash.h>


#define EUI64_CHIP_ADDRESS 0x50
#define EUI64_MAC_ADDRESS 0xF8
#define EUI64_MAC_LENGTH 0x08
#define MAX_DATA_SIZE 46
#define MEAS_ARRAY_SIZE 24

RTCZero rtc;

static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

//rs-01: 0004A30B001A513E 
//rs-02: 0004A30B001AD9E7 
u1_t DEVEUI[EUI64_MAC_LENGTH];
void os_getDevEui (u1_t* buf) {
  memcpy(buf, DEVEUI, EUI64_MAC_LENGTH);
}
//90850BB41767BE4AB88B0DB707C1FA57
static const u1_t PROGMEM APPKEY[16] = { 0xD6, 0x4C, 0x29, 0xC0, 0x71, 0x0A, 0xE3, 0xFC, 0x8C, 0x1D, 0x4E, 0x2A, 0x90, 0x67, 0xE4, 0xCB }; //MSB  //Tester AppKey
// static const u1_t PROGMEM APPKEY[16] = { 0x90, 0x85, 0x0B, 0xB4, 0x17, 0x67, 0xBE, 0x4A, 0xB8, 0x8B, 0x0D, 0xB7, 0x07, 0xC1, 0xFA, 0x57 }; //MSB //weather AppKey
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

static uint8_t payload[5];
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 30;

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
    switch(ev) {
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
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  SerialUSB.print("-");
                printHex2(artKey[i]);
              }
              SerialUSB.println("");
              SerialUSB.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              SerialUSB.print("-");
                      printHex2(nwkKey[i]);
              }
              SerialUSB.println();
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	    // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     SerialUSB.println(F("EV_RFU1"));
        ||     break;
        */
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
              SerialUSB.print(F("Received "));
              SerialUSB.print(LMIC.dataLen);
              SerialUSB.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
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
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    SerialUSB.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
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

void do_send(osjob_t* j){
        float temperature = random(-20, 40);
        temperature = temperature / 100;
        float rHumidity = random(0, 100);
        rHumidity = rHumidity / 100;
        uint16_t payloadTemp = LMIC_f2sflt16(temperature);
        // int -> bytes
        byte tempLow = lowByte(payloadTemp);
        byte tempHigh = highByte(payloadTemp);
        // place the bytes into the payload
        payload[0] = tempLow;
        payload[1] = tempHigh;
        // float -> int
        uint16_t payloadHumid = LMIC_f2sflt16(rHumidity);
        // int -> bytes
        byte humidLow = lowByte(payloadHumid);
        byte humidHigh = highByte(payloadHumid);
        payload[2] = humidLow;
        payload[3] = humidHigh;

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        SerialUSB.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, payload, sizeof(payload)-1, 0);
        SerialUSB.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
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
  // pinMode(30, INPUT_PULLUP);  // D30 (RX) & D31 (TX) of Serial
  // pinMode(31, INPUT_PULLUP);
  for (pinNumber = 34; pinNumber <= 38; pinNumber++)  // D34-D38 (EBDG Interface)
  {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  // ***** End of unused pins state initialization *****

  pinMode(LED_BUILTIN, OUTPUT);
  setDevEui(&DEVEUI[EUI64_MAC_LENGTH - 1]);
  while (!SerialUSB && millis() < 10000);
  SerialUSB.begin(115200);
  SerialUSB.println(F("Starting"));
  SerialUSB.print(F("DEVEUI: "));  
  SerialUSB.begin(9600);
  SerialUSB.println("Serial0 started");
  for (count = EUI64_MAC_LENGTH; count > 0; count--) {
    SerialUSB.print("0x");
    if (DEVEUI[count - 1] <= 0x0F) SerialUSB.print("0");
    SerialUSB.print(DEVEUI[count - 1], HEX);
    SerialUSB.print(" ");
  }
  SerialUSB.println();
  SerialFlash.begin(4);  // Initialize serial flash
  SerialFlash.sleep(); // Put serial flash in sleep
  rtc.begin();  // Initialize RTC
  rtc.setEpoch(0);  // Use RTC as a second timer instead of calendar
  os_init();  // LMIC init
  LMIC_reset();  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  do_send(&sendjob);  // Start job (sending automatically starts OTAA too)
}

void loop() {
    os_runloop_once();
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