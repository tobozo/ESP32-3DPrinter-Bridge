/*

  ESP32-3DPrinter-Bridge - A network <=> ESP32 <=> USB (FTDI) <=> 3D printer bridge
  Source: https://github.com/tobozo/ESP32-3DPrinter-Bridge
 
  MIT License

  Copyright (c) 2018 tobozo

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  -----------------------------------------------------------------------------


  Credits:

    - https://github.com/tomorrow56/M5Stack_USB_Host_FTDI_Serial_Monitor
    - https://github.com/AlphaLima/ESP32-Serial-Bridge
    - http://www.circuitsathome.com
    - https://reprap.org/forum/read.php?1,236755


  Software Library Requirements:

    - https://github.com/felis/USB_Host_Shield_2.0


  Hardware Requirements:

    - Mini USB Host Shield
    - ESP32


  Wiring:

   USBHOST  |  ESP32
   ------------------
       INT  |  17
       GND  |  GND
       RST  |  EN
        SS  |  05
      MOSI  |  23
      MISO  |  19
       SCK  |  18
       3V3  |  3V3
       GND  |  GND


  Usage: 

    - Plug the 3D Printer to the Mini USB Host Shield
    - Power the ESP32
    - From any machine on the network: telnet [IP Address of the ESP32] 1337 to access the bridge


 */


//#define DEBUG_USB_HOST
//#define BRIDGE_DEBUG

#include "cdcftdimod.h" // https://github.com/tomorrow56/M5Stack_USB_Host_FTDI_Serial_Monitor
#include <usbhub.h>
#include "pgmstrings.h"
#include "gcode.h" // https://reprap.org/forum/read.php?1,236755
#include "descriptor.h" // USB FTDIMod instance / tools for this sketch

#include <WiFi.h>
#include <SPI.h>
#include <ArduinoOTA.h> 

WiFiServer TCPServer( 1337 ); // port number to speak to the printer
WiFiClient TCPClient;

// For the byte we read from the serial port
byte data = 0;
bool detected = false;
bool async = false;
String SerialInput = "";


void setup() {

  WiFi.mode(WIFI_STA);
  //WiFi.begin("my-WiFi-SSID", "My-WiFi-Password");
  WiFi.begin();
  uint8_t ticks_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    ticks_count++;
    delay(500);
    Serial.print(".");
    if(ticks_count>10) {
      ESP.restart();
    }
  }

  // OTA Handler
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  ArduinoOTA.begin();

  // Start TCP Port
  TCPServer.begin(); // start TCP server 
  TCPServer.setNoDelay(true);
  //keepalive = millis();

  Serial.begin( 115200 );
  Serial.println("Booted");
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("GW IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
  
  if (Usb.Init() == -1){
    Serial.println("OSC did not start.");
  } else {
    Serial.println("Started USB :-)");
  }
}



void broadcastPrintln(String str) {
  Serial.println(str);
  if(TCPClient) {
    TCPClient.println(str);
  }  
}
void broadcastPrint(char chr) {
  Serial.print(chr);
  if(TCPClient) {
    TCPClient.write(chr);
  }  
}



void loop() {
  ArduinoOTA.handle();
  Usb.Task();

  if(TCPServer.hasClient()) { // handle TCP client connect / disconnect
    if (!TCPClient || !TCPClient.connected()){
      if(TCPClient) TCPClient.stop();
      TCPClient = TCPServer.available();
      broadcastPrintln("New TCP client for COM"); 
      //continue;
    }
    //no free/disconnected spot so reject
    WiFiClient TmpserverClient = TCPServer.available();
    TmpserverClient.stop();
  }

  if(TCPClient.available()) { // read TCP input
    SerialInput += TCPClient.readStringUntil('\r\n');
    SerialInput.trim();
    #ifdef BRIDGE_DEBUG
    if(SerialInput!="") {
      broadcastPrintln("TCP: "+ SerialInput);
    }
    #endif
  }

  if(Serial.available()) { // read Serial input (use it for debug)
    SerialInput += Serial.readStringUntil('\r\n');
    SerialInput.trim();
    #ifdef BRIDGE_DEBUG
    if(SerialInput!="") {
      broadcastPrintln("SERIAL: "+ SerialInput);
    }
    #endif
  }


  if(SerialInput=="async") { // handle external commands
    async = !async;
    if(async) {
      broadcastPrintln("now async (will write to SD)");
    } else {
      broadcastPrintln("now sync (will commit realtime)");
    }
  }

  if(SerialInput=="restart") { // handle external commands
    ESP.restart();
  }
  
  if(SerialInput=="imperialmarch") { // handle external commands
    if(async) {
      broadcastPrintln("Will write on the SD, please be patient");
      SerialInput = "M28 blah.g";
      nextnote = millis() + 1000;
    }
    playing = true;

  }

  if(SerialInput=="status") { // handle external commands
    SerialInput = "";
    if( Usb.getUsbTaskState() == USB_STATE_RUNNING ) {
      broadcastPrintln("USB Running");
      Usb.ForEachUsbDevice(&PrintAllDescriptors);
      Usb.ForEachUsbDevice(&PrintAllAddresses);
      detected = true;
    } else {
      broadcastPrintln("USB NOT Running");
    }
  }

  if( Usb.getUsbTaskState() == USB_STATE_RUNNING ) { // handle usb tasks
    uint8_t  rcode;
    uint8_t  buf[64];
    uint16_t rcvd = 64;
    
    if(!detected) { // show device information on detection and play a beep
      Usb.ForEachUsbDevice(&PrintAllDescriptors);
      Usb.ForEachUsbDevice(&PrintAllAddresses);
      detected = true;
      // char strbuf[] = "M31\r\n";
      char strbuf[] = "M300 S100 P200\r\nM300 S200 P300\r\nM300 S300 P436\r\n";
      rcode = Ftdi.SndData(strlen(strbuf), (uint8_t*)strbuf);
      if (rcode){
        ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
      } else {
        broadcastPrintln("Handshake OK");
      }
      delay(50);
    }

    if(playing) {
      if(nextnote<millis()) {
        SerialInput = imperialMarchAsString(async);
        return;
      }
    }

    if(SerialInput!="") { // forward collected input to USB
      SerialInput += "\r\n";
      rcode = Ftdi.SndData(strlen(SerialInput.c_str()), (uint8_t*)SerialInput.c_str());
      if (rcode){
        ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
      } else {
        #ifdef BRIDGE_DEBUG
        broadcastPrintln("SENT: "+  SerialInput);
        #endif
      }
      SerialInput = "";
    }

    // collect data received from USB, if any
    for (uint8_t i=0; i<64; i++){
      buf[i] = 0;
    }
    rcvd = 64;
    rcode = Ftdi.RcvData(&rcvd, buf);

    if (rcode && rcode != hrNAK){
      ErrorMessage<uint8_t>(PSTR("Ret"), rcode);
    }

    if( rcvd > 2 ) { //more than 2 bytes received
      for(uint16_t i = 2; i < rcvd; i++ ) {
        broadcastPrint((char)buf[i]);
        data = buf[i];
        if (data > 31 && data < 128) { /* printable char */  }
        if (data == '\t') { /* tab */ }
        if (data == '\r') { /* end of line */ }
      }
    }
  }
}
