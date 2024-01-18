#include <ModbusMaster.h>
#include "pin-blynk.h"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPLDElJ7baq"
#define BLYNK_TEMPLATE_NAME "MPPT Tracer"
#define BLYNK_AUTH_TOKEN "F90jj9fVuyJCAGkapTC-GydC2SwGZO8J"


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Guest.UM";
char pass[] = "sehatselalu";
#define ARRAY_SIZE(A) (sizeof(A) / sizeof ((A)[0]))



const int defaultBaudRate = 115200;
int timerTask1, timerTask2, timerTask3, timerTask4;
float GEtoday1, GEmonth1, GEyear1, COred1, CEtoday1, CEmonth1, CEyear1, CEtotal1, GEtotal1;
float pvratvoltage1, pvratcurrent1, pvratpower1, bratvoltage1, battratChargeCurrent1, battratChargePower1;
float pvvoltage1, pvcurrent1, pvpower1, bvoltage1, battChargeCurrent1, battChargePower1;
float maxinvolt1, mininvolt1, maxbatvolt1, minbatvolt1, lvoltage1, lcurrent1, lpower1;
float bremaining1, btemp1, ambtemp1, ctemp1, powcomptemp1, battOverallCurrent1;
uint8_t result;
bool rs485DataReceived = true;
bool loadPoweredOn = true;
bool state = true;
bool debug = true;
#define MAX485_DE D1
//#define MAX485_RE_NEG D2

void preTransmission() {
  //digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission() {
  //digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

ModbusMaster node;
BlynkTimer timer;


// A list of the registries to query in order
typedef void (*RegistryList[])();

RegistryList Registries = {
  AddressRegistry_330C,
  AddressRegistry_330E,
  AddressRegistry_3310,
  AddressRegistry_3314,
  AddressRegistry_3304,
  AddressRegistry_3306,
  AddressRegistry_3308,
  AddressRegistry_330A,
  AddressRegistry_3312,
  AddressRegistry_3000,
  AddressRegistry_3100,
  AddressRegistry_3106,
  AddressRegistry_3300,
  AddressRegistry_310C,
  AddressRegistry_311A,
  AddressRegistry_331D,
  AddressRegistry_3111,
  AddressRegistry_331B,
};

uint8_t currentRegistryNumber = 0;

// function to switch to next registry
void nextRegistryNumber() {
  // better not use modulo, because after overlow it will start reading in incorrect order
  currentRegistryNumber++;
  if (currentRegistryNumber >= ARRAY_SIZE(Registries)) {
    currentRegistryNumber = 0;
  }
}

void setup()

{
  //pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  
  //digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  Serial.begin(defaultBaudRate);
  //pinMode(Erasing_button, INPUT);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // Modbus slave ID 1
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

   
  timerTask1 = timer.setInterval(1000L, executeCurrentRegistryFunction);
  timerTask2 = timer.setInterval(1000L, nextRegistryNumber);
  timerTask3 = timer.setInterval(1000L, uploadToBlynk);
}

  // upload values
  void uploadToBlynk() {
    Blynk.virtualWrite(vPIN_PV_POWER1,                   pvpower1);
    Blynk.virtualWrite(vPIN_PV_CURRENT1,                 pvcurrent1);
    Blynk.virtualWrite(vPIN_PV_VOLTAGE1,                 pvvoltage1);
    Blynk.virtualWrite(vPIN_LOAD_CURRENT1,               lcurrent1);
    Blynk.virtualWrite(vPIN_LOAD_POWER1,                 lpower1);
    Blynk.virtualWrite(vPIN_BATT_TEMP1,                  btemp1);
    Blynk.virtualWrite(vPIN_BATT_VOLTAGE1,               bvoltage1);
    Blynk.virtualWrite(vPIN_BATT_REMAIN1,                bremaining1);
    Blynk.virtualWrite(vPIN_CONTROLLER_TEMP1,            ctemp1);
    Blynk.virtualWrite(vPIN_BATTERY_CHARGE_CURRENT1,     battChargeCurrent1);
    Blynk.virtualWrite(vPIN_BATTERY_CHARGE_POWER1,       battChargePower1);
    Blynk.virtualWrite(vPIN_LOAD_ENABLED1,               state);
    Blynk.virtualWrite(vPIN_DAILY_ENERGY_GNERATED1 ,     GEtoday1);
    Blynk.virtualWrite(vPIN_GENERATED_ENERGY_MONTH1,     GEmonth1);
    Blynk.virtualWrite(vPIN_GENERATED_ENERGY_YEAR1,      GEyear1);
    Blynk.virtualWrite(vPIN_CARBON_REDUCTION1,           COred1);
    Blynk.virtualWrite(vPIN_CONSUMED_ENERGY_TODAY1,      CEtoday1);
    Blynk.virtualWrite(vPIN_CONSUMED_ENERGY_MONTH1,      CEmonth1);
    Blynk.virtualWrite(vPIN_CONSUMED_ENERGY_YEAR1,       CEyear1);
    Blynk.virtualWrite(vPIN_LOAD_VOLTAGE_1,              lvoltage1);
    
//    Blynk.virtualWrite(V70, bat11);
//    Blynk.virtualWrite(V74, bat21);
  }
  
  void executeCurrentRegistryFunction() {
    Registries[currentRegistryNumber]();
  }
  
uint8_t setOutputLoadPower(uint8_t state)
{
  Serial1.print("Writing coil 0x0002 value to: ");
  Serial1.println(state);

  delay(10);
  // Set coil at address 0x0002 (Force the load on/off)
  node.writeSingleCoil(0x0002, 1);
  result = node.writeSingleCoil(0x0002, state);  

  if (result == node.ku8MBSuccess)
  {
    node.getResponseBuffer(0x00);
    Serial1.println("Success.");
  }
  return result;
}

// callback to on/off button state changes from the Blynk app
  BLYNK_WRITE(vPIN_LOAD_ENABLED1) {
    uint8_t newState = (uint8_t)param.asInt();

    if (debug == 1) {
      Serial.print("Setting load state output coil to value: ");
      Serial.println(newState);
    }

    result = setOutputLoadPower(newState);
    //readOutputLoadState();
    result = checkLoadCoilState();

        if (debug == 1) {
     if (result == node.ku8MBSuccess) {
        Serial.println("Write & Read suceeded.");
      } else {
        Serial.println("Write & Read failed.");
      }

      Serial.print("Output Load state value: ");
      Serial.println(state);
      Serial.println();
      Serial.println("Uploading results to Blynk.");
    }
    uploadToBlynk();
  }

// reads Load Enable Override coil
  uint8_t checkLoadCoilState() {
    if (debug == 1) {
      Serial.print("Reading coil 0x0006... ");
    }
    delay(10);
    result = node.readCoils(0x0002, 1);

        if (debug == 1) {
    Serial.print("Result: ");
    Serial.println(result);
        }
        
    if (result == node.ku8MBSuccess) {
      state = node.getResponseBuffer(0x00) > 0;

      if (debug == 1) {
        Serial.print(" Value: ");
        Serial.println(state);
      }
    } else {
      if (debug == 1) {
        Serial.println("Failed to read coil 0x0002!");
      }
    }
   
    return result;
 }


// -----------------------------------------------------------------

//Energi Harian
 void AddressRegistry_330C() {
  result = node.readInputRegisters(0x330C, 2);
 
  if (result == node.ku8MBSuccess) {
  GEtoday1 = ((node.getResponseBuffer(0x01) << 16) | node.getResponseBuffer(0x00)) / 100.0f;
//   CEtoday1 = ((((long)node.getResponseBuffer(0x05) << 16 | node.getResponseBuffer(0x00)) / 100.0f));
    } else {
    rs485DataReceived = false;
  }
}

//Energi Bulanan
 void AddressRegistry_330E() {
  result = node.readInputRegisters(0x330E, 2);
  if (result == node.ku8MBSuccess)
  {
    GEmonth1 = ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}
//Energi Tahunan
void AddressRegistry_3310(){
  result = node.readInputRegisters(0x3310,2);
  if (result == node.ku8MBSuccess)
  {
    GEyear1 =  ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}

//CO Reduction
void AddressRegistry_3314(){
  result = node.readInputRegisters(0x3314,2);
  if (result == node.ku8MBSuccess)
  {
    COred1 = ((long)node.getResponseBuffer(0x01) << 16| node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}

// Consumed Energi today
void AddressRegistry_3304(){
  result = node.readInputRegisters(0x3304, 2);
  if (result == node.ku8MBSuccess)
  {
    CEtoday1 = ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}
// Consumed Energi bulanan
void AddressRegistry_3306(){
  result = node.readInputRegisters(0x3306, 2);
  if (result == node.ku8MBSuccess)
  {
    CEmonth1 = ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}

// Consumed Energi tahunan
void AddressRegistry_3308(){
  result = node.readInputRegisters(0x3308, 2);
  if (result == node.ku8MBSuccess)
  {
    CEyear1 = ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}
// Consumed Energi total
void AddressRegistry_330A(){
  result = node.readInputRegisters(0x330A, 2);
  if (result == node.ku8MBSuccess)
  {
    CEtotal1 = ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}

//Energi Total 
 void AddressRegistry_3312() {
  result = node.readInputRegisters(0x3312, 2);
  if (result == node.ku8MBSuccess)
  {
    GEtotal1 = ((long)node.getResponseBuffer(0x01) << 16 | node.getResponseBuffer(0x00)) / 100.0f;
  } else {
    rs485DataReceived = false;
  }
}
//rated
void AddressRegistry_3000(){
  result = node.readInputRegisters(0x3000, 8);
  if (result == node.ku8MBSuccess){
    pvratvoltage1 = node.getResponseBuffer(0x00) / 100.0f;
    pvratcurrent1 = node.getResponseBuffer(0x01) / 100.0f;
    pvratpower1 = (node.getResponseBuffer(0x02) | node.getResponseBuffer(0x03) << 16) / 100.0f;
    bratvoltage1 = node.getResponseBuffer(0x04) / 100.0f;
    battratChargeCurrent1 = node.getResponseBuffer(0x05) / 100.0f;
    battratChargePower1 = (node.getResponseBuffer(0x06) |node.getResponseBuffer(0x07) << 16) / 100.0f;
  }
}
 void AddressRegistry_3100() {
  result = node.readInputRegisters(0x3100, 6);
  if (result == node.ku8MBSuccess) {
      pvvoltage1 = node.getResponseBuffer(0x00) / 100.0f;
      pvcurrent1 = node.getResponseBuffer(0x01) / 100.0f;
      pvpower1 = (node.getResponseBuffer(0x02) | node.getResponseBuffer(0x03) << 16) / 100.0f;
      bvoltage1 = node.getResponseBuffer(0x04) / 100.0f;
      battChargeCurrent1 = node.getResponseBuffer(0x05) / 100.0f;

    }
  }

  void AddressRegistry_3106(){
    result = node.readInputRegisters(0x3106, 2);

    if (result == node.ku8MBSuccess) {
      battChargePower1 = (node.getResponseBuffer(0x00) | node.getResponseBuffer(0x01) << 16)  / 100.0f;
      if (debug == 1) {
        Serial.print("Battery Charge Power: ");
        Serial.println(battChargePower1);
      }
    }
  }
//maxmininput volt harian, batt harian
void AddressRegistry_3300(){
  result = node.readInputRegisters(0x3300, 4);
  if (result == node.ku8MBSuccess) {
    maxinvolt1 = node.getResponseBuffer(0x00) / 100.0f;
    mininvolt1 = node.getResponseBuffer(0x01) / 100.0f;
    maxbatvolt1 = node.getResponseBuffer(0x02) / 100.0f;
    minbatvolt1 = node.getResponseBuffer(0x03) / 100.0f;
  }
}

  void AddressRegistry_310C(){
    result = node.readInputRegisters(0x310C, 4);

      if (result == node.ku8MBSuccess) {
      lvoltage1 = node.getResponseBuffer(0x00) / 100.0f;
      lcurrent1 = node.getResponseBuffer(0x01) / 100.0f;
      lpower1 = (node.getResponseBuffer(0x02) | node.getResponseBuffer(0x03) << 16) / 100.0f;
    } else {
      rs485DataReceived = false;
    }    
  }
 

  void AddressRegistry_311A() {
    result = node.readInputRegisters(0x311A, 2);
   
    if (result == node.ku8MBSuccess) {    
      bremaining1 = node.getResponseBuffer(0x00) ;
    } else {
      rs485DataReceived = false;
    }
  }

void AddressRegistry_331D() {
    result = node.readInputRegisters(0x331D, 2);
  
    if (result == node.ku8MBSuccess) {    
      btemp1 = node.getResponseBuffer(0x00) / 100.0f;
      ambtemp1 = node.getResponseBuffer (0x01) / 100.0f;
  }
 }

void AddressRegistry_3111() {
    result = node.readInputRegisters(0x3111, 2);
  
    if (result == node.ku8MBSuccess) {    
      ctemp1 = node.getResponseBuffer(0x00) / 100.0f;
      powcomptemp1 = node.getResponseBuffer (0x01) / 100.0f;
  }
 }

   void AddressRegistry_331B() {
    result = node.readInputRegisters(0x331B, 2);

    if (result == node.ku8MBSuccess) {
      battOverallCurrent1 = (node.getResponseBuffer(0x00) | node.getResponseBuffer(0x01) << 16) / 100.0f;
    }
  }


void loop()
{

    Blynk.run();
    timer.run();
}
