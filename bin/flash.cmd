@echo off
e:
cd E:\fablab\PHABLABS-POV\bin
"D:\arduino-1.8.5\hardware\esp8266com\esp8266/tools/esptool/esptool.exe" -vv -cd nodemcu -cb 921600 -cp COM31 -ca 0x00000000 -cf PHABLABS-POV.ino.bin
"D:\arduino-1.8.5\hardware\esp8266com\esp8266/tools/esptool/esptool.exe" -vv -cd nodemcu -cb 921600 -cp COM31 -ca 0x00300000 -cf PHABLABS-POV.spiffs.bin
