// =========================== Online Firmware Update ===================================================
void checkForUpdates() {
  String fwURL = String(fwUrlBase);
  fwURL.concat(name);
  String fwVersionURL = fwURL;
  fwVersionURL.concat(".version");
  HTTPClient http;
  http.begin(wifiClient, fwVersionURL);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String newFWVersion = http.getString();
    int newVersion = newFWVersion.toInt();
    if (newVersion > FW_VERSION) {
      String fwImageURL = fwURL;
      fwImageURL.concat(".bin");
      t_httpUpdate_return ret = httpUpdate.update(wifiClient, fwImageURL);
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          break;
        case HTTP_UPDATE_NO_UPDATES:
          break;
      }
    }
  }
  http.end();
  up_date = 0;
  delay(2);
}
