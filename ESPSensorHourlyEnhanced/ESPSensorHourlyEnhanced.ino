
ADC_MODE(ADC_VCC); //vcc read-mode

#include <ESP8266WiFi.h>
#include <credentials.h>;
extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}
#define RTCMEMORYSTART 66
#define RTCMEMORYLEN 125
#define VCC_ADJ  1.0

#define COLLECT 17
#define SEND 66
#define SLEEPTIME 1000000

#define SPARKFUN_BATTERY 1



typedef struct {
  int magicNumber;
  int valueCounter;
} rtcManagementStruc;

rtcManagementStruc rtcManagement;

typedef struct {
  float battery;
  int other;
} rtcStore;

rtcStore rtcValues;

int i;
int buckets;
unsigned long startTime;

WiFiClient client;


void setup() {
  Serial.begin(115200);
  Serial.println();
  //  Serial.println("Start");
  buckets = (sizeof(rtcValues) / 4);
  if (buckets == 0) buckets = 1;
  // Serial.print("Buckets ");
  //  Serial.println(buckets);
  system_rtc_mem_read(64, &rtcManagement, 8);
  // Serial.print("Magic Number ");
  //  Serial.println(rtcManagement.magicNumber);
  Serial.print("Counter ");
  Serial.println(rtcManagement.valueCounter);

  // initialize System after first start
  if (rtcManagement.magicNumber != COLLECT && rtcManagement.magicNumber != SEND ) {
    rtcManagement.magicNumber = COLLECT;
    rtcManagement.valueCounter = 0;
    system_rtc_mem_write(64, &rtcManagement, 8);
    Serial.println("Initial values set");
    ESP.deepSleep(10, WAKE_RF_DISABLED);
  }
  if (rtcManagement.magicNumber == COLLECT) {   // Read sensor and store
    if (rtcManagement.valueCounter <= RTCMEMORYLEN / buckets) {
      Serial.println("Sensor reads");

      rtcValues.battery = 3.0+(float)random(100)/5000;  // ESP.getVcc() * VCC_ADJ ;
      rtcValues.other = rtcManagement.valueCounter;

      int rtcPos = RTCMEMORYSTART + rtcManagement.valueCounter * buckets;
      system_rtc_mem_write(rtcPos, &rtcValues, buckets * 4);
      system_rtc_mem_write(rtcPos, &rtcValues, 4);
      system_rtc_mem_write(64, &rtcManagement, 4);

      Serial.print("Counter : ");
      Serial.print(rtcManagement.valueCounter);
      Serial.print(" Position: ");
      Serial.print(rtcPos);
      Serial.print(", battery: ");
      Serial.print(rtcValues.battery);
      rtcManagement.valueCounter++;
      system_rtc_mem_write(64, &rtcManagement, 8);
      Serial.println("before sleep W/O RF");
      ESP.deepSleep(SLEEPTIME, WAKE_NO_RFCAL);
    }
    else {
      rtcManagement.magicNumber = SEND;
      system_rtc_mem_write(64, &rtcManagement, 8);
      Serial.println("before sleep w RF");
      ESP.deepSleep(10, WAKE_RFCAL);
    }
  }
  else {  // Send to Cloud
    startTime = millis();
    Serial.print("Sending ");
    Serial.println(startTime);
    for (i = 0; i <= RTCMEMORYLEN / buckets; i++) {
      int rtcPos = RTCMEMORYSTART + i * buckets;
      system_rtc_mem_read(rtcPos, &rtcValues, sizeof(rtcValues));
      Serial.print("i: ");
      Serial.print(i);
      Serial.print(" Position ");
      Serial.print(rtcPos);
      Serial.print(", battery: ");
      Serial.print(rtcValues.battery);
      Serial.print(", other: ");
      Serial.println(rtcValues.other);
      sendSparkfun(SPARKFUN_BATTERY, rtcValues.other, rtcValues.battery);
      yield();
    }
    rtcManagement.magicNumber = COLLECT;
    rtcManagement.valueCounter = 0;
    system_rtc_mem_write(64, &rtcManagement, 8);
    Serial.print("Writing to Cloud done ");
    Serial.println(millis() - startTime);
    Serial.println("before sleep W/O RF ");
    ESP.deepSleep(SLEEPTIME, WAKE_NO_RFCAL);
  }
}


void loop() {}

