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


void printHex2(unsigned);
void alarmMatch();
void do_send(osjob_t*);
void listenForWeather();
float getBatV();
String split(String, char, int);
void onEvent(ev_t);
void setDevEui(unsigned char*);

static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

//rs-01: 0004A30B001A513E  //this one is outside
//rs-02: 0004A30B001AD9E7  //this one is inside
u1_t DEVEUI[EUI64_MAC_LENGTH];
void os_getDevEui (u1_t* buf) {
  memcpy(buf, DEVEUI, EUI64_MAC_LENGTH);
}
//90850BB41767BE4AB88B0DB707C1FA57 //this is for the weather app
static const u1_t PROGMEM APPKEY[16] = { 0x90, 0x85, 0x0B, 0xB4, 0x17, 0x67, 0xBE, 0x4A, 0xB8, 0x8B, 0x0D, 0xB7, 0x07, 0xC1, 0xFA, 0x57 }; //MSB
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}
static uint8_t data[MAX_DATA_SIZE];
static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;

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

String input = "";
String meas[MEAS_ARRAY_SIZE];
int measINT[MEAS_ARRAY_SIZE];
float measFLOAT[MEAS_ARRAY_SIZE];
float getBatV();

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
  Serial.begin(9600);
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

void alarmMatch()
{

}

void do_send(osjob_t* j)
{
  SerialUSB.println("Checking the Weather");
  listenForWeather();
  SerialUSB.println("data Collected, Attempting to Send");
  digitalWrite(LED_BUILTIN, HIGH);
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    SerialUSB.println(F("OP_TXRXPEND, not sending"));
  }
  else
  {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, data, sizeof(data), 0);
    SerialUSB.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
  digitalWrite(LED_BUILTIN, LOW);
}

void listenForWeather() {
  Serial.println("0R0"); //request measurements
  // SerialUSB.println(F("0R0"));
  delay(40);
    //Parse Measurements
  if(Serial.available()) 
  {
    input = Serial.readStringUntil('\n');
    input = "0R0,Dn=005D,Dm=005D,Dx=005D,Sn=2.8M,Sm=2.8M,Sx=2.8M,Ta=23.0C,Tp=24.0C,Ua=30.0P,Pa=1028.2H,Rc=0.00M,Rd=10s,Ri=1.00M,Hc=0.00M,Hd=10s,Hi=1.00M,Rp=0.00M,Hp=0.00M,Th=23.6C,Vh=3.3V,Vs=5.0V,Vr=2.5V";
    for(int i = 0; i < MEAS_ARRAY_SIZE; i++) 
      meas[i] = split(input, ',',i);
    if(meas[0]=="0R0") { //check for correct return
    //trims non-number values off each return
      for(int i = 0; i < MEAS_ARRAY_SIZE; i++) {
        meas[i].remove(0,3); //remove first three values
        meas[i].remove(meas[i].length()-1,1); //remove the last value
      }
      //converts values to floats
      for(int i = 0; i < MEAS_ARRAY_SIZE; i++) {
        measFLOAT[i] = meas[i].toFloat();
        // SerialUSB.println(measFLOAT[i]);
      }
      measFLOAT[23] = getBatV()*10;
      measFLOAT[7] += 50; //air temp
      measFLOAT[8] += 50; //air temp internal
      measFLOAT[19] += 50;  //heating temp
      //converts floats into char pairs
      for(int i = 0; i < MEAS_ARRAY_SIZE; i++) {
        measINT[i] = measFLOAT[i] * 10; //upscale data and convert floats to ints
        data[i*2] = measINT[i] >> 8;
        data[(i*2)+1] = measINT[i];
      }
    }
  }
}

float getBatV() {
  unsigned char counter;
  float batteryVoltage;
  int adcReading;
  adcReading = analogRead(A5);
  // Discard inaccurate 1st reading
  adcReading = 0;
  // Perform averaging
  for (counter = 10; counter > 0; counter--)
  {
    adcReading += analogRead(A5);
  }
  adcReading = adcReading / 10;
  // Convert to volts
  
  batteryVoltage = adcReading * (4.3 / 1023.0);
  // voltage = batteryVoltage * 100;
  // data[0] = voltage >> 8;
  // data[1] = voltage;
  return batteryVoltage;
}

String split(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void onEvent (ev_t ev)
{
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
      break;
    case EV_TXCOMPLETE:
        SerialUSB.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
        if (LMIC.txrxFlags & TXRX_ACK)
            SerialUSB.println(F("Received ack"));
        if (LMIC.dataLen) 
        {
            SerialUSB.println(F("Received "));
            SerialUSB.println(LMIC.dataLen);
            SerialUSB.println(F(" bytes of payload"));
        }
        // Ensure all debugging messages are sent before sleep
        SerialUSB.flush();

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