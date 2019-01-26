/* 
*************************************************************
             wifi_scanner.ino
             ================
             Uwe Berger; 2019
 
ODROID-Go (https://wiki.odroid.com/odroid_go/odroid_go)



Have fun!
---------


*************************************************************
*/


#include <odroid_go.h>
#include "WiFi.h"


// *************************************************************
String encryptiontyp2str(uint8_t e)
{
  switch (e) {
    case 5:
      return "WEP";
      break;
    case 2:
      return "WPA";
      break;
    case 4:
      return "WPA2";
      break;
    case 7:
      return "None";
      break;
    case 8:
      return "Auto";
      break;
    default:
      return "Unknown";
      break;
  }
}


// *************************************************************
void setup()
{
    //Serial.begin(115200);
    GO.begin();
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
}

// *************************************************************
// *************************************************************
// *************************************************************
void loop()
{
	String tstr;
	char buf[100];
	char sbuf[50];
    // WiFi.scanNetworks gibt Anzahl der gefundenen WLANs zurueck
    int n = WiFi.scanNetworks();
    GO.lcd.clearDisplay();
    GO.lcd.setCursor(0, 0);
    if (n == 0) {
        GO.lcd.println("no networks found");
    } else {
		// wieviele WLANs gefunden? --> ausgeben...
        sprintf(buf, "%i networks found (SSID, Encrypt., ...):", n);
   		GO.lcd.println(buf);
   		// zu jedem gefundenen WLAN relevante Infos ausgeben
        for (int i = 0; i < n; ++i) {
			strcpy(buf, "");
			tstr = WiFi.SSID(i);
			tstr.toCharArray(sbuf, sizeof(sbuf));
			sprintf(buf, "   %s, ", sbuf);
			tstr = encryptiontyp2str((WiFi.encryptionType(i)));
			tstr.toCharArray(sbuf, sizeof(sbuf));
			sprintf(buf, "%s%s (%i), %idBm", buf, sbuf, WiFi.encryptionType(i), WiFi.RSSI(i));
    		GO.lcd.println(buf);
        }
    }
    // alle 5s...
    delay(5000);
}
