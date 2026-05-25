/*
***in main routine ***
  #include "time.h"
  #include <NTPtimeESP.h>
  String time_str;
  ing go;
*/
void DisplayTime() {
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 3600;
  const int daylightOffset_sec = 3600;
  configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", ntpServer);
  time_str = printLocalTime();
}
String printLocalTime() {
  // ****************** getting NTP time *****************************
  time_t timestamp = time(NULL);
  char output[80];
  struct tm* pTime = localtime(&timestamp);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
#ifdef DEBUG
    Serial.println("Failed to obtain time");
#endif
    return "----";
  }
  strftime(output, 80, "%H,%M,", &timeinfo);
  // no display at night
#ifdef DEBUG
  Serial.print("output[0] and [1] = ");
  Serial.print(output[0]);
  Serial.print(" ");
  Serial.println(output[1]);
#endif
  if ((output[0] == '0') and (output[1] < '6')) {  // no display between 24 et 7h
    go = 0;
  } else {
    go = 1;
  }
  if (output[0] == '0') {
    (output[0] = ' ');
  }
  time_str = String(output);
  return String(time_str);
}
