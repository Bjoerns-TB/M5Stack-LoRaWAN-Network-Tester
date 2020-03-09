# M5Stack-LoRaWAN-Network-Tester

A LoRaWAN Network Tester based on the M5Stack, compatible with TTN (The Things Network)

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
UART2 with GPIO 16 and 17 willbe used for the GPS module and UART1 with GPIO 2 and 5 for the LoRaWAN module. Last can be changed in LoRaWan.cpp The UART Port on the LoRaWAN module has to changed (solderpads) to use this ports.

Replace the original files LoraWan.h and LoRaWan.cpp delivered by the M5Stack Library under M5Stack/src with the ones in the src directory. Delete the src directory afterwards or move it.

Change the your TTN keys in initlora() funtion of the networktester.ino file. They are filled in as is from the TTN console. If you want yo use OTAA you have to register a second device for your application. 

## Instructions for Use

#### Menu

On boot you will be presented with the "Boot-Logo" followed by the first working mode. At the moment the tester has 7 modes to select:
  - [NACK](#nack) 
  - [ACK](#ack)  
  - [MAN](#man)  
  - [LCM](#lcm)
  - [SSV](#ssv)
  - [OTAA](#otaa)
  - [SET](#set)
 
You can move between menu items by pushing the button A. 

![Menu Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/menu.jpg "Fig 1. Menu")
  
#### NACK 
#### (No Acknowladge)
"NACK" is a mode that utlises the current device [settings](#set) to perform periodic transmissions. "NACK" mode is great for use with TTN Mapper.

![NACK Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/nack.jpg "Fig 2. NACK")

Pushing button B will let you cycle through each spreadfactor. By pushig button C the display and LEDs will be turned off. Pushing button C again will turn them on.

#### ACK 
#### (Acknowladge)
"ACK" will perform the same test as NACK but it will request an ACK for every transmission. The RSSI and SNR values of the received packet will be shown on the display. Pushing button B will let you cycle through each spreadfactor. By pushig button C the display and LEDs will be turned off. Pushing button C again will turn them on.

![ACK Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/ack.jpg "Fig 3. ACK")

#### MAN 
#### (Manual)
"MAN" will send a LoRaWAN packet with ACK by pushing button C. Pushing button B will let you cycle through each spreadfactor.

![MAN Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/man.jpg "Fig 4. MAN")

#### LCM 
#### (LinkCheckMode)
"LCM" is a mode that will trigger a LinkCheckRequest. The TTN backend will report back the number of gateways which received the request. Pushing button B will let you cycle through each spreadfactor. The request is triggered by button C.

![LCM Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/lcm.jpg "Fig 5. LCM")

#### SSV 
#### (SiteSurvey)
"SSV" is supposed as mode for testing a location. During SSV mode the DutyCycle check will be disabled an the Node will send a LinkCheckRequest for every spreadfactor from SF7 to SF12. After the test the node will show you on which datarates a ACK was received back. The data is also stored on the SD card in GeoJSON format an could be analyzed with [geojson.io]

![SSV Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/ssv-1.jpg "Fig 6. SSV running")
![SSV Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/ssv-2.jpg "Fig 7. SSV results")

#### OTAA 
#### (OverTheAirActivation)
"OTAA" enables the tester to perform OTAA-Joins. By selecting Join the tester will try to join the TTN Network. After an successful the the tester will start with periodic transmissions. You have the choice between transmission with or without ACK. The RSSI and SNR values of the received packet will be shown on the display. If there is no valid GPS fix, a packet can by manually send by pushing button B.

![OTAA Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/otaa.jpg "Fig 7. OTAA")

#### SET 
#### (Settings)

"SET" allows to change the transmission intervall in NACK or ACK mode. Possible settings are 15/30/45/60/120 seconds. Pressing button C will active the powersaving mode. The node will go to light sleep and wakes up every 15 seconds. Sleep mode can only be stopped by resetting the devicde (ToDo).

![SET Image](https://github.com/Bjoerns-TB/M5Stack-LoRaWAN-Network-Tester/blob/master/images/set.jpg "Fig 7. SET")

## Notes
  - The DutyCycle check ist activated except for the SSV mode.
  - If you have a valid GPS fix the GPS track will be written to the SD card as a GPX file.
  - Periodic transmission will only work with a valid GPS fix and an GPS age below 2 seconds
  
## Changelog

  - 09.03.2020
    - Add LowPower mode for LoRaWAN module
    - improve SF selection, ADR, and Duty Cycle check
  
  - 08.03.2020
    - Enforce Duty Cycle check
    - Fix SF selection and ADR for ACK and MAN mode

  - 05.03.2020
    - Fix SF selection and ADR for NACK mode
    - Add OTAA support
    
  - 04.03.2020
    - Move gpsupdate task to core 0
    
  - 03.03.2020
    - improve gspupdate task
    
  - 28.02.2020
    - fix typos
    
  - 27.02.2020
    - switch to M5.Power library

## ToDo
  - ~~improve botton reaction~~
  - ~~cleanup the code~~
  - improve powersave features
  - ~~Add OTAA~~ 
  - inform about DutyCycle restriction on display

[M5Stack]: https://github.com/m5stack/M5Stack
[TinyGPSPlus]: https://github.com/mikalhart/TinyGPSPlus
[NeoPixelBus]: https://github.com/Makuna/NeoPixelBus
[M5_UI]: https://github.com/dsiberia9s/M5_UI
[geojson.io]: http://geojson.io/


