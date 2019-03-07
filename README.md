PHABLABS-POV
===
___
Das hier ist die Software für das POV-Device des PHABLABS 4.0 Projektes, Grundlage ist ein zuvor entwickeltes ESP-Matrixdisplay


Komponenten
-----------
Als Controller wird ein ESP8266 in Form eines WeMos D1 mini verwendet.

Weiterhin sind weitere vorgefertigte Funktionen im ESP denkbar, so dass neben freien Texten und Zeitinformationen auch andere Informationen aus dem Internet geholt und dargestellt werden können.

Es wird empfohlen eine [möglichst aktuelle Arduino-IDE (1.8.x)](https://www.arduino.cc/en/Main/Software) zu verwenden. Bei älteren IDEs kommt es mit den aktuellen ESP-Toolchains zu Problemen.  Die verwendeten asynchronen Bibliotheken benötigen mindestens die Version 2.3 des ESP-Tools, die entweder über den [Boardmanager](https://github.com/esp8266/Arduino#installing-with-boards-manager) oder aber direkt als [GIT-Version] (https://github.com/esp8266/Arduino#using-git-version)installiert werden kann.

Zusätzlich muss der Arduino-IDE noch der [ESP-Uploadmanager hinzugefügt werden](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system). Damit bekommt die IDE unter dem Werkzeuge Menupunkt eine weitere Option: **"ESP8266 Sketch Data Upload"**. 
Mit dem Uploadmanager werden alle Dateien aus einem Unterverzeichnis namens "data" in ein SPIFFS-Dateisystem konvertiert und in den zweiten, 3MB großen, FLASH-Bereich des WeMos D1 mini geladen.

Zum Fehlersuchen ist die Erweiterung [ESP Exception Stack Decoder](https://github.com/me-no-dev/EspExceptionDecoder) hilfreich, mit deren Hilfe festgestellt werden kann, wo etwas schief läuft, wenn beim Debuggen im Terminalfenster ein hexadezimaler Stackdump angezeigt wird.

Bibliotheken
------------
Die folgende Bibliothek aus Github wurde für das Projekt gepatcht. Die gepatchten Bibliothek findet sich im Ordner "libraries" dieses Repositories und muss vor dem Kompilieren in den libraries-Ordner der Arduino-Umgebung verschoben oder kopiert werden. Sie hierzu: [Manuelle Installation von Bibliotheken ](https://www.arduino.cc/en/Guide/Libraries#toc5)

- [**Timelib.h**]( http://github.com/PaulStoffregen/Time)

Bei der Time-Bibliothek kommt es unter nicht case-sensitiven Systemen (Windows, Mac) oft zu Fehlermeldungen, wenn die Bibliothek als ***T**ime.h* eingebunden wird. Zahlreiche andere Bibliotheken bringen Headerdateien in der Schreibweise ***t**ime.h* mit.  Daher ist es sinnvoll diese Bibliothek in ein Verzeichnis *Time**lib*** zu installieren und auch die direkt die *Timelib.h* einzubinden.  Die im Projekt aus Rückwärtskompatibilitätsgründen enthaltene *Time.h* macht nichts anderes!

Neben den vorhandenen Arduino bzw. ESP-Standardbibliotheken werden folgende Bibliotheken aus Github-Repositories ohne Veränderung verwendet. Diese können entweder per "git clone" in das lokale Bibliotheksverzeichnis installiert werden, oder aber als ZIP-Datei heruntergeladen werden und dann [als ZIPFILE installiert werden ](https://www.arduino.cc/en/Guide/Libraries#toc4) 

- [**ESPAsyncUDP.h**]( http://github.com/me-no-dev/ESPAsyncUDP)
- [**ESPAsyncTCP.h**]( http://github.com/me-no-dev/ESPAsyncTCP)
- [**ESPAsyncWebServer.h**]( http://github.com/me-no-dev/ESPAsyncWebServer)
- [**Timezone.h**]( http://github.com/JChristensen/Timezone)

Compilieren und Laden des Programmes
------------------------------------
- Herunterladen des Programmcodes oder Clonen des Github-Repositories
- Installieren der zur beschriebenen Komponenten in die Arduino IDE
  - ESP8266 Toolchain (entweder über den Boardverwalter oder [direkt aus Github](https://arduino-esp8266.readthedocs.io/en/latest/installing.html#using-git-version)
  - Installation des [ESP Sketch Data Upload-Tools](https://github.com/esp8266/arduino-esp8266fs-plugin)
  - Optional: [ESP Exception Stack Decoder](https://github.com/me-no-dev/EspExceptionDecoder) 
  - [**ESPAsyncUDP.h**]( http://github.com/me-no-dev/ESPAsyncUDP)
  - [**ESPAsyncTCP.h**]( http://github.com/me-no-dev/ESPAsyncTCP)
  - [**Timezone.h**]( http://github.com/JChristensen/Timezone)
  - gepatchte Timelib ins Bibliotheksverzeichnis installieren

- Öffnen der Datei 	PHABLABS-POV.ino in der Arduino IDE
- Folgende Einstellungen für das Board vornehmen:
  - ***Lolin (Wemos) D1 R2 & mini***
  - Flash Size: ***4M (3M SPIFFS)***
  - Debug Port: ***Serial***
  - Erase Flash: ***All Flash Contents*** 
  - COM-Port: ***aktuellen COM-Port des angesteckten WeMOS D1***
- Programm Compilieren und auf den Controller laden
- Inhalt des SPIFFS-Dateisystem mit dem Menüpunkt: ***Werkzeuge=> ESP8266 Sketch Data Upload*** auf den Controller laden.
- Für ein erneutes Hochladen des Programmcodes ***Erase Flash*** wieder auf ***Only Sketch*** stellen. Somit bleiben das SPIFFS-Dateisystem und eventuell gespeicherte Konfigurationsdaten erhalten.


Grundidee bzw. weitere Entwicklung
---------------------------------------
Die Grundidee ist ein ständig laufender Webserver auf dem ESP, der sowohl für die Kommunikation mit der Applikation (hier der LED-Laufschrift) als auch für die Konfiguration des WLAN zuständig ist.  Der Schwachpunkt anderer Bibliotheken wie WiFi-Manager ist, dass sich dort der Webserver nach der WLAN-Konfiguration beendet. Zudem wurde Wert darauf gelegt, die asynchronen UDP und TCP Bibliotheken verwenden zu können, um weniger blockierende Kommunikation zu erhalten. 
Die im Paket *ESPAsyncWebServer* enthaltene Funktion des [ACE-Editors](https://ace.c9.io/ "ACE Javascript Editor") wurde soweit abgeändert, dass dieser vollständig im Speicher des ESP abgelegt werden kann, und damit auch im AP Mode zur Verfügung steht. Gleiches sollte für eine Adaption einer graphischen Programmiermöglichkeit dienen, die ESP-Funktionen nach Javascript durchreicht.


**Ansonsten steht noch aus:**
 - **Refactoring:** Zusammenfassen der Webserver- und Konfigurationsfunktionen in eine eigene Bibliothek und Klasse als Basis für beliebige web-basierte ESP-Anwendungen. Desweiteren Kapseln der Matrix- und Zeitfunktionen in ebenfalls 
 - Mehrstufige Benutzerauthentifizierung und -verwaltung (Trennen von Konfiguration und Anwendung)
 - Ausbau der IP-Konfiguration um WPS, feste IP-Adressen und Netzwerkkonfiguration, DHC-Range, etc..
 - Einsatz von https
 - Durchreichen der ESP-Funktionen zum Browser zum Einsatz browserbasierter Programmierung
 - OTA Updates

aktuell implementierte Funktionen
---------------------------------
Derzeit startet der ESP in den AP-Mode und ist über den im Sourcecode hinterlegten SSID-Namen **ESPMATRIX** und das Passwort **ABCdef123456** erreichbar.
Sollte der ESP bereits die Zugangsinformationen zu einem aktuell erreichbaren WLAN gespeichert haben (z.B. weil vorher ein anderer Sketch geladen war), so verbindet er sich dorthin!
Die LED-Matric zeigt in allen Fällen dann die IP-Adresse an, unter der der ESP zu erreichen ist. 

Sobald man mit dem ESP verbunden ist, gibt es diese wichtigen URLs:
- **/content**  Erlaubt Inhalt, Helliugkeit und Geschwindigkeit des Textes einzustellen. Uhrzeit und Datum sind derzeit nur dann aktuell, wenn der ESP sich als Client in einem WLAN befindet und einen NTP-Server erreicht (de.pool.ntp.org).
- **/wificonnectAP** Ermöglicht es dem ESP, sich als Client an ein vorhandenes WLAN zu verbinden.
- **/wificonfigAP** Erlaubt es, SSID und Passwort für den AP Mode, sowie das Passwort für den Administrationsaccount (voreingestellt admin/admin) zu ändern.
- **/wifiRestartAP** Löscht die gespeicherten WLAN Client-Zugangsdaten und startet im AP-Mode
- **/edit** Startet den eingebauten ACE Editor, um die auf dem SPIFFS abgelegten Dateien zu editieren.  Für den Editor is es nun unabdingbar, dass der Inhalt des data-Unterverzeichnisses mittels des oben beschriebenen [ESP-Uploadmanagers](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system) auf des SPIFFS geladen wird. Ansonsten ist keine Anzeige von Webseiten und damit keine Funktion möglich!



