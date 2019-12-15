# Datarate-Client-Server-App

1. Ziel der Benchmark-Anwendung

Um die verfügbare Datenrate der Haupt- und Ersatzstrecke/n zu überprüfen wird eine Anwendung programmiert. 
Das Programm wird in der Programmiersprache C erstellt. Als Betriebssystem wird Fedora 29 verwendet. Als Entwicklungsumgebung dient ein Standard-Texteditor wie emacs, gedit oder nano. Zur Kompilierung des Programms wird der C-Compiler gcc der GNU Compiler Collection verwendet.
Da es sich um eine Client-Server-Anwendung handelt, wird ein VPS (virtual private server) mit öffentlicher IP-Adresse verwendet um das Programm zu testen. Dabei läuft auf dem VPS die Serveranwendung und auf einem PC das Client-Programm. Durch das Versenden von Pakten vom Client an den Server und die anschließende Quittierung des Servers soll die Datenrate der Verbindung getestet werden.

2. Aufbau der Server-Anwendung

Die Serveranwendung erstellt einen AF_INET Stream-Socket. Dieser Socket wird an den Port 7777 gebunden. Anschließend wartet die Anwendung auf eingehende Anfragen von Clients (listen). In einer Dauerschleife wird auf neue Verbindungen geprüft. Sobald eine neue Verbindung vom Server akzeptiert wird, wird für diese ein neuer Socket erstellt und ein Child-Prozess gestartet. Der Elternprozess wartet weiter auf neue Anfragen, während der Child-Prozess die Anfrage des Clients bearbeitet. So wird sichergestellt, dass neue Anfragen von anderen Clients zu jeder Zeit entgegengenommen werden können, auch wenn der Server gerade eine andere Anfrage bearbeitet. 
Im Child-Prozess wird ein Puffer erstellt, der die vom Client gesendeten Daten zwischenspeichert. Dem Server ist die Anzahl der vom Client gesendeten Bytes eines einzelnen Blocks über die Makrovariable „BLOCK_SIZE“ bekannt. Für jeden empfangenen Block wird eine Zählervariable inkrementiert. Daraufhin wird die Zählervariable an den Client gesendet um den Empfang zu quittieren. Sobald der Client aufhört weitere Blöcke zu senden wird der Child-Prozess der Serveranwendung beendet. 

3. Aufgabe der Client-Anwendung

Der Client hat die Aufgabe in bestimmten Intervallen für eine definierte Zeitspanne Datenpakete an den Server zu senden. Anhand der Quittierung des Servers kann über die Anzahl der gesendeten Blocke und der festgelegten Blockgröße die Gesamtmenge der übertragenen Bytes berechnet werden. Wird die Gesamtanzahl an übertragenen Bytes durch die Dauer der Übertragung dividiert, dann kann die Datenrate in Bytes pro Sekunde berechnet werden. Über eine Makrovariable ist die benötigte Mindestdatenrate angegeben. Sobald diese Datenrate nicht mehr erreicht wird, wird der Benutzer gefragt, ob die Datenrate der anderen Gateways, die die Ersatzstrecken und/oder -median repräsentieren, getestet werden sollen. 

4. Ablauf der Client-Anwendung
Beim Programmstart müssen die IP-Adressen der verfügbaren Gateways angegeben werden. Die Eingabe der IP-Adressen wird in einer History-Datei gespeichert. Sollten keine Gateways-IP-Adressen angegeben werden, dann versucht das Programm auf die History-Datei zuzugreifen und die IP-Adressen von dort auszulesen. Ist keine History-Datei verfügbar, dann brich das Programm mit einer Fehlermeldung ab, die dem Benutzer mitteilt, dass dieser die Gateway-IP-Adressen angeben muss. 
Sobald das Programm startet wird in definierten Intervallen durch das Senden von Blöcken an die Serveranwendung die Datenrate berechnet. Über eine Schleife wird dieser Vorgang solange wiederholt, bis die benötigte Datenraten nicht erreicht wird. Tritt dieser Fall ein, dann wird der Benutzer gefragt, ob er die anderen Gateways testen möchte. Wird dies verneint, dann wird das Programm abgebrochen. Bestätigt der Benutzer, dann wird ein Gateway nach dem anderen getestet. Im Rahmen des Tests der Datenrate aller Gateways werden die verfügbaren Datenraten ausgegeben. Am Ende des Tests wird außerdem das Gateway mit der höchsten Datenrate ausgegeben. Der Benutzer wird aufgefordert eine Gateway-ID einzugeben um das Standard-Gateway dementsprechend zu ändern. Nachdem der Benutzer die ID eingegeben hat springt das Programm zum Beginn des Programms, in die Schleife, die die Datenrate wieder solange überprüft bis die Mindestdatenrate nicht erreicht wird.

5. Bestandteile der Client-Anwendung
Neben der main-Funktion, die die anderen Funktionen koordiniert und den Programmablauf steuert, existieren folgende Funktionen und Makrovariablen:

<code>
#define SERVER_IP "127.0.0.1" // IP-Adresse des Servers
#define SERVER_PORT "7777" // Port der Serveranwendung
#define BLOCK_SIZE 102400 // 100 KiB
#define CHECK_INTERVAL 10 // Sekunden des Testintervalls
#define TEST_DURATION 3 // Übertragungsdauer eines Tests in Sekunden
#define MIN_DATARATE 2.5 // benötigte Mindestdatenrate in MiB/s
#define COMMAND_HISTORY_FILE "history.txt" // Dateiname der History-Datei
</code>

<code>double getDatarateInMiBPerSecond()</code>
Diese Funktion ist für das Senden der Blöcke an die Serveranwendung und die anschließende Berechung der verfügbaren Datenrate verantwortlich.  Es wird ein Socket erstellt, das mit dem Socket der Serveranwendung verbunden wird. Dann werden für eine definierte Zeitspanne Blöcke an den Server gesendet und die Quittierung, die den serverseitigen Zähler beinhaltet, entgegenommen. Sobald die Dauer des Übertragungstests erreicht ist wird das Senden eingestellt. Anhand der lezten Antwort des Servers, die die Gesamtanzahl der übertragenen Blöcke beinhaltet, wird zusammen mit der definierten Blockgröße die Anzahl der übertragenen Bytes berechnet. Über die Gesamtanzahl der übertragenen Byte wird dann zusammen mit der Übertragungsdauer die Datenrate berechnet. Dieser Wert wird von der Funktion zurückgegeben. 

<code>int changeGateway(char ipAddress[])</code>
Diese Funktion wird verwendet um das Standard-Gateway des Clients zu ändern. Sie kommt zum Einsatz wenn die benötigte Datenrate nicht mehr erreicht wird und der Benutzer das Testen aller Gateways wünscht. Nach dem Test aller verfügbaren Gateways kommt die Funktion nochmals zum Einsatz, nachdem der User angegeben hat, welches Gateway für die zukünftige Übertragung verwendet werden soll. Als Parameter wird die IP-Adresse des gewünschten Gateways erwartet. In der Funktion wird ein Systemcall durchgeführt, der das Kommando zum Ändern des Standard-Gateways ausführt. Dieses Kommando ist vom jeweiligen Betriebssystem abhängig und muss vor dem Kompilieren angepasst werden.
<code>
/* command is linux dependent */
car command[] = "ip route change default via "; // + gateway ip 	address
/* command for windows*/
//char command[] = "route change 0.0.0.0 mask 0.0.0.0 "; // + gateway 	ip address
</code>

Beim Entwickeln des Programms wurde vor allem auf Betriebssystemunabhängigkeit geachtet. Diese Zeile ist die einzige, die vom jeweiligen Betriebssystem abhängig ist. 

<code>void testAllGateways(char** gatewayList, int numGateways)</code>
Diese Funktion führt den Test aller Gateways durch. Dabei wird über die Liste aller angegeben bzw. aus der History-Datei ausgelesenen Menge aller verfügbaren Gateways iteriert. Nach dem Test der Datenrate wird das jeweilige Testergebnis mit dem Testergebnis des bis dahin schnellsten Gateways verglichen. Verfügt das getestete Gateway über eine höhere Datenrate, dann wird es als neues schnellstes Gateway gespeichert. Während des Tests werden alle Testergebnisse ausgegeben, nach dem Test wird außerdem das Gateway mit der höchsten Datenrate nochmals explizit genannt. 

<code>char **getCommandHistory()</code>
Diese Funktion wird ausgeführt, wenn beim Programmstart keine Gateway-IP-Adressen angegeben werden. Dann wird versucht, die IP-Adressen aus einer lokalen History-Datei einzulesen, die die IP-Adressen, die beim letzten Programmstart angegeben wurden, beinhaltet. Ist diese Datei nicht verfügbar, dann wird das Programm mit einer Fehlermeldung abgebrochen, die den Benutzer auffordert die IP-Adressen beim nächsten Programmstart anzugeben. Ansonsten werden die gefunden IP-Adressen zurückgegeben. 
