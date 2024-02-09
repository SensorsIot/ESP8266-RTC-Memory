
#define RTCMEMORYSTART 65
#define RTCMEMORYLEN 127


extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}

typedef struct {
  int battery;
  int other;
} rtcStore;

rtcStore rtcMem;

int i;
int buckets;
bool toggleFlag;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Start");
  buckets = (sizeof(rtcMem) + 3) / 4;
  Serial.print("Buckets ");
  Serial.println(buckets);
  system_rtc_mem_read(64, &toggleFlag, 4);
  Serial.print("toggle Flag ");
  Serial.println(toggleFlag);
  if (toggleFlag) {
    Serial.println("Start Writing");
    for (i = 0; i < RTCMEMORYLEN / buckets; i++) {
      rtcMem.battery = i;
      rtcMem.other = i * 11;
      int rtcPos = RTCMEMORYSTART + i * buckets;
      system_rtc_mem_write(rtcPos, &rtcMem, buckets * 4);
      toggleFlag = false;
      system_rtc_mem_write(64, &toggleFlag, 4);

      Serial.print("i: ");
      Serial.print(i);
      Serial.print(" Position: ");
      Serial.print(rtcPos);
      Serial.print(", battery: ");
      Serial.print(rtcMem.battery);
      Serial.print(", other: ");
      Serial.println(rtcMem.other);
      yield();
    }
    Serial.println("Writing done");
  }
  else {
    Serial.println("Start reading");
    for (i = 0; i < RTCMEMORYLEN / buckets; i++) {
      int rtcPos = RTCMEMORYSTART + i * buckets;
      system_rtc_mem_read(rtcPos, &rtcMem, sizeof(rtcMem));
      toggleFlag = true;
      system_rtc_mem_write(64, &toggleFlag, 4);

      Serial.print("i: ");
      Serial.print(i);
      Serial.print(" Position ");
      Serial.print(rtcPos);
      Serial.print(", battery: ");
      Serial.print(rtcMem.battery);
      Serial.print(", other: ");
      Serial.println(rtcMem.other);
      yield();
    }
    Serial.println("reading done");
    for (i = 0; i < RTCMEMORYLEN / buckets; i++) {
      rtcMem.battery = 0;
      rtcMem.other = 0;
      int rtcPos = RTCMEMORYSTART + i * buckets;
      system_rtc_mem_write(rtcPos, &rtcMem, buckets * 4);
    }
  }
  Serial.println("before sleep");
  
  ESP.deepSleep(5000000, WAKE_RFCAL);
}
void loop() {}

