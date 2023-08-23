#include <RadioLib.h>
#include <IWatchdog.h>
#include <EEPROM.h>

// board STM32G030C8T6
//#define LED        PC13
//#define LR_RST     PA3
//#define NSS        PA4
//#define DIO0       PB1
//#define DIO1       PA15
//#define DIO2       PB3
//#define LORA_PWR   PA11   // active LOW

#define LED        PC13
#define LR_RST     PB14
#define NSS        PA4
#define DIO0       PB1
#define DIO1       PA15
#define DIO2       PB3
#define LORA_PWR   PA11   // active LOW

#define RLY1       PB5
#define RLY2       PB4
#define RLY3       PC15
#define RLY4       PC14
#define RLY5       PB9
#define RLY6       PB8
#define RLY7       PB7
#define RLY8       PB6
#define INP        PB10

#define TYPEID     2     // Relay Out 16 bit

SX1276 radio = new Module(NSS, DIO0, LR_RST, DIO1);

#define SERIAL_BAUD   115200
#define CTRBYTE    43

typedef struct {
  uint8_t       typeId;
  uint16_t      nodeId; //store this nodeId
  uint8_t       cmd; //type parameter
  uint32_t      vParams;   //temperature maybe?
} Payload;
Payload theData;

typedef struct {
  uint8_t configId;
  uint8_t typeId;
  uint16_t nodeId;
  uint8_t mode;
  uint8_t periode;
} Params;
Params  id;

uint32_t updateRate = 120000;
long lastUpdate;
uint16_t statusRelay = 0;
byte statusInput = 1;
bool sendSta = false;
bool sendPara = false;

// save transmission states between loops
int transmissionState =  ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

void blinkLed(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED, LOW);
    delay(200);
    digitalWrite(LED, HIGH);
    delay(200);
  }
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  //  if (!enableInterrupt) {
  //    return;
  //  }

  // we got a packet, set the flag
  operationDone = true;
}

//Function to calculate CRC over an array 'ar' up to element 's'
byte calculateCRC(byte ar[], byte s) {
  byte rtn = 0;;
  for (byte i = 0; i < s; i++) {
    rtn ^= ar[i];
  }
  return rtn;
}

void changeRelay(uint16_t vParams) {

  for (int i = 0; i < 8; i++) {
    switch (i) {
      case 0:
        // statements
        digitalWrite(RLY1, !bitRead(vParams, i));
        delay(100);
        break;
      case 1:
        // statements
        digitalWrite(RLY2, !bitRead(vParams, i));
        delay(100);
        break;
      case 2:
        // statements
        digitalWrite(RLY3, !bitRead(vParams, i));
        delay(100);
        break;
      case 3:
        // statements
        digitalWrite(RLY4, !bitRead(vParams, i));
        delay(100);
        break;
      case 4:
        // statements
        digitalWrite(RLY5, !bitRead(vParams, i));
        delay(100);
        break;
      case 5:
        // statements
        digitalWrite(RLY6, !bitRead(vParams, i));
        delay(100);
        break;
      case 6:
        // statements
        digitalWrite(RLY7, !bitRead(vParams, i));
        delay(100);
        break;
      case 7:
        // statements
        digitalWrite(RLY8, !bitRead(vParams, i));
        delay(100);
        break;
      default:
        // statements
        break;
    }
  }
  statusRelay = vParams & 0x00FF;
  statusRelay = statusRelay + statusInput * 256;
  delay(2000);
  sendStatus();
}

void sendStatus() {
  theData.typeId = id.typeId;
  theData.nodeId = id.nodeId;
  theData.cmd = 111;
  theData.vParams = statusRelay;
  Serial.print(F("theData: "));
  Serial.print(theData.typeId);
  Serial.print(F(","));
  Serial.print(theData.nodeId);
  Serial.print(F(","));
  Serial.print(theData.cmd);
  Serial.print(F(","));
  Serial.println(theData.vParams);
  byte len = sizeof(theData);
  byte byteArray[len + 1];
  memcpy(byteArray, (const void*)(&theData), len);
  byteArray[len] = calculateCRC(byteArray, len);
  transmissionState = radio.startTransmit(byteArray, len + 1);
  transmitFlag = true;
  blinkLed(2);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pinMode(LORA_PWR, OUTPUT);
  digitalWrite(LORA_PWR, LOW);
  delay(10);

  pinMode(RLY1, OUTPUT);
  pinMode(RLY2, OUTPUT);
  pinMode(RLY3, OUTPUT);
  pinMode(RLY4, OUTPUT);
  pinMode(RLY5, OUTPUT);
  pinMode(RLY6, OUTPUT);
  pinMode(RLY7, OUTPUT);
  pinMode(RLY8, OUTPUT);
  pinMode(INP, INPUT);
  digitalWrite(RLY1, HIGH);
  digitalWrite(RLY2, HIGH);
  digitalWrite(RLY3, HIGH);
  digitalWrite(RLY4, HIGH);
  digitalWrite(RLY5, HIGH);
  digitalWrite(RLY6, HIGH);
  digitalWrite(RLY7, HIGH);
  digitalWrite(RLY8, HIGH);


  if (IWatchdog.isReset(true)) {
    // LED blinks to indicate reset
    blinkLed(10);
  }

  EEPROM.get(0, id);
  if (id.configId == CTRBYTE) {
    updateRate = 1000 * id.periode;
  }
  else {
    id.configId = CTRBYTE;
    id.typeId = TYPEID;
    id.nodeId = 2;
    id.mode = 1;
    updateRate = 120 * 1000;
    id.periode = 120;
    EEPROM.put(0, id);
  }

  int state = radio.begin(920.0, 125.0, 9, 7,  SX127X_SYNC_WORD, 17, 8, 0);
  if (state ==  ERR_NONE) {
    //Serial.println(F("success!"));
  } else {
    //Serial.print(F("failed, code "));
    //Serial.println(state);
    blinkLed(20);
    while (true);
  }

  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag);

  state = radio.startReceive();
  if (state ==  ERR_NONE) {
    // Serial.println(F("success!"));
  } else {
    //Serial.print(F("failed, code "));
    //Serial.println(state);
    while (true);
  }

  analogReadResolution(12);

  // Init the watchdog timer with 10 seconds timeout
  IWatchdog.begin(10000000);
  lastUpdate = millis();
  blinkLed(3);

}

void loop() {
  // put your main code here, to run repeatedly:

  IWatchdog.reload();

  if (operationDone) {
    // disable the interrupt service routine while
    // processing the data
    //enableInterrupt = false;

    // reset flag
    operationDone = false;

    if (transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState ==  ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {

      byte len = sizeof(theData);
      byte byteArray[len + 1];
      int state = radio.readData(byteArray, len + 1);
      if (state ==  ERR_NONE) {
        theData = *(Payload*)byteArray;
        if (byteArray[len] == calculateCRC(byteArray, len)) {
          if ((theData.typeId == id.typeId) && (theData.nodeId == id.nodeId)) {
            int cmd = theData.cmd;
            int vParams = theData.vParams;
            switch (cmd) {
              case 0:
                // request status
                Serial.println("request data dieksekusi");
                delay(2000);
                sendSta = true;
                break;
              case 1:
                // change relay
                Serial.println("request changeRelay dieksekusi");
                changeRelay(vParams);
                sendPara = false;
                break;
              case 2:
                // ubah nodeId
                id.nodeId = vParams;
                EEPROM.put(0, id);
                break;
              case 3:
                // ubah mode
                id.mode = vParams;
                EEPROM.put(0, id);
                break;
              case 4:
                // ubah periode
                id.periode = vParams;
                updateRate = 1000 * id.periode;
                EEPROM.put(0, id);
                break;
              default:
                //
                break;
            }
          }
          blinkLed(2);
        }
      }
      radio.startReceive();
      transmitFlag = false;
    }
  }

  if (id.mode == 1) {
    if ((millis() - lastUpdate) > updateRate) {
      lastUpdate = millis();
      sendStatus();
    }
  }

  int ab = digitalRead(INP);
  if (statusInput != ab) {
    statusInput = ab;
    if (ab) {
      bitSet(statusRelay, 8);
    } else {
      bitClear(statusRelay, 8);
    }
    statusRelay = statusRelay & 0x01FF;
    sendSta = true;
  }

  if (sendSta) {
    sendSta = false;
    sendStatus();
  }

}
