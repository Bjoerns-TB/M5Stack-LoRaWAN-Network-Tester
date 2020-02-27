#include <M5Stack.h>				//  https://github.com/m5stack/M5Stack
#include <TinyGPS++.h>				//  https://github.com/mikalhart/TinyGPSPlus
#include <NeoPixelBrightnessBus.h>	//  https://github.com/Makuna/NeoPixelBus
#include <M5_UI.h>					//  https://github.com/dsiberia9s/M5_UI
#include <LoRaWan.h>

//Task
TaskHandle_t TaskGPS;

//Image
extern const unsigned char gImage_logoM5[];

//NeoPixel
const uint16_t PixelCount = 10;
const uint8_t PixelPin = 15;
NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
RgbColor red(128, 0, 0);
RgbColor green(0, 128, 0);
RgbColor blue(0, 0, 128);
RgbColor lightblue(0, 95, 128);
RgbColor yellow(128, 128, 0);
RgbColor orange(128, 64, 0);
RgbColor off(0, 0, 0);

//GPS
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
HardwareSerial serialgps(2);
float latitude, longitude, hdop, alt, hdop2;
int sats;

//LoRa
int isf = 0;
int oldisf = 0;
char *dr[6] = {"DR5", "DR4", "DR3", "DR2", "DR1", "DR0"};
char *sf[6] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
int iwm = 0;
char *workmode[6] = {"NACK", "ACK", "MAN", "LCM", "SSV", "SET"};
char buffer[256];
short length;
short rssi;
float snr;
char charsnr[5];
short gwcnt;
byte coords[9];
long sentMillis = 0;
long currentMillis = 0;
int iiv = 0;
long interval[5] = {15000, 30000, 45000, 60000, 120000};
char *ttext[5] = {"15s", "30s", "45s", "60s", "120s"};
int cnt = 0;
String txcnt;

//Battery
int8_t BattLevel = 0;
#define FULL       (   3)

//SDCard
char filename[] = "/";
bool cardin = false;
bool sdwrite = false;
File dataFile;

//GPX
int year;
byte month, day, hour, minute, second;
char filename1[20];
char date1[22];
char filepath[20];

//SSV
char filename2[20];
char date2[22];
char filepath2[20];
bool firstssv = false;
bool lastssv = false;
String ssvresult = "DR ";

//M5Stack
bool dim = false;
bool powersave = false;

/* RootVar's for UI elements (note: not edit manually) */
String UIInputbox_6nssds = "";        //No GWs for LCR
String UITextbox_vimqus = "SF7";      //SpreadingFactor (B2)
String UITextbox_eq79hh46 = "NACK";   //Workmode  (B1)
String UITextbox_67ofwdh = "Dim";     //Dimming (B3)
String UIProgressbar_eymzer = "70";   //Progressbar RSSI
String UITextbox_859t1hi = "-130";    //RSSI
String UIInputbox_awnh87 = "inactive";//Status
String UITextbox_4t0l0bn = "0";		  //Stattelites
String UITextbox_q7sl3uo = "0";		  //HDOP
String UITextbox_403ohip = "0";		  //Battery Level
String UITextbox_olwwlae = "-20.00";  //SNR
String UITextbox_7mnuudb = "SNR";     //SNR

/* Function for layer default: */
void LayerFunction_default(String* rootVar) {
  /* UI Elements */
  UIInputbox(160, 58, 150, "default", "No of GWs", 0, &UIInputbox_6nssds);
  UITextbox(144, 214, 50, 20, 0x0000, "default", &UITextbox_vimqus);
  UITextbox(44, 215, 50, 20, 0x0000, "default", &UITextbox_eq79hh46);
  UITextbox(227, 215, 50, 20, 0x0000, "default", &UITextbox_67ofwdh);
  UIProgressbar(10, 144, 300, "default", "RSSI, dB", &UIProgressbar_eymzer);
  UITextbox(124, 142, 50, 20, 0x0000, "default", &UITextbox_859t1hi);
  UIInputbox(5, 58, 150, "default", "Status", 0, &UIInputbox_awnh87);
  UITextbox(40, 11, 25, 20, 0x0000, "default", &UITextbox_4t0l0bn);
  UITextbox(100, 11, 60, 20, 0x0000, "default", &UITextbox_q7sl3uo);
  UITextbox(270, 11, 50, 20, 0x0000, "default", &UITextbox_403ohip);
  UITextbox(249, 142, 70, 20, 0x0000, "default", &UITextbox_olwwlae);
  UITextbox(200, 142, 40, 20, 0x0000, "default", &UITextbox_7mnuudb);

  /* To open this layer use: */
  UILayer("default");
}

//Update GPS data from GPS Chip
static void smartDelay(void * pcParameters)
{
  while (true) {
    unsigned long start = millis();
    do
    {
      while (serialgps.available())
        gps.encode(serialgps.read());
    } while (millis() - start < 1000);
  }
}

//Delay without delay
static void smartDelay2(unsigned long ms)
{
  unsigned long start2 = millis();
  do
  {} while (millis() - start2 < ms);
}

//Write GPS-Data into variables
void gpsdata() {
  year = gps.date.year();
  month = gps.date.month();
  day = gps.date.day();
  hour = gps.time.hour();
  minute = gps.time.minute();
  second = gps.time.second();
  latitude = gps.location.lat();
  longitude = gps.location.lng();
  alt = gps.altitude.meters();
  hdop = gps.hdop.value();
}

//Initialize GPX-Track to SD-Card
void gpxinit() {
  if (cardin == true && gps.location.isValid() == true) {
    sdwrite = true;
    sprintf(filename1, "/%02d-%02d-%02d", day, month, year - 2000);
    sprintf(filepath, "/%02d-%02d-%02d/%02d-%02d%s", day, month, year - 2000,  hour, minute, ".GPX");

    SD.mkdir(filename1);
    if (!SD.exists(filepath)) {
      dataFile = SD.open(filepath, FILE_WRITE);
      dataFile.print(F(
                       "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                       "<gpx version=\"1.1\" creator=\"Batuev\" xmlns=\"http://www.topografix.com/GPX/1/1\" \r\n"
                       "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n"
                       "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\r\n"
                       "\t<trk>\r\n<trkseg>\r\n"));
      dataFile.print(F("</trkseg>\r\n</trk>\r\n</gpx>\r\n"));
      dataFile.close();
    }
  }
}

//Write data to GPX-File
void writegpx() {
  if (gps.location.isValid() == true) {
    gpsdata();
    sprintf(date1, "%4d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, hour, minute, second);
    dataFile = SD.open(filepath, FILE_WRITE);
    unsigned long filesize = dataFile.size();
    filesize -= 27;
    dataFile.seek(filesize);
    dataFile.print(F("<trkpt lat=\""));
    dataFile.print(latitude, 7);
    dataFile.print(F("\" lon=\""));
    dataFile.print(longitude, 7);
    dataFile.println(F("\">"));
    dataFile.print(F("<time>"));
    dataFile.print(date1);
    dataFile.println(F("</time>"));
    dataFile.print(F("<ele>"));
    dataFile.print(alt, 1);
    dataFile.print(F("</ele>\r\n<hdop>"));
    dataFile.print(hdop2, 1);
    dataFile.println(F("</hdop>\r\n</trkpt>"));
    dataFile.print(F("</trkseg>\r\n</trk>\r\n</gpx>\r\n"));
    dataFile.close();
  }
}


//Settings for LoRaWAN
void initlora() {

  lora.init();

  delay(1000);

  memset(buffer, 0, 256);
  lora.getVersion(buffer, 256, 1);
  Serial.print(buffer);

  memset(buffer, 0, 256);
  lora.getId(buffer, 256, 1);
  Serial.print(buffer);

  // void setId(char *DevAddr, char *DevEUI, char *AppEUI);
  lora.setId("yourdeviceaddress", NULL, NULL);
  // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
  lora.setKey("yourNetworkSKey", "yourappSKey", NULL);

  lora.setDeviceMode(LWABP);
  lora.setDataRate(DR5, EU868);

  lora.setChannel(0, 868.1);
  lora.setChannel(1, 868.3);
  lora.setChannel(2, 868.5);
  lora.setChannel(3, 867.1);
  lora.setChannel(4, 867.3);
  lora.setChannel(5, 867.5);
  lora.setChannel(6, 867.7);
  lora.setChannel(7, 867.9);

  lora.setReceiveWindowFirst(0, 868.1);
  lora.setReceiveWindowSecond(869.525, DR3);

  lora.setPower(14);
  lora.setPort(1);
  lora.setAdaptiveDataRate(false);
}

//Send data using LoRaWAN
void sendobject() {
  bool result = false;

  if (oldisf != isf) {
    if (isf == 0) {
      lora.setDataRate(DR5, EU868);
      oldisf = isf;
      cnt = 0;
    } else if (isf == 1) {
      lora.setDataRate(DR4, EU868);
      oldisf = isf;
      cnt = 0;
    } else if (isf == 2) {
      lora.setDataRate(DR3, EU868);
      oldisf = isf;
      cnt = 0;
    } else if (isf == 3) {
      lora.setDataRate(DR2, EU868);
      oldisf = isf;
      cnt = 0;
    } else if (isf == 4) {
      lora.setDataRate(DR1, EU868);
      oldisf = isf;
      cnt = 0;
    } else if (isf == 5) {
      lora.setDataRate(DR0, EU868);
      oldisf = isf;
      cnt = 0;
    }
  }

  int32_t lat = latitude * 10000;
  int32_t lon = longitude * 10000;
  int16_t altitude = alt * 100;
  int8_t hdopGPS = hdop / 10;

  coords[0] = lat;
  coords[1] = lat >> 8;
  coords[2] = lat >> 16;

  coords[3] = lon;
  coords[4] = lon >> 8;
  coords[5] = lon >> 16;

  coords[6] = altitude;
  coords[7] = altitude >> 8;

  coords[8] = hdopGPS;

  sentMillis = millis();

  if (iwm == 0 && gps.location.isValid() == true) {
    result = lora.transferPacket(coords, sizeof(coords), 4);

    UISet(&UIInputbox_awnh87, "Sending");

    if (result == true) {
      cnt++;
      txcnt = String("Sent " + String(cnt));
      UISet(&UIInputbox_awnh87, txcnt);
    } else {
      UISet(&UIInputbox_awnh87, "Error");
    }
  } else if (((iwm == 1) && gps.location.isValid() == true) || (iwm == 2)) {
    UISet(&UIInputbox_awnh87, "ACK");
    result = lora.transferPacketWithConfirmed(coords, sizeof(coords), 4);

    if (result == true) {
      cnt++;

      UISet(&UIInputbox_awnh87, "ACK OK");

      short length;
      short rssi;
      float snr;
      char charsnr[5];
      short gwcnt;

      memset(buffer, 0, 256);
      length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);

      dtostrf(snr, 5, 1, charsnr);

      UISet(&UIProgressbar_eymzer, rssi + 130);
      UISet(&UITextbox_859t1hi, rssi);
      UISet(&UITextbox_olwwlae, charsnr);

      if (rssi < -120) {
        strip.SetPixelColor(7, blue);
      } else if (rssi < -115) {
        strip.SetPixelColor(7, lightblue);
      } else if (rssi < -110) {
        strip.SetPixelColor(7, green);
      } else if (rssi < -105) {
        strip.SetPixelColor(7, yellow);
      } else if (rssi < -100) {
        strip.SetPixelColor(7, orange);
      } else {
        strip.SetPixelColor(7, red);
      }

    } else {
      UISet(&UIInputbox_awnh87, "Error");
    }
  } else if (iwm == 3) {
    UISet(&UIInputbox_awnh87, "LCR");
    result = lora.transferPacketLinkCheckReq(5);

    if (result == true) {
      cnt++;

      UISet(&UIInputbox_awnh87, "LCR OK");

      short length;
      short rssi;
      float snr;
      char charsnr[5];
      short gwcnt;

      memset(buffer, 0, 256);
      length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);

      dtostrf(snr, 5, 1, charsnr);

      UISet(&UIProgressbar_eymzer, rssi + 130);
      UISet(&UITextbox_859t1hi, rssi);
      UISet(&UITextbox_olwwlae, charsnr);
      UISet(&UIInputbox_6nssds, gwcnt);

    } else {
      UISet(&UIInputbox_awnh87, "Error");
    }
  }
}

//SiteSurvey function
void ssv() {

  ssvinit();
  lora.setDutyCycle(false);

  bool result = false;
  ssvresult = "DR ";

  UISet(&UIInputbox_awnh87, "SSV running");

  lora.setDataRate(DR5, EU868);
  isf = 0;
  result = lora.transferPacketLinkCheckReq(5);

  if (result == true) {
    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);
    writessv();
    bool result = false;
    ssvresult += "5";
  }

  lora.setDataRate(DR4, EU868);
  isf = 1;
  result = lora.transferPacketLinkCheckReq(5);

  if (result == true) {
    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);
    writessv();
    bool result = false;
    ssvresult += "4";
  }

  lora.setDataRate(DR3, EU868);
  isf = 2;
  result = lora.transferPacketLinkCheckReq(5);

  if (result == true) {
    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);
    writessv();
    bool result = false;
    ssvresult += "3";
  }

  lora.setDataRate(DR2, EU868);
  isf = 3;
  result = lora.transferPacketLinkCheckReq(5);

  if (result == true) {
    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);
    writessv();
    bool result = false;
    ssvresult += "2";
  }

  lora.setDataRate(DR1, EU868);
  isf = 4;
  result = lora.transferPacketLinkCheckReq(5);

  if (result == true) {
    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);
    writessv();
    bool result = false;
    ssvresult += "1";
  }

  lora.setDataRate(DR0, EU868);
  isf = 5;
  result = lora.transferPacketLinkCheckReq(5);

  if (result == true) {
    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);
    writessv();
    bool result = false;
    ssvresult += "0";
  }

  lastssv = true;
  writessv();
  lastssv = false;
  firstssv = false;

  UISet(&UIInputbox_awnh87, ssvresult);

  lora.setDutyCycle(true);
  lora.setDataRate(DR5, EU868);
  isf = 0;
  cnt = 0;
}

//Initialize GeoJSON file
void ssvinit() {
  if (cardin == true && gps.location.isValid() == true) {
    sprintf(filename2, "/%02d-%02d-%02d", day, month, year - 2000);
    sprintf(filepath2, "/%02d-%02d-%02d/%02d-%02d%s", day, month, year - 2000,  hour, minute, ".json");

    SD.mkdir(filename2);
    if (!SD.exists(filepath2)) {
      dataFile = SD.open(filepath2, FILE_WRITE);
      dataFile.close();
    }
  }
}

//Write data to GeoJSON file
void writessv() {
  if (gps.location.isValid() == true) {
    gpsdata();
    sprintf(date1, "%4d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, hour, minute, second);

    dataFile = SD.open(filepath2, FILE_WRITE);
    unsigned long filesize = dataFile.size();
    dataFile.seek(filesize);

    if (lastssv == false) {
      if (firstssv == false) {
        firstssv = true;
        dataFile.println(F("{"));
        dataFile.println(F("\"type\": \"FeatureCollection\","));
        dataFile.println(F("\"features\": [{"));
      } else {
        dataFile.println(F(",{"));
      }
      dataFile.println(F("\"type\": \"Feature\","));
      dataFile.println(F("\"properties\": {"));
      dataFile.print(F("\"sf\": \""));
      dataFile.print(sf[isf]);
      dataFile.print(F("\",\r\n"));
      dataFile.print(F("\"rssi\": \""));
      dataFile.print(rssi);
      dataFile.print(F("\",\r\n"));
      dataFile.print(F("\"snr\": \""));
      dataFile.print(snr);
      dataFile.print(F("\",\r\n"));
      dataFile.print(F("\"gwcnt\": \""));
      dataFile.print(gwcnt);
      dataFile.print(F("\",\r\n"));
      dataFile.println(F("\"marker-color\": \"#008800\","));
      dataFile.println(F("\"marker-symbol\": \"lighthouse\""));
      dataFile.println(F("},"));
      dataFile.println(F("\"geometry\": {"));
      dataFile.println(F("\"type\": \"Point\","));
      dataFile.print(F("\"coordinates\": ["));
      dataFile.print(longitude, 7);
      dataFile.print(F(", "));
      dataFile.print(latitude, 7);
      dataFile.print(F("]\r\n"));
      dataFile.println(F("}"));
      dataFile.println(F("}"));
      dataFile.close();
    } else {
      dataFile.println(F("]}"));
      dataFile.close();
    }
  }
}


//initial setup
void setup() {
  /* Prepare M5STACK */
  M5.begin();
  M5.Power.begin();
  Wire.begin();
  serialgps.begin(9600, SERIAL_8N1, 16, 17);
  strip.Begin();
  strip.Show();
  strip.SetBrightness(50);
  M5.Lcd.setBrightness(50);
  //M5.Lcd.drawBitmap(0, 0, 320, 240, (uint16_t *)imgName);
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint16_t *)gImage_logoM5);
  initlora();

  xTaskCreatePinnedToCore(
    smartDelay,
    "TaskGPS",
    10000,
    NULL,
    0,
    &TaskGPS,
    1);

  /* Prepare UI */
  UIBegin();
  LayerFunction_default(0);
  M5.Lcd.drawBitmap(5, 2, 24, 24, (uint16_t *)ICON_10_24);
  M5.Lcd.drawBitmap(65, 5, 24, 24, (uint16_t *)ICON_23_24);

  if (SD.exists(filename)) {
    M5.Lcd.drawBitmap(200, 5, 24, 24, (uint16_t *)ICON_22_24);
    cardin = true;
  }

  //Prepare UI for iwm = 0
  UISet(&UITextbox_vimqus, sf[isf]);
  UIDisable(true, &UIProgressbar_eymzer);
  UIDisable(true, &UITextbox_859t1hi);
  UIDisable(true, &UITextbox_olwwlae);
  UIDisable(true, &UIInputbox_6nssds);
  UIDisable(true, &UITextbox_7mnuudb);
  UIDisable(false, &UIInputbox_awnh87);

  Serial.println("Started");
}

void loop() {

  //update button status
  if (M5.BtnA.wasPressed()) {
    if (iwm == 5) {
      iwm = 0;
      UISet(&UITextbox_eq79hh46, workmode[iwm]);
    }
    else {
      iwm++;
      UISet(&UITextbox_eq79hh46, workmode[iwm]);
    }

    if (iwm == 0) {
      UISet(&UITextbox_vimqus, sf[isf]);
      UIDisable(false, &UIInputbox_awnh87);
      UISet(&UITextbox_67ofwdh, "Dim");
    } else if (iwm == 1) {
      UIDisable(false, &UIProgressbar_eymzer);
      UIDisable(false, &UITextbox_859t1hi);
      UIDisable(false, &UITextbox_olwwlae);
      UIDisable(false, &UITextbox_7mnuudb);
    } else if (iwm == 2) {
      UISet(&UITextbox_67ofwdh, "Send");
    } else if (iwm == 3) {
      UIDisable(false, &UIInputbox_6nssds);
    } else if (iwm == 4) {
      UIDisable(true, &UIProgressbar_eymzer);
      UIDisable(true, &UITextbox_859t1hi);
      UIDisable(true, &UITextbox_olwwlae);
      UIDisable(true, &UIInputbox_6nssds);
      UIDisable(true, &UITextbox_7mnuudb);
    } else if (iwm == 5) {
      UISet(&UITextbox_vimqus, ttext[iiv]);
      UIDisable(true, &UIInputbox_awnh87);
      UISet(&UITextbox_67ofwdh, "PS");
      strip.SetPixelColor(7, off);
    }
  }

  if (M5.BtnB.wasPressed()) {
    if (isf == 5) {
      isf = 0;
      UISet(&UITextbox_vimqus, sf[isf]);
    }
    else if (iwm == 5) {
      if (iiv == 4) {
        iiv = 0;
        UISet(&UITextbox_vimqus, ttext[iiv]);
      } else {
        iiv++;
        UISet(&UITextbox_vimqus, ttext[iiv]);
      }
    } else {
      isf++;
      UISet(&UITextbox_vimqus, sf[isf]);
    }
  }

  if (M5.BtnC.wasPressed()) {
    if (iwm < 2 && dim == false) {
      dim = true;
      M5.Lcd.setBrightness(0);
      strip.SetBrightness(0);
    } else if (iwm < 2 && dim == true) {
      dim = false;
      M5.Lcd.setBrightness(50);
      strip.SetBrightness(50);
    } else if (iwm == 4) {
      ssv();
    } else if (iwm == 5 && powersave == false) {
      powersave = true;
      iwm = 0;
    } else if (iwm == 5 && powersave == true) {
      powersave = false;
    } else if (iwm > 1) {
      sendobject();
    }
  }

  //Print satellites and change NeoPixel 4
  sats = gps.satellites.value();
  UISet(&UITextbox_4t0l0bn, sats);

  if (gps.satellites.value() < 3) {
    strip.SetPixelColor(4, red);
  }
  else if (gps.satellites.value() < 6) {
    strip.SetPixelColor(4, yellow);
  }
  else {
    strip.SetPixelColor(4, green);
  }

  //Print HDOP and change NeoPixel 0
  hdop = gps.hdop.value();
  hdop2 = hdop / 100.0;
  String stringhdop = String(hdop2);
  UISet(&UITextbox_q7sl3uo, stringhdop);

  if (gps.hdop.value() < 500) {
    strip.SetPixelColor(0, green);
  }
  else if (gps.hdop.value() < 1000) {
    strip.SetPixelColor(0, yellow);
  }
  else {
    strip.SetPixelColor(0, red);
  }

  //Print GPS fix status und change NeoPixel 2
  if (gps.location.isValid() == false) {
    strip.SetPixelColor(2, red);
    M5.Lcd.drawBitmap(160, 5, 24, 24, (uint16_t *)ICON_25_24);
  }
  else if (gps.location.isValid() && gps.location.age() > 2000) {
    strip.SetPixelColor(2, red);
    M5.Lcd.drawBitmap(160, 5, 24, 24, (uint16_t *)ICON_25_24);
  }
  else if (gps.location.isValid() == true) {
    strip.SetPixelColor(2, green);
    M5.Lcd.drawBitmap(160, 5, 24, 24, (uint16_t *)ICON_20_24);
  }
  else {
    strip.SetPixelColor(2, green);
    M5.Lcd.drawBitmap(160, 5, 24, 24, (uint16_t *)ICON_20_24);
  }

  //Battery Status
  if (M5.Power.isCharging() == true) {
    M5.Lcd.drawBitmap(240, 5, 24, 24, (uint16_t *)ICON_40_24);
  }

  if (M5.Power.isChargeFull() == true) {
    UISet(&UITextbox_403ohip, "Full");
  }
  else {
    BattLevel = M5.Power.getBatteryLevel();
    String strbattlevel = String(BattLevel);
    strbattlevel = String(strbattlevel + "%");
    UISet(&UITextbox_403ohip, strbattlevel);
  }

  strip.Show();

  //Update GPS Data
  gpsdata();

  //Init of SD Card for GPX-file
  if (sdwrite == false) {
    gpxinit();
  }

  //Write GPS-Track
  if (sdwrite == true) {
    writegpx();
  }

  //Sending intervall
  currentMillis = millis();
  if ((currentMillis - sentMillis > interval[iiv]) && iwm < 2) {
    sendobject();
  }

  //light sleep timer
  if (powersave == true) {
    strip.SetBrightness(0);
    esp_sleep_enable_timer_wakeup(15000000);
    esp_light_sleep_start();
  }

  //used to deflicker the display, more possible, but with less reactive buttons
  smartDelay2(200);

  M5.update();
}
