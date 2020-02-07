
#define TIMER_INTERRUPT_DEBUG      1
#include "ESP32TimerInterrupt.h"

#include <ezTime.h>

#include <odroid_go.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>



#include <WiFi.h>
#include <ArduinoJson.h>
#include "MpdClient.h"


#define CONFIG_FILE "/config/mpdclient.cfg"

// WLAN-Konfiguration (Default)
#define WLAN_SSID	"ssid"
#define WLAN_PWD	"pwd"
#define HOSTNAME	"odroid-hostname"

// MPD-Konfiguration (Default)
#define MPD_HOST "localhost"
#define MPD_PORT 6600
#define MPD_PWD  ""

// Zeitzone
#define MY_TIME_ZONE "Europe/Berlin"


char ssid[30] 	  = WLAN_SSID;
char password[30] = WLAN_PWD;
char hostname[30] = HOSTNAME;

char mpd_host[30]     = MPD_HOST;
unsigned int mpd_port = MPD_PORT;
char mpd_pwd[30]      = MPD_PWD;

File cfg_file;

unsigned long counter=0;

MpdClient mpdClient;


uint8_t display_playlists_active = 0;
uint8_t display_db_active        = 0;


// Instantiate ESP32 timer0
#define TIMER0_INTERVAL_MS        1000 
ESP32Timer Timer0(0);

Timezone MyZone;
char my_time_zone[50] = MY_TIME_ZONE;

#define RESISTANCE_NUM    2
#define DEFAULT_VREF      1100
#define NO_OF_SAMPLES     64
static esp_adc_cal_characteristics_t adc_chars;


// **************************************************************
void load_config(void)
{
  char buf[1024] = "";
  char str_i[10] = "";

  // Konfigurationsdatei oeffnen
  cfg_file = SD.open(CONFIG_FILE);
  // Datei da?
  if (cfg_file) {
	 // Datei einlesen und weiter
	 cfg_file.readBytes(buf, sizeof(buf));
  } else {
    // Datei nicht da, dann Ende (...Default-Werte bleiben erhalten)
	return;  
  }
  // JSON-String analysieren und Werte entsprechend setzen
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(buf);
  // wenn Fileinhalt kein JSON, dann Ende (...Default-Werte bleiben erhalten)
  if (!root.success()) return;
  // Werte auslesen/setzen
  strlcpy(ssid, root["wifi_ssid"], sizeof(ssid));
  strlcpy(password, root["wifi_pwd"], sizeof(password));
  strlcpy(hostname, root["hostname"], sizeof(hostname));
  strlcpy(mpd_host, root["mpd_host"], sizeof(mpd_host));
  strlcpy(str_i, root["mpd_port"], sizeof(str_i));
  mpd_port=atoi(str_i);
  strlcpy(mpd_pwd, root["mpd_pwd"], sizeof(mpd_pwd));
  strlcpy(my_time_zone, root["my_time_zone"], sizeof(my_time_zone));

  cfg_file.close();
}

// **************************************************************
void print_config(void)
{
  GO.lcd.println("Configuration:");
  GO.lcd.print("SSID....: "); GO.lcd.println(ssid);
  GO.lcd.print("Pwd.....: "); GO.lcd.println(password);
  GO.lcd.print("Hostname: "); GO.lcd.println(hostname);
  GO.lcd.print("MPD-Host: "); GO.lcd.println(mpd_host);
  GO.lcd.print("MPD-Port: "); GO.lcd.println(mpd_port);
  GO.lcd.print("MPD-Pwd.: "); GO.lcd.println(mpd_pwd);
  GO.lcd.print("Timezone: "); GO.lcd.println(my_time_zone);
  GO.lcd.println("");
}

// **************************************************************
void wifi_connect()
{
  // ins WIFI anmelden 
  WiFi.begin(ssid, password);
  WiFi.setHostname(hostname);
  GO.lcd.print("Connect to WiFi: ") ;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    GO.lcd.print(".");
  }
  GO.lcd.println("");
  GO.lcd.println("WiFi connected!");
  GO.lcd.println(WiFi.localIP());
  GO.lcd.println("");
}

// **************************************************************
void mpd_connect()
{
  GO.lcd.print("Connect to MPD: ");
  boolean connectionEstablished = false;
  while(!connectionEstablished){
    if (mpdClient.connect(mpd_host, mpd_port)) {
      GO.lcd.println("");
      GO.lcd.println("MPD connected!");
       // No password needed...
       connectionEstablished = true;
    } else {
      GO.lcd.print(".");
    }
  }
  mpdClient.startIdle();
}

// ************************************************
void split_text_debug(char *t)
{
	char * pch;
  char buf[50];
  GO.lcd.setCursor(0,0);
	pch = strtok (t," ");
  	while (pch != NULL)  {
      sprintf(buf, "%s(%i)", pch, strlen(pch));
      GO.lcd.println(buf);
      pch = strtok (NULL, " ");
  	}
}

// ************************************************
uint16_t display_split_text(char *t, uint16_t x, uint16_t y, uint16_t dx, uint8_t font)
{
	char * pch;
  char txt[100];
	int px, py;
  int fdx = GO.lcd.textWidth("A", font);
  int fdy = GO.lcd.fontHeight(font);

	px = x;
	py = y;
	// Text Wortweise splitten
  strncpy(txt, t, sizeof(txt)-1);
	pch = strtok (txt,",;_- ");
  	while (pch != NULL)  {
      if ((px + strlen(pch)*fdx) >= (x + dx)) {
        px = x;
        py = py + fdy + 2;
      }
      GO.lcd.setCursor(px, py);
      GO.lcd.print(pch);
      px = px + (strlen(pch)+1)*fdx;
      pch = strtok (NULL, ",;_- ");
  	}
	return (py + fdy + 5);
}

// **************************************************************
uint16_t display_entry(uint16_t px, uint16_t py, int label_color, char* label, int text_color, char* text)
{
  uint16_t y = py;
  GO.lcd.setCursor(0, y);
  GO.lcd.setTextColor(label_color);
  GO.lcd.setTextSize(1);
  GO.lcd.print(label);
  y = y + GO.lcd.fontHeight(1) + 3;
  GO.lcd.setTextColor(text_color);
  GO.lcd.setTextSize(2);
  y = display_split_text(text, 0, y, 320, 1);
  return y + 4;
}

// **********************************************************
double readBatteryVoltage() {
    uint32_t adc_reading = 0;
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t) ADC1_CHANNEL_0);
    }
    adc_reading /= NO_OF_SAMPLES;

    return (double) esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars) * RESISTANCE_NUM / 1000;
}

// **************************************************************
void display_songinfo()
{
  //GO.lcd.clearDisplay();
  // clear Songinfo
  GO.lcd.fillRect(0, 20, 320, 200, BLACK);
  GO.lcd.setTextWrap(false);
  // songinfo
  MpdResponse* response = mpdClient.currentSong();
  char* artist = response->getResponseValue("Artist");
  char* album = response->getResponseValue("Album");
  char* title = response->getResponseValue("Title");
  char* name = response->getResponseValue("Name");
  char* date = response->getResponseValue("Date");

  // split_text_debug(title);

  uint16_t y = 30;
  if (artist != NULL) y = display_entry(0, y, BLUE, "Artist", WHITE, artist);
  if (album  != NULL) y = display_entry(0, y, BLUE, "Album",  WHITE, album);
  if (title  != NULL) y = display_entry(0, y, BLUE, "Title",  WHITE, title);
  if (date   != NULL) y = display_entry(0, y, BLUE, "Year",   WHITE, date);
  if (name   != NULL) y = display_entry(0, y, BLUE, "Name",   WHITE, name);


}

// **************************************************************
void IRAM_ATTR display_odroid_status(void)
{
	char buf[50];
	static int min = -1;
	
    // zu jeder "neuen" Minute Ausgabe aktualisieren...
    if (min != minute()) {
		min = minute();
		// Debugausgabe...
		//sprintf(buf, "--> %i", minute());
		//Serial.println(buf);	
		// Displayausgabe...
		GO.lcd.fillRect(0, 0, 320, 20, DARKGREY);
		GO.lcd.setTextSize(2);
		GO.lcd.setTextColor(LIGHTGREY);
		// ...Uhrzeit
		GO.lcd.setCursor(10, 3);
		GO.lcd.print(MyZone.dateTime("d.m.Y H:i"));
		// ...Akku-Spannung
		GO.lcd.setCursor(250, 3);
		GO.lcd.printf("%1.2lfV \n", readBatteryVoltage());
	}
}

// **************************************************************
void display_mpd_status()
{
    // Status MPD
    MpdResponse* response = mpdClient.status();
    char* state = response->getResponseValue("state");
    char* repeat = response->getResponseValue("repeat");
    char* random = response->getResponseValue("random");
    if (state   != NULL) {
      char buf[50];
      sprintf(buf, "%s | rep.=%s | rand.=%s", state, repeat, random);
      GO.lcd.fillRect(0, 220, 320, 20, DARKGREEN);
      GO.lcd.setCursor(10, 222);
      GO.lcd.setTextSize(2);
      GO.lcd.setTextColor(WHITE);
      GO.lcd.print(buf);
    }
}

// **************************************************************
void draw_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h, char * title, uint32_t title_bg)
{
  GO.lcd.fillRect(x, y, w, h, BLACK);
  GO.lcd.drawRect(x, y, w, h, WHITE);
  GO.lcd.fillRect(x+1, y+1, w-2,  25, title_bg);
  GO.lcd.setCursor(x+7, y+7);
  GO.lcd.setTextSize(2);
  GO.lcd.setTextColor(WHITE);
  GO.lcd.print(title);
}


// **************************************************************
void display_playlists() {
  draw_window(10, 20, 300, 100, "Playlists", RED);
  if (mpdClient.playlists_is_empty() != true) {
    Serial.println(mpdClient.playlists_get_data());
    display_split_text(mpdClient.playlists_get_data(), 16, 60, 288, 1);
  } else {
    GO.lcd.setCursor(16, 60);
    GO.lcd.print("No playlists found!");
  }
}

// **************************************************************
void display_db() {
  draw_window(10, 20, 300, 150, "Database", RED);
  // GO.lcd.setCursor(16, 60);
  // GO.lcd.print("Is coming!");
  if (mpdClient.mpd_db_is_empty() != true) {
    Serial.println(mpdClient.mpd_db_get_data());
    display_split_text(mpdClient.mpd_db_get_data(), 16, 60, 288, 1);
    // Type
    GO.lcd.setCursor(16, 150);
    switch (mpdClient.mpd_db_get_data_type())
      {
        case ENTRY_TYP_DIRECTORY:
          GO.lcd.print(">");
          break;
        case ENTRY_TYP_FILE:
          GO.lcd.print("-");
          break;
        default:
          break;
      }
  } else {
    GO.lcd.setCursor(16, 60);
    GO.lcd.print("No mpdDB found!");
  }
}

// **************************************************************
void button_loop(void) 
{
  MpdResponse * response;
  static uint8_t BtnSelect_pressed = 0;
  int i;
  char actual_directory[2048] = "";
  char prev_directory[2048] = "";
  
  uint8_t found = 0;
  char buf[100];
  //  
  // Buttons verarzten...
  //

  if (GO.JOY_Y.wasAxisPressed() == 2) {
    Serial.println("Y-Axis=2");
    if (display_playlists_active) {
      // Hinzufuegen...
      mpdClient.add(mpdClient.playlists_get_data());
    } else {
      if (display_db_active) {
        // Variablen setzen
        strncpy(actual_directory, mpdClient.mpd_db_get_data(), sizeof(actual_directory));
        i=strlen(actual_directory);
        while(i>=0) {
          if (actual_directory[i] == '/') {
            actual_directory[i]='\0';
            break;
          } else {
            actual_directory[i]='\0';
          }
          i--;
        }
        Serial.print("actual_directory: "); Serial.println(actual_directory);
        strncpy(prev_directory, actual_directory, sizeof(prev_directory));
        i=strlen(prev_directory);
        while(i>=0) {
          if (prev_directory[i] == '/') {
            prev_directory[i]='\0';
            break;
          } else {
            prev_directory[i]='\0';
          }
          i--;
        }
        Serial.print("prev_directory: "); Serial.println(prev_directory);
        // neue Liste holen, positionieren, anzeigen
        if (mpdClient.get_mpd_db(prev_directory)) {
            // mpdClient.mpd_db_search_str(actual_directory);
            found=0;
            if (mpdClient.mpd_db_set_pos_head()) {
              strncpy(buf, mpdClient.mpd_db_get_data(), sizeof(buf));
              Serial.println(buf);
              if (strcmp(mpdClient.mpd_db_get_data(), actual_directory) == 0) found = 1;
              while (!found && mpdClient.mpd_db_set_pos_next()) {
                strncpy(buf, mpdClient.mpd_db_get_data(), sizeof(buf));
                Serial.println(buf);
                if (strcmp(mpdClient.mpd_db_get_data(), actual_directory) == 0) found = 1;
              }
            }
            display_db();
          }
      } else {
        // ...
      }
    }
  }
  
  if (GO.JOY_Y.wasAxisPressed() == 1) {
    Serial.println("Y-Axis=1");
    if (display_playlists_active) {
      // Playlist ersetzen
      mpdClient.remove(mpdClient.playlists_get_data());
    } else {
      if (display_db_active) {
        // wenn Type Directory, dann eine Ebene tiefer
        if (mpdClient.mpd_db_get_data_type() == ENTRY_TYP_DIRECTORY) {
          if (mpdClient.get_mpd_db(mpdClient.mpd_db_get_data())) {
            display_db();
          }
         }
      } else {
        response=mpdClient.pause();
        Serial.println(response->errorMsg);
      }
  }
  }

  if (GO.JOY_X.wasAxisPressed() == 1) {
    Serial.println("X-Axis=1");
    if (display_playlists_active) {
        if (mpdClient.playlists_set_pos_next()) display_playlists();
    } else {
      if (display_db_active) {
        if (mpdClient.mpd_db_set_pos_next()) display_db();
      } else {
        mpdClient.next();
      }
    }
  }

  if (GO.JOY_X.wasAxisPressed() == 2) {
    Serial.println("X-Axis=2");
    if (display_playlists_active) {
        if (mpdClient.playlists_set_pos_prev()) display_playlists();
    } else {
      if (display_db_active) {
        if (mpdClient.mpd_db_set_pos_prev()) display_db();
      } else {
        mpdClient.prev();
      }
    }
  }

  if (GO.BtnMenu.wasPressed() == 1) {
    response=mpdClient.random();
    Serial.println("Random: ");
    Serial.println(response->errorMsg);
  }


  if (GO.BtnVolume.wasPressed() == 1) {
    response=mpdClient.repeat();
    Serial.println("Repeat: ");
    Serial.println(response->errorMsg);
  }

  if (GO.BtnSelect.wasPressed() == 1) {
    if (!display_playlists_active) {
      display_db_active = !display_db_active;
      if (display_db_active) {
        mpdClient.mpd_db_set_pos_head();
        display_db();
      } else {
		display_odroid_status();
        display_songinfo();
        display_mpd_status();
      }
    }
  }
  
  if (GO.BtnStart.wasPressed() == 1) {
    if (!display_db_active) {
      display_playlists_active = !display_playlists_active;
      if (display_playlists_active) {
        mpdClient.playlists_set_pos_head();
        display_playlists();
      } else {
		display_odroid_status();
        display_songinfo();
        display_mpd_status();
      }
    }
  }

  if (GO.BtnA.wasPressed() == 1);
  if (GO.BtnB.wasPressed() == 1);

}


// **********************************************************
void setup() {

  // falls man es irgendwo mal brauchen sollte... 
  Serial.begin(115200);
  Serial.println("Setup...");
  // ODROID-Zeugs initialisieren
  GO.begin();
  
  //fix speaker ticking
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);

  // SD-Karte
  if (!SD.begin(22)) {
    Serial.println("SD-Card failed!");
    while (1);
  }
  // Konfiguration von SD-Karte einlesen (, wenn Datei vorhanden)
  load_config();
  // aktuelle Konfiguration auf Display ausgeben
  print_config();
  // WIFI initialisieren
  wifi_connect();
  // mit MPD verbinden
  mpd_connect();
  // MPD-Infos das erste Mal ausgeben
  display_songinfo();
  display_mpd_status();

  // Playlisten holen/ausgeben (UART)  
  Serial.println("Beginn Playlisten:");
  char buf[100];
  if (mpdClient.get_playlists()) {
    if (mpdClient.playlists_set_pos_head()) {
      strncpy(buf, mpdClient.playlists_get_data(), sizeof(buf));
      Serial.println(buf);
      while (mpdClient.playlists_set_pos_next()) {
          strncpy(buf, mpdClient.playlists_get_data(), sizeof(buf));
          Serial.println(buf);
      }
    }
  } else {
    Serial.println("Fehler bei get_playlists!");  
    Serial.println(mpdClient.last_mpd_errormsg());  
  }  
  Serial.println("Ende Playlisten!");

  // DB holen/ausgeben (UART)
  Serial.println("\n");
  Serial.println("Beginn new DB:");
  if (mpdClient.get_mpd_db("")) {
    if (mpdClient.mpd_db_set_pos_head()) {
      strncpy(buf, mpdClient.mpd_db_get_data(), sizeof(buf));
      Serial.println(buf);
      while (mpdClient.mpd_db_set_pos_next()) {
          strncpy(buf, mpdClient.mpd_db_get_data(), sizeof(buf));
          Serial.println(buf);
      }
    }
  } else {
    Serial.println("Fehler bei get_mpd_db!");  
    Serial.println(mpdClient.last_mpd_errormsg());  
  }
  Serial.println("Ende new DB!");

  // NTP-Time
  waitForSync();
  Serial.println("UTC: " + UTC.dateTime());
  MyZone.setLocation(my_time_zone);
  Serial.println("MyZone time: " + MyZone.dateTime("d-M-y H:i"));
  
  // Odroid-Batterie
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adc_chars);

  // alle Sekunde Odroid-Status aufrufen
  display_odroid_status();
  if (Timer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, display_odroid_status))
	Serial.println("Starting  Timer0 OK, millis() = " + String(millis()));
  else
	Serial.println("Can't set Timer0. Select another freq. or timer");
  
}

// **********************************************************
void loop() {
  GO.update();
  // gab es eine Statusaenderung beim MPD?
  if (mpdClient.statusChange()) {
    Serial.println("mpd-status...");
    if (!display_playlists_active && !display_db_active) {
      display_songinfo();
      display_mpd_status();
    }
  } 
  button_loop();
}
