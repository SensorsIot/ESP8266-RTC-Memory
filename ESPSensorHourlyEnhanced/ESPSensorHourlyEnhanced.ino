
ADC_MODE(ADC_VCC); //vcc read-mode

#include <ESP8266WiFi.h>
#include <credentials.h>;
extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}
#define RTCMEMORYSTART 66
#define RTCMEMORYLEN 125
#define VCC_ADJ  1.0   // measure with your voltmeter and calculate that the number mesured from ESP is correct

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
  Serial.println();
  Serial.print("Booted ");


  rst_info *rsti;
  rsti = ESP.getResetInfoPtr();

  switch (rsti->reason) {

    case 5:
      Serial.println(" from RTC-RESET (ResetInfo.reason = 5)");
      break;
    case 6:
      Serial.println(" from POWER-UP (ResetInfo.reason = 6)");
      rtcManagement.magicNumber = COLLECT;
      rtcManagement.valueCounter = 0;
      break;
  }

  system_get_rst_info();

  buckets = (sizeof(rtcValues) / 4);
  if (buckets == 0) buckets = 1;
  // Serial.print("Buckets ");
  //  Serial.println(buckets);
  system_rtc_mem_read(64, &rtcManagement, 8);
  // Serial.print("Magic Number ");
  //  Serial.println(rtcManagement.magicNumber);

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

      rtcValues.battery = ESP.getVcc() * VCC_ADJ ;
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
    else {    // set initial values
      rtcManagement.magicNumber = SEND;
      rtcManagement.valueCounter = 0;
      system_rtc_mem_write(64, &rtcManagement, 8);
      Serial.println("before sleep w RF");
      ESP.deepSleep(10, WAKE_RFCAL);
    }
  }
  else {  // Send to Cloud
    startTime = millis();
    Serial.println();
    Serial.println();
    WiFi.mode(WIFI_STA);
    Serial.print("Start Sending values. Connecting to ");
    Serial.println(ASP_ssid);
    WiFi.begin(ASP_ssid, ASP_password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Sending ");

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
    Serial.print("Writing to Cloud done. It took ");
    Serial.print(millis() - startTime) / 1000.0;
    Serial.println(" Seconds ");
    ESP.deepSleep(SLEEPTIME, WAKE_NO_RFCAL);
  }
}


void loop() {}

