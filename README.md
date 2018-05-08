PHABLABS-POV
===
___
Das hier gibt die Software für das POV-Device des PHABLABS 4.0 Projektes, Grundlage ist ein zuvor entwickeltes ESP-Matrixdisplay


Komponenten
-----------
Als Controller wird ein ESP8266 in Form eines WeMos D1 mini verwendet.

Weiterhin sind weitere vorgefertigte Funktionen im ESP denkbar, so dass neben freien Texten und Zeitinformationen auch andere Informationen aus dem Internet geholt und dargestellt werden können.

Es wird empfohlen eine möglichst aktuelle Arduino-IDE (mindestens 1.8.3) zu verwenden. Bei älteren IDEs kommt es mit den aktuellen ESP-Toolchains zu Problemen.  Die verwendeten asynchronen Bibliotheken benötigen mindestens die Version 2.3 des ESP-Tools, die entweder über den [Boardmanager](https://github.com/esp8266/Arduino#installing-with-boards-manager) oder aber direkt als [GIT-Version] (https://github.com/esp8266/Arduino#using-git-version)installiert werden kann.

Zusätzlich muss der Arduino-IDE noch der [ESP-Uploadmanager hinzugefügt werden](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system "Uploading files to SPIFFS"). Damit bekommt die IDE unter dem Werkzeuge Menupunkt eine weitere Option: **"ESP8266 Sketch Data Upload"**. 
Mit dem Uploadmanager werden alle Dateien aus einem Unterverzeichnis namens "data" in ein SPIFFS-Dateisystem konvertiert und in den zweiten, 3MB großen, FLASH-Bereich des WeMos D1 mini geladen.

Zum Fehlersuchen ist die Erweiterung [ESP Exception Stack Decoder](https://github.com/me-no-dev/EspExceptionDecoder) hilfreich, mit deren Hilfe festgestellt werden kann, wo etwas schief läuft, wenn beim Debuggen im Terminalfenster ein hexadezimaler Stackdump angezeigt wird.

Bibliotheken
------------
Verwendung finden die folgenden Bibliotheken aus Github, die für das Projekt gepatcht wurden. Diese gepatchten Bibliotheken finden sich im Ordner "libraries" dieses Repositories und müssen vor dem Kompilieren in den libraries-Ordner der Arduino-UMgebung verschoben werden.

- [**ESPAsyncWebServer.h**]( http://github.com/me-no-dev/ESPAsyncWebServer "Asynchroner Webserver")
- [**Timelib.h**]( http://github.com/PaulStoffregen/Time "Timelib")

Bei der Time-Bibliothek kommt es unter nicht case-sensitiven Systemen (Windows, Mac) oft zu Fehlermeldungen, wenn die Bibliothek als ***T**ime.h* eingebunden wird. Zahlreiche andere Bibliotheken bringen Headerdateien in der Schreibweise ***t**ime.h* mit.  Daher ist es sinnvoll diese Bibliothek in ein Verzeichnis *Time**lib*** zu installieren und auch die direkt die *Timelib.h* einzubinden.  Die im Projekt aus Rückwärtskompatibilitätsgründen enthaltene *Time.h* macht nichts anderes!

Neben den vorhandenen Arduino bzw. ESP-Standardbibliotheken werden folgende Bibliotheken aus Github-Repositories ohne Veränderung verwendet:

- [**ESPAsyncUDP.h**]( http://github.com/me-no-dev/ESPAsyncUDP "Asynchrones UDP")
- [**ESPAsyncTCP.h**]( http://github.com/me-no-dev/ESPAsyncTCP "Asynchrones TCP")
- [**Timezone.h**]( http://github.com/JChristensen/Timezone "Timezone")
- [**Max72xxPanel.h**]( https://github.com/markruys/arduino-Max72xxPanel.git "Max72xx Paneltreiber")


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
Sollte der ESP sich nicht programmieren lassen, oder sollte es Probleme beim Verbinden mit WLANs geben, so ist es ratsam, den ESP zunächst komplett zu löschen. Dies geschieht mit dem esptool.py:
```Shell
python path/to/esptool.py --port COMPORTNAME erase_flash
```
Dies kann auch bei neu gekauften Modulen notwendig sein, da diese manchmal mit inkompatiblem Aufbau der Speicherstrukturen zur Arduino IDE ausgeliefert werden. Die Arduino-IDE schreibt nur in der ersten Speicherblock und geht dabei [von einem Standardlayout aus](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#flash-layout "Flash Layout")!
Das ist einerseits sinnvoll, weil so keine Inhalte der anderen Speicherbereiche angerührt werden, kann aber zu Problemen führen, wenn diese Bereich nicht so liegen bzw. gefüllt sind, wie dies die IDE oder der Sketch erwartet!
Sobald man mit dem ESP verbunden ist, gibt es diese wichtigen URLs:
- **/content**  Erlaubt Inhalt, Helliugkeit und Geschwindigkeit des Textes einzustellen. Uhrzeit und Datum sind derzeit nur dann aktuell, wenn der ESP sich als Client in einem WLAN befindet und einen NTP-Server erreicht (de.pool.ntp.org).
- **/wificonnectAP** Ermöglicht es dem ESP, sich als Client an ein vorhandenes WLAN zu verbinden.
- **/wificonfigAP** Erlaubt es, SSID und Passwort für den AP Mode, sowie das Passwort für den Administrationsaccount (voreingestellt admin/admin) zu ändern.
- **/wifiRestartAP** Löscht die gespeicherten WLAN Client-Zugangsdaten und startet im AP-Mode
- **/edit** Startet den eingebauten ACE Editor, um die auf dem SPIFFS abgelegten Dateien zu editieren.  Für den Editor is es nun unabdingbar, dass der Inhalt des data-Unterverzeichnisses mittels des oben beschriebenen [ESP-Uploadmanagers](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system "ESP-Uploadmanager") auf des SPIFFS geladen wird. Ansonsten werden die Javascriptdateien nicht gefunden!

Verdrahtung
-----------
Es ist wichtig, dem ESP einen möglichst großen Stützkondensator zur Seite zu stellen.  Je nach USB-Port kommt es sonst zu Fehlverhalten beim Programmieren.  Sollten mehr als 4 LED-Module in Reihe geschaltet werden, so ist dafür zu sorgen, dass die LED-Module eine vom Programmierport unabhängige Spannungsversorgung erhalten.
Ansonsten werden die folgenden PINs miteinander verbunden:

|Wemos D1 mini|LED-Matrix|
|-------------|----------|
| D5 (14/CLK) | CLK|
| D6 (12/MISO)| CS |
| D7 (13/MOSI)| DIN|



