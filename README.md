# M5Stack-LoRaWAN-Network-Tester

A LoRaWAN Network Tester based on the M5Stack, compatible with TTN

## Setup
This is designed to work with the following hardware:
  - M5Core (Basic, Gray or Fire)
  - M5Go Base
  - M5Stack GPS Module
  - M5Stack LoRaWAN Module

#### Required Libraries!
  - [M5Stack]
  - [TinyGPSPlus]
  - [NeoPixelBus]
  - [M5_UI]
  - Seeeduino LoRaWan (Already included, patched and bugfixed)

 
#### Installation and Configuration
Upload this sketch to your M5 using the Arduino IDE. M5Stack Fire users have to disable PSRAM, because it will interfer with UART2.
UART2 with GPIO 16 and 17 willbe used for the GPS module and UART1 with GPIO 2 and 5 for the LoRaWAN module. Last can be changed in LoRaWan.cpp

## Instructions for Use

#### Menu

On boot you will be presented with the "Boot-Logo" followed by the first working mode. At the moment the tester has 6 modes to select:
  - [NACK](#nack) 
  - [ACK](#ack)  
  - [MAN](#man)  
  - [LCM](#lcm)
  - [SSV](#ssv)
  - [SET](#set)
 
You can move between menu items by pushing the button A. 

![Menu Image](https://raw.githubusercontent.com/ "Fig 1. Menu")
  
#### NACK 
#### (No Acknowladge)
"NACK" is a mode that utlises the current device [settings](#set) to perform periodic transmissions. "NACK" mode is great for use with TTN Mapper.

![NACK Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/nack.jpg "Fig 2. NACK")

Pushing button B will let you cicle through each spreadfactor. By pushig button C the display and LEDs will be turned off. Pushing button C again will turn them on.

#### ACK 
#### (Acknowladge)
"ACK" will perform the same test as NACK but it will request an ACK for every transmission. The RSSI and SNR values of the received packet will be shown on the display Pushing button B will let you cicle through each spreadfactor. By pushig button C the display and LEDs will be turned off. Pushing button C again will turn them on.

![ACK Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/ack.jpg "Fig 3. ACK")

#### MAN 
#### (Manual)
"MAN" will send a LoRaWAN packet with ACK by pushing button C. Pushing button B will let you cicle through each spreadfactor.

#### LCM 
#### (LinkCheckMode)
"LCM" is a mode that will trigger a LinkCheckRequest. The TTN backend will report back the number of gateways which received the request. Pushing button B will let you cicle through each spreadfactor. The request is triggered by button C.

![LCM Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/lcm.jpg "Fig 4. LCM")

#### SSV 
#### (SiteSurvey)
"SSV" 

#### SET 
#### (Settings)

  

[M5Stack]: https://github.com/m5stack/M5Stack
[TinyGPSPlus]: https://github.com/mikalhart/TinyGPSPlus
[NeoPixelBus]: https://github.com/Makuna/NeoPixelBus
[M5_UI]: https://github.com/dsiberia9s/M5_UI


