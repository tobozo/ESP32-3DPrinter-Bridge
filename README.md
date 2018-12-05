# ESP32-3DPrinter-Bridge

Play the imperial march on your 3D printer in an excessively complicated fashion.

[![image](https://img.youtube.com/vi/QIOWCP2BMf0/0.jpg)](https://youtu.be/QIOWCP2BMf0)


 Credits:
 ========
 
  - https://github.com/tomorrow56/M5Stack_USB_Host_FTDI_Serial_Monitor
  - https://github.com/AlphaLima/ESP32-Serial-Bridge
  - http://www.circuitsathome.com
  - https://reprap.org/forum/read.php?1,236755
  
  Software Library Requirements:
  ==============================
  - https://github.com/felis/USB_Host_Shield_2.0
   
  Hardware Requirements:
  ======================
  - Mini USB Host Shield
  - ESP32
  
  Wiring:
  =======
<pre>
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
</pre>

  Usage: 
  ======
  - Plug the 3D Printer to the Mini USB Host Shield
  - Power the ESP32
  - From any machine on the network: telnet [IP Address of the ESP32] 1337 to access the bridge
  
