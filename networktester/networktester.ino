#include <M5Stack.h>				//  https://github.com/m5stack/M5Stack
#include <M5_UI.h>					//  https://github.com/dsiberia9s/M5_UI
#include <LoRaWan.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define M5go
#define M5gps

#ifdef M5go
#include <NeoPixelBrightnessBus.h>  //  https://github.com/Makuna/NeoPixelBus
#endif

#ifdef M5gps
#include <TinyGPS++.h>        //  https://github.com/mikalhart/TinyGPSPlus
#endif

//Task
TaskHandle_t TaskGPS;
TaskHandle_t TaskPixel;

//Image
extern const unsigned char gImage_logoM5[];

#ifdef M5go
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
#endif

#ifdef M5gps
//GPS
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
HardwareSerial serialgps(2);
#endif
float latitude, longitude, hdop, alt, hdop2;
int sats;


//LoRa
int isf = 0;
int oldisf = 0;
char *dr[6] = {"DR5", "DR4", "DR3", "DR2", "DR1", "DR0"};
char *sf[6] = {"SF7", "SF8", "SF9", "SF10", "SF11", "SF12"};
RTC_DATA_ATTR int iwm = 0;
char *workmode[7] = {"NACK", "ACK", "MAN", "LCM", "SSV", "OTAA", "SET"};
char buffer[256];
short length;
short rssi;
float snr;
char charsnr[5];
short gwcnt;
byte coords[9];
byte ncoords[1];
long sentMillis = 0;
long currentMillis = 0;
RTC_DATA_ATTR int iiv = 0;
long interval[5] = {15000, 30000, 45000, 60000, 120000};
char *ttext[5] = {"15s", "30s", "45s", "60s", "120s"};
RTC_DATA_ATTR int cnt = -1;
String txcnt;
int otaa = 0;
int otaaack = 0;

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
RTC_DATA_ATTR bool powersave = false;

/* RootVar's for UI elements (note: not edit manually) */
String UIInputbox_6nssds = "";        //No GWs for LCR
String UITextbox_vimqus = "SF7";      //SpreadingFactor (B2)
String UITextbox_eq79hh46 = "NACK";   //Workmode  (B1)
String UITextbox_67ofwdh = "Dim";     //Dimming (B3)
String UIProgressbar_eymzer = "0";   //Progressbar RSSI
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

#ifdef M5gps
static void gpsupdate(void * pcParameters)
{
  for (;;) {
    unsigned long start = millis();
    do {
      while (serialgps.available()) {
        gps.encode(serialgps.read());
      }
    } while (millis() - start < 1000);
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
  }
}
#endif

#ifdef M5go
static void pixelupdate(void * pcParameters)
{
  for (;;) {

#ifdef M5gps
    //Change NeoPixel 4
    if (gps.satellites.value() < 3) {
      strip.SetPixelColor(4, red);
    }
    else if (gps.satellites.value() < 6) {
      strip.SetPixelColor(4, yellow);
    }
    else {
      strip.SetPixelColor(4, green);
    }

    //Change NeoPixel 0
    if (gps.hdop.value() < 500) {
      strip.SetPixelColor(0, green);
    }
    else if (gps.hdop.value() < 1000) {
      strip.SetPixelColor(0, yellow);
    }
    else {
      strip.SetPixelColor(0, red);
    }

    //Change NeoPixel 2
    if (gps.location.isValid() == false) {
      strip.SetPixelColor(2, red);
    }
    else if (gps.location.isValid() && gps.location.age() > 2000) {
      strip.SetPixelColor(2, red);
    }
    else if (gps.location.isValid() == true) {
      strip.SetPixelColor(2, green);
    }
    else {
      strip.SetPixelColor(2, green);
    }
#endif

    //Change NeoPixel 7 for RX status
    if ((iwm == 0) || (iwm == 4) || (iwm == 6)) {
      strip.SetPixelColor(7, off);
    }

    strip.Show();

    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    smartDelay(500);
  }
}
#endif

//Delay without delay
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {} while (millis() - start < ms);
}

#ifdef M5gps
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
#endif

#ifdef M5gps
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
#endif

//Settings for LoRaWAN
void initlora() {

  lora.init();

  if (powersave == false) {
    delay(1000);

    memset(buffer, 0, 256);
    lora.getVersion(buffer, 256, 1);
    Serial.print(buffer);

    memset(buffer, 0, 256);
    lora.getId(buffer, 256, 1);
    Serial.print(buffer);

    // void setId(char *DevAddr, char *DevEUI, char *AppEUI);
    lora.setId("yourdeviceaddress", NULL, NULL);            //for ABP
    //lora.setId("ABP-yourdeviceaddress", "OTAA-yourdeviceEUI", "OTAA-yourAppEUI"); //for OTAA

    // setKey(char *NwkSKey, char *AppSKey, char *AppKey);
    lora.setKey("yourNetworkSKey", "yourappSKey", NULL);          //for ABP
    //lora.setKey("ABP-yourNetworkSKey", "ABP-yourappSKey", "OTAAyourAppKey);   //for OTAA

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
    lora.setDutyCycle(true);
    lora.setDeviceLowPower();
  }
}

//Settings for LoRaWAN ABP
void initloraabp() {
  lora.sendDevicePing();
  lora.setDeviceMode(LWABP);
  lora.setAdaptiveDataRate(false);
  lora.setDutyCycle(true);
  otaa = 0;
  cnt = -1;
  lora.setDeviceLowPower();
}

//Settings for LoRaWAN OTAA
void initloraotaa() {
  lora.sendDevicePing();
  lora.setDeviceMode(LWOTAA);
  lora.setAdaptiveDataRate(true);
  lora.setDutyCycle(false);
  UISet(&UIInputbox_awnh87, "Joining");
  while (!lora.setOTAAJoin(JOIN, 10));
  UISet(&UIInputbox_awnh87, "Joined");
  lora.setDutyCycle(true);
  otaa = 1;
  cnt = -1;
  lora.setDeviceLowPower();
}

//Send data using LoRaWAN
void sendobject() {
  bool result = false;

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

#ifndef M5gps
  if (iwm == 0) {
#else
  if (iwm == 0 && gps.location.isValid() == true && gps.location.age() < 2000) {
#endif

    UISet(&UIInputbox_awnh87, "Sending");
    lora.sendDevicePing();

    if (oldisf != isf) {
      if (isf == 0) {
        lora.setDataRate(DR5, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 1) {
        lora.setDataRate(DR4, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 2) {
        lora.setDataRate(DR3, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 3) {
        lora.setDataRate(DR2, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 4) {
        lora.setDataRate(DR1, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 5) {
        lora.setDataRate(DR0, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      }
    }

#ifdef M5gps
    result = lora.transferPacket(coords, sizeof(coords), 5);
#else
    result = lora.transferPacket(ncoords, sizeof(ncoords), 5);
#endif

    if (result == true) {
      cnt++;
      txcnt = String("Sent " + String(cnt));
      UISet(&UIInputbox_awnh87, txcnt);
    } else if (lora.dutycycle == true) {
      UISet(&UIInputbox_awnh87, "DutyCycle");
    } else {
      UISet(&UIInputbox_awnh87, "Error");
    }
#ifndef M5gps
  } else if ((iwm == 1) || (iwm == 2)) {
#else
  } else if (((iwm == 1) && gps.location.isValid() == true && gps.location.age() < 2000) || (iwm == 2)) {
#endif

    UISet(&UIInputbox_awnh87, "ACK");
    lora.sendDevicePing();

    if (isf == 0) {
      lora.setDataRate(DR5, EU868);
      oldisf = 6;
      cnt = -1;
    } else if (isf == 1) {
      lora.setDataRate(DR4, EU868);
      oldisf = 6;
      cnt = -1;
    } else if (isf == 2) {
      lora.setDataRate(DR3, EU868);
      oldisf = 6;
      cnt = -1;
    } else if (isf == 3) {
      lora.setDataRate(DR2, EU868);
      oldisf = 6;
      cnt = -1;
    } else if (isf == 4) {
      lora.setDataRate(DR1, EU868);
      oldisf = 6;
      cnt = -1;
    } else if (isf == 5) {
      lora.setDataRate(DR0, EU868);
      oldisf = 6;
      cnt = -1;
    }

#ifdef M5gps
    result = lora.transferPacketWithConfirmed(coords, sizeof(coords), 5);
#else
    result = lora.transferPacketWithConfirmed(ncoords, sizeof(ncoords), 5);
#endif


    if (result == true) {
      cnt++;

      UISet(&UIInputbox_awnh87, "ACK OK");
      if (iwm == 1){
        M5.Speaker.beep();
      }

      short length;
      short rssi;
      float snr;
      char charsnr[5];
      short gwcnt;

      memset(buffer, 0, 256);
      length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);

      dtostrf(snr, 5, 1, charsnr);

      UISet(&UIProgressbar_eymzer, (rssi + 130) * 2);
      UISet(&UITextbox_859t1hi, rssi);
      UISet(&UITextbox_olwwlae, charsnr);

#ifdef M5go
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
#endif

    } else {
      UISet(&UIInputbox_awnh87, "Error");
      UISet(&UITextbox_859t1hi, "-130");
      UISet(&UITextbox_olwwlae, "-20.0");
      UISet(&UIProgressbar_eymzer, 0);
    }
  } else if (iwm == 3) {

    UISet(&UIInputbox_awnh87, "LCR");
    lora.sendDevicePing();

    if (oldisf != isf) {
      if (isf == 0) {
        lora.setDataRate(DR5, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 1) {
        lora.setDataRate(DR4, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 2) {
        lora.setDataRate(DR3, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 3) {
        lora.setDataRate(DR2, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 4) {
        lora.setDataRate(DR1, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      } else if (isf == 5) {
        lora.setDataRate(DR0, EU868);
        lora.setAdaptiveDataRate(false);
        lora.setDutyCycle(true);
        oldisf = isf;
        cnt = -1;
      }
    }

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

      UISet(&UIProgressbar_eymzer, (rssi + 130) * 2);
      UISet(&UITextbox_859t1hi, rssi);
      UISet(&UITextbox_olwwlae, charsnr);
      UISet(&UIInputbox_6nssds, gwcnt);

#ifdef M5go
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
#endif

    } else if (lora.dutycycle == true) {
      UISet(&UIInputbox_awnh87, "DutyCycle");
    } else {
      UISet(&UIInputbox_awnh87, "Error");
      UISet(&UITextbox_859t1hi, "-130");
      UISet(&UITextbox_olwwlae, "-20.0");
      UISet(&UIProgressbar_eymzer, 0);
      UISet(&UIInputbox_6nssds, 0);
    }
  }
  lora.setDeviceLowPower();
}

void sendobjectotaa() {

  bool result = false;

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

  lora.sendDevicePing();

  UISet(&UIInputbox_awnh87, "Sending");

#ifdef M5gps
  if (otaaack == 0) {
    result = lora.transferPacket(coords, sizeof(coords), 5);
  } else if (otaaack == 1) {
    result = lora.transferPacketWithConfirmed(coords, sizeof(coords), 5);
  }
#else
  if (otaaack == 0) {
    result = lora.transferPacket(ncoords, sizeof(ncoords), 5);
  } else if (otaaack == 1) {
    result = lora.transferPacketWithConfirmed(ncoords, sizeof(ncoords), 5);
  }
#endif

  if (result == true) {
    cnt++;
    txcnt = String("Sent " + String(cnt));
    UISet(&UIInputbox_awnh87, txcnt);

    short length;
    short rssi;
    float snr;
    char charsnr[5];
    short gwcnt;

    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi, &snr, &gwcnt);

    dtostrf(snr, 5, 1, charsnr);
    if (rssi >= -200) {
      UISet(&UIProgressbar_eymzer, rssi + 130);
      UISet(&UITextbox_859t1hi, rssi);
      UISet(&UITextbox_olwwlae, charsnr);

#ifdef M5go
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
#endif

    }
  } else if (lora.dutycycle == true) {
    UISet(&UIInputbox_awnh87, "DutyCycle");
  } else {
    UISet(&UIInputbox_awnh87, "Error");
  }
  lora.setDeviceLowPower();
}

#ifdef M5gps
//SiteSurvey function
void ssv() {

  UISet(&UIInputbox_awnh87, "SSV running");
  ssvinit();
  lora.sendDevicePing();
  lora.setDutyCycle(false);

  bool result = false;
  ssvresult = "DR ";

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
  lora.setDeviceLowPower();
  isf = 0;
  cnt = -1;
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
#endif


//initial setup
void setup() {
  /* Prepare M5STACK */
  M5.begin();
  M5.Power.begin();
  Wire.begin();
#ifdef M5gps
  serialgps.begin(9600, SERIAL_8N1, 16, 17);
#endif
  M5.Lcd.setBrightness(50);
  //M5.Lcd.pushImage(0, 0, 320, 240, (uint16_t *)imgName);
  M5.Lcd.pushImage(0, 0, 320, 240, (uint16_t *)gImage_logoM5);
  initlora();

#ifdef M5gps
  xTaskCreatePinnedToCore(
    gpsupdate,
    "TaskGPS",
    10000,
    NULL,
    1,
    &TaskGPS,
    0);
#endif

#ifdef M5go
  strip.Begin();
  strip.Show();

  if (powersave == false) {
    strip.SetBrightness(50);
  } else {
    strip.SetBrightness(0);
  }

  xTaskCreatePinnedToCore(
    pixelupdate,
    "TaskPixel",
    10000,
    NULL,
    1,
    &TaskPixel,
    0);
#endif

  /* Prepare UI */
  UIBegin();
  LayerFunction_default(0);

#ifdef M5gps
  M5.Lcd.pushImage(5, 2, 24, 24, (uint16_t *)ICON_10_24);
  M5.Lcd.pushImage(65, 5, 24, 24, (uint16_t *)ICON_23_24);
#endif

  if (SD.exists(filename)) {
    M5.Lcd.pushImage(200, 5, 24, 24, (uint16_t *)ICON_22_24);
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

#ifndef M5gps
  UIDisable(true, &UITextbox_4t0l0bn);
  UIDisable(true, &UITextbox_q7sl3uo);
#endif

  Serial.println("Started");

  if (powersave == true) {
    smartDelay(1000);
#ifdef M5gps
    gpsdata();
#endif
    sendobject();
    esp_sleep_enable_timer_wakeup(interval[iiv] * 1000);
    esp_deep_sleep_start();
  }

}

void loop() {

  //update button status
  if (M5.BtnA.wasPressed()) {
    if (iwm == 6) {
      iwm = 0;
      UISet(&UITextbox_eq79hh46, workmode[iwm]);
      if (otaa == 1) {
        initloraabp();
      }
    } else if (iwm == 3) {
      iwm++;
#ifndef M5gps
      iwm++;
#endif
      UISet(&UITextbox_eq79hh46, workmode[iwm]);
    } else {
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
      UISet(&UITextbox_859t1hi, "-130");
      UISet(&UITextbox_olwwlae, "-20.0");
      UISet(&UIProgressbar_eymzer, 0);
    } else if (iwm == 2) {
      UISet(&UITextbox_67ofwdh, "Send");
      UISet(&UITextbox_859t1hi, "-130");
      UISet(&UITextbox_olwwlae, "-20.0");
      UISet(&UIProgressbar_eymzer, 0);
    } else if (iwm == 3) {
      UIDisable(false, &UIInputbox_6nssds);
      UISet(&UITextbox_859t1hi, "-130");
      UISet(&UITextbox_olwwlae, "-20.0");
      UISet(&UIProgressbar_eymzer, 0);
      UISet(&UIInputbox_6nssds, "");
    } else if (iwm == 4) {
      UIDisable(true, &UIProgressbar_eymzer);
      UIDisable(true, &UITextbox_859t1hi);
      UIDisable(true, &UITextbox_olwwlae);
      UIDisable(true, &UIInputbox_6nssds);
      UIDisable(true, &UITextbox_7mnuudb);
    } else if (iwm == 5) {
#ifndef M5gps
      UIDisable(true, &UIInputbox_6nssds);
      UIDisable(true, &UITextbox_7mnuudb);
#endif
      UISet(&UITextbox_vimqus, "Join");
      UISet(&UITextbox_67ofwdh, "NACK");
      UIDisable(false, &UIProgressbar_eymzer);
      UIDisable(false, &UITextbox_859t1hi);
      UIDisable(false, &UITextbox_olwwlae);
      UISet(&UITextbox_859t1hi, "-130");
      UISet(&UITextbox_olwwlae, "-20.0");
      UISet(&UIProgressbar_eymzer, 0);
      //strip.SetPixelColor(7, off);
    } else if (iwm == 6) {
      UISet(&UITextbox_vimqus, ttext[iiv]);
      UIDisable(true, &UIInputbox_awnh87);
      UIDisable(true, &UIProgressbar_eymzer);
      UIDisable(true, &UITextbox_859t1hi);
      UIDisable(true, &UITextbox_olwwlae);
      UISet(&UITextbox_67ofwdh, "PS");
    }
  }

  if (M5.BtnB.wasPressed()) {
    if (isf == 5) {
      isf = 0;
      UISet(&UITextbox_vimqus, sf[isf]);
    } else if (iwm == 5 && otaa == 0) {
      initloraotaa(); //OTAA Join
      UISet(&UITextbox_vimqus, "Send");
    } else if (iwm == 5 && otaa == 1) {
      sendobjectotaa(); //Manual send
    } else if (iwm == 6) {
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
#ifdef M5go
      strip.SetBrightness(0);
#endif
    } else if (iwm < 2 && dim == true) {
      dim = false;
      M5.Lcd.setBrightness(50);
#ifdef M5go
      strip.SetBrightness(50);
#endif
    } else if (iwm == 4) {
#ifdef M5gps
      ssv();
#endif
    } else if (iwm == 5 && otaaack == 0) {
      otaaack = 1;
      UISet(&UITextbox_67ofwdh, "ACK");
    } else if (iwm == 5 && otaaack == 1) {
      otaaack = 0;
      UISet(&UITextbox_67ofwdh, "NACK");
    } else if (iwm == 6 && powersave == false) {
      powersave = true;
      iwm = 0;
    } else if (iwm == 6 && powersave == true) {
      powersave = false;
    } else if (iwm > 1) {
      sendobject();
    }
  }

#ifdef M5gps
  //Update GPS Data
  gpsdata();
#endif

#ifdef M5gps
  //Print satellites and change NeoPixel 4
  sats = gps.satellites.value();
  UISet(&UITextbox_4t0l0bn, sats);

  //Print HDOP and change NeoPixel 0
  hdop = gps.hdop.value();
  hdop2 = hdop / 100.0;
  String stringhdop = String(hdop2);
  UISet(&UITextbox_q7sl3uo, stringhdop);

  //Print GPS fix status und change NeoPixel 2
  if (gps.location.isValid() == false) {
    M5.Lcd.pushImage(160, 5, 24, 24, (uint16_t *)ICON_25_24);
  }
  else if (gps.location.isValid() && gps.location.age() > 2000) {
    M5.Lcd.pushImage(160, 5, 24, 24, (uint16_t *)ICON_25_24);
  }
  else if (gps.location.isValid() == true) {
    M5.Lcd.pushImage(160, 5, 24, 24, (uint16_t *)ICON_20_24);
  }
  else {
    M5.Lcd.pushImage(160, 5, 24, 24, (uint16_t *)ICON_20_24);
  }
#endif

  //Battery Status
  if (M5.Power.isCharging() == true) {
    M5.Lcd.pushImage(240, 5, 24, 24, (uint16_t *)ICON_40_24);
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

#ifdef M5go
  strip.Show();
#endif

#ifdef M5gps
  //Init of SD Card for GPX-file
  if (sdwrite == false) {
    gpxinit();
  }

  //Write GPS-Track
  if (sdwrite == true) {
    writegpx();
  }
#endif

  //Sending intervall
  currentMillis = millis();
  if ((currentMillis - sentMillis > interval[iiv]) && iwm < 2) {
    sendobject();
  }

#ifdef M5gps
  if ((currentMillis - sentMillis > interval[iiv]) && iwm == 5 && otaa == 1 && gps.location.isValid() == true && gps.location.age() < 2000) {
    sendobjectotaa();
  }
#else
  if ((currentMillis - sentMillis > interval[iiv]) && iwm == 5 && otaa == 1) {
    sendobjectotaa();
  }
#endif

  //light sleep timer
  if (powersave == true) {
#ifdef M5go
    strip.SetBrightness(0);
#endif
    esp_sleep_enable_timer_wakeup(interval[iiv] * 1000);
    esp_deep_sleep_start();
  }

  //used to deflicker the display, more possible, but with less reactive buttons
  smartDelay(200);

  M5.update();
}
