// =========================== connectWiFi ==============================================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);  // Set WiFi mode to station (client)
  //  WiFi.config( ip, gateway, subnet );
#ifdef DEBUG
  Serial.print(ssid);  // to know which
  Serial.print(" ");
#endif
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }
#ifdef DEBUG
  Serial.println(" ");
#endif
}
