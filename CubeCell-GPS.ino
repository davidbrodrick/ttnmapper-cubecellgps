#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "GPS_Air530.h"
#include <Wire.h>  
#include "cubecell_SSD1306Wire.h"
#include <LoraMessage.h>

//Don't use ADR for mobile devices: use this fixed data rate (region dependent?)
uint8_t txDataRate=2;

// How often to transmit positions (ms)
uint32_t appTxDutyCycle = 15000;

//Don't request acknowlegements
bool isTxConfirmed = false;

/* OTAA para*/
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
/* ABP para*/
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t devAddr =  0x00000000;

bool joined=false;
extern SSD1306Wire  display;

uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t  loraWanClass = LORAWAN_CLASS;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = false; //Don't use ADR for mobile devices
bool keepNet = LORAWAN_NET_RESERVE;
uint8_t appPort = 1;
uint8_t confirmedNbTrials = 4;

int fracPart(double val, int n)
{
  return (int)((val - (int)(val))*pow(10,n));
}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

static void prepareTxFrame( uint8_t port )
{
  LoraMessage message;
  message.addUint8(Air530.hdop.hdop()*10)
  .addLatLng(Air530.location.lat(), Air530.location.lng())
  .addUint16(Air530.altitude.meters());

  appDataSize = message.getLength();
  if (appDataSize>0) {
    memcpy(appData, message.getBytes(), appDataSize);
  }
}

void setup() {
  boardInitMcu();
  Serial.begin(115200);

  //Initialise LoRa
  if (loraWanRegion==LORAMAC_REGION_AU915) {
    //TTN uses sub-band 2
    userChannelsMask[0]=0xFF00;
  }
  LoRaWAN.setDataRateForNoADR(txDataRate);
  LoRaWAN.displayMcuInit();
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();

  //Initialise GPS
  Air530.begin();    
}

void loop()
{
  if (joined) {    
    char str[30];
    display.clear();
    display.setFont(ArialMT_Plain_10);
    int index = sprintf(str,"%02d-%02d-%02d",Air530.date.year(),Air530.date.day(),Air530.date.month());
    str[index] = 0;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, str);
  
    index = sprintf(str,"%02d:%02d:%02d",Air530.time.hour(),Air530.time.minute(),Air530.time.second(),Air530.time.centisecond());
    str[index] = 0;
    display.drawString(60, 0, str);

    if( Air530.location.age() < 5000 )  {
      display.drawString(120, 0, "A");
    } else {
      display.drawString(120, 0, "V");
    }
  
    index = sprintf(str,"alt: %d.%d",(int)Air530.altitude.meters(),fracPart(Air530.altitude.meters(),2));
    str[index] = 0;
    display.drawString(0, 16, str);
   
    index = sprintf(str,"hdop: %d.%d",(int)Air530.hdop.hdop(),fracPart(Air530.hdop.hdop(),2));
    str[index] = 0;
    display.drawString(0, 32, str); 
 
    index = sprintf(str,"lat :  %d.%d",(int)Air530.location.lat(),fracPart(Air530.location.lat(),4));
    str[index] = 0;
    display.drawString(60, 16, str);   
  
    index = sprintf(str,"lon:%d.%d",(int)Air530.location.lng(),fracPart(Air530.location.lng(),4));
    str[index] = 0;
    display.drawString(60, 32, str);

    index = sprintf(str,"speed: %d.%d km/h",(int)Air530.speed.kmph(),fracPart(Air530.speed.kmph(),3));
    str[index] = 0;
    display.drawString(0, 48, str);
    display.display();
   
    uint32_t starttime = millis();
    while( (millis()-starttime) < 1000 ) {
      while (Air530.available()) {
        Air530.encode(Air530.read());
      }
    }    
  }
    
  switch( deviceState ) {
    case DEVICE_STATE_INIT: {
      printDevParam();
      LoRaWAN.init(loraWanClass,loraWanRegion);
      deviceState = DEVICE_STATE_JOIN;
      break;
    }
    case DEVICE_STATE_JOIN: {
      LoRaWAN.displayJoining();
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND: {
      joined=true;
      if( Air530.location.isValid() && Air530.altitude.isValid() && Air530.location.age() < 5000) {      
        prepareTxFrame( appPort );
        LoRaWAN.send();      
      }
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE: {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle;// + randr( 0, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP: {
      LoRaWAN.displayAck();
      LoRaWAN.sleep();
      break;
    }
    default: {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}
