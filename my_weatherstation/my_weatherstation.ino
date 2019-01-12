/* *************************************************************
           my_weatherstation.ino
           =====================
             Uwe Berger; 2018
 
ODROID-Go (https://wiki.odroid.com/odroid_go/odroid_go)

Im WIFI anmelden und Topics von einem MQTT-Broker abonnieren und ent-
sprechend darstellen.
 
Konfiguration auf SD-Karte via Textdatei:
-----------------------------------------
Der Dateiname der Konfigurationsdatei auf der SD-Karte ist im 
Header-File my_weatherstation.h via Define #CONFIG_FILE festgelegt.

Inhalt sollte im JSON-Format sein; Bsp.:
----------------------------------------

{
  "wifi_ssid":"hastdunichtgesehen",
  "wifi_pwd":"eingeheimnis",
  "hostname":"my_odroid",
  "mqtt_server":"dermqttbroker",
  "mqtt_port":"nocheingeheimnis",
  "mqtt_user":"soso",
  "mqtt_pwd":"wieauchimmer",
  "mqtt_client_id":"mqttclient",
  "icon_path":"/icons/owm/"
}

Wird die definierte Datei nicht gefunden bzw. ist der Dateiinhalt nicht
im JSON-Format, gelten die Defaultwerte, welche in der Header-Datei 
my_weatherstation.h via Defines festgelegt sind.

MQTT-Topics, welche abonniert und ausgewertet werden:
-----------------------------------------------------

myweather/all/json 
{
	"timestamp":"19.12.2018, 19:44:47",
	"temperature_out":"3.3",
	"temperature_in":"18.7",
	"humidity_in":"55.9",
	"humidity_out":"88.3",
	"pressure_rel":"1015.0",
	"pressure_rising":"0"
}

weatherforecast/all/json 
{
  "city":"Brandenburg an der Havel",
  "longitude":"12.55",
  "latitude":"52.42",
  "current_date":"Sat, 05.01.2019, 19:07",
  "sunrise":"08:18",
  "sunset":"16:12",
  "current_temp":"6",
  "current_pressure":"1022",
  "current_humidity":"87",
  "current_wind_speed":"2.6",
  "current_wind_deg":"10",
  "current_clouds":"75",
  "current_visibility":"10000",
  "forecast":[
    {
      "day":"Sat",
      "date":"05.01.2019",
      "temp_low":"2",
      "temp_high":"6",
      "temp_day":"6",
      "temp_night":"2",
      "temp_eve":"5",
      "temp_morn":"6",
      "pressure":"1026",
      "humidity":"96",
      "speed":"4.66",
      "deg":"343",
      "clouds":"92",
      "rain":"0.32",
      "snow":"n/a",
      "text":"light rain",
      "code":"500"
    },
    {"day":"Sun","date":"06.01.2019","temp_low":"-2","temp_high":"3","temp_day":"3","temp_night":"1","temp_eve":"-2","temp_morn":"1","pressure":"1033","humidity":"100","speed":"2.11","deg":"326","clouds":"0","rain":"0.35","snow":"0.07","text":"light snow","code":"600"},
    {"day":"Mon","date":"07.01.2019","temp_low":"2","temp_high":"5","temp_day":"5","temp_night":"3","temp_eve":"3","temp_morn":"3","pressure":"1031","humidity":"99","speed":"4.66","deg":"262","clouds":"92","rain":"5.98","snow":"n/a","text":"moderate rain","code":"501"},
    {"day":"Tue","date":"08.01.2019","temp_low":"1","temp_high":"6","temp_day":"6","temp_night":"1","temp_eve":"4","temp_morn":"5","pressure":"999","humidity":"96","speed":"6.46","deg":"273","clouds":"92","rain":"9.35","snow":"0.37","text":"light snow","code":"600"},
    {"day":"Wed","date":"09.01.2019","temp_low":"2","temp_high":"3","temp_day":"2","temp_night":"3","temp_eve":"2","temp_morn":"2","pressure":"1024","humidity":"0","speed":"5.41","deg":"20","clouds":"74","rain":"1.8","snow":"0.18","text":"light snow","code":"600"},
    {"day":"Thu","date":"10.01.2019","temp_low":"-2","temp_high":"2","temp_day":"2","temp_night":"-2","temp_eve":"1","temp_morn":"2","pressure":"1035","humidity":"0","speed":"3.28","deg":"357","clouds":"96","rain":"0.32","snow":"0.05","text":"light snow","code":"600"},
    {"day":"Fri","date":"11.01.2019","temp_low":"-1","temp_high":"3","temp_day":"2","temp_night":"3","temp_eve":"3","temp_morn":"-1","pressure":"1018","humidity":"0","speed":"7.23","deg":"245","clouds":"99","rain":"1.6","snow":"4.14","text":"snow","code":"601"}
    ]
}

sensors/+/json
{
	"heap":"23216",
	"temperature":"30.8",
    "humidity":"41.7",
    "unixtime":"1533665227",
    "node_name":"esp8266-9982412",
    "node_alias":"Bad",
    "node_type":"dht22",
    "readable_ts":"2018/08/07 20:07:07"
}

zerow/sysinfo/json 
{ 
	"hostname": "zerow", 
	"uptime": "2 days, 2:34:21", 
	"load": [ "0.32", "0.25", "0.27" ], 
	"ram": { "free": "234832", "share": "21912", "buffer": "39472", "total": "492620" }, 
	"swap": { "free": "102396", "total": "102396" }, 
	"processes": "108", 
	"time": { "unix": "1536080941", "readable": "2018\/09\/04 19:09:01" } 
}


Have fun!
---------

***************************************************************
*/


#include <odroid_go.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "my_weatherstation.h"
 
myweather_t myweather	= {0};
forecast_t forecast 	= {0};
sensors_t sensors 		= {0};
sysinfo_t sysinfo 		= {0};

char ssid[30] 	= WLAN_SSID;
char password[30] = WLAN_PWD;
char hostname[30] = HOSTNAME;

char mqttServer[30] 	= MQTT_BROKER;
int  mqttPort 		= MQTT_PORT;
char mqttUser[30] 	= MQTT_USER;
char mqttPassword[30] = MQTT_PWD;
char mqttClientId[30] = MQTT_CLIENT_ID;

int8_t current_display = DISPLAY_MYWEATHER;
int8_t forecast_idx = 0;
int8_t sensors_idx = 0;
int8_t sysinfo_idx = 0;

char weather_icon_path[50] = ICON_PATH;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
File cfg_file;

// **************************************************************
void load_config(void)
{
  char buf[1024] = "";
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
  strlcpy(mqttServer, root["mqtt_server"], sizeof(mqttServer));
  mqttPort = root["mqtt_port"];
  strlcpy(mqttUser, root["mqtt_user"], sizeof(mqttUser));
  strlcpy(mqttPassword, root["mqtt_pwd"], sizeof(mqttPassword));
  strlcpy(mqttClientId, root["mqtt_client_id"], sizeof(mqttClientId));
  strlcpy(weather_icon_path, root["icon_path"], sizeof(weather_icon_path));
  cfg_file.close();
}

// **************************************************************
void print_config(void)
{
  GO.lcd.print("SSID: "); GO.lcd.println(ssid);
  GO.lcd.print("Pwd: "); GO.lcd.println(password);
  GO.lcd.print("Hostname: "); GO.lcd.println(hostname);
  GO.lcd.print("MQTT-Broker: "); GO.lcd.println(mqttServer);
  GO.lcd.print("MQTT-Port: "); GO.lcd.printf("%i\n", mqttPort);
  GO.lcd.print("MQTT-User: "); GO.lcd.println(mqttUser);
  GO.lcd.print("MQTT-Pwd: "); GO.lcd.println(mqttPassword);
  GO.lcd.print("MQTT-Client-ID: "); GO.lcd.println(mqttClientId);
  GO.lcd.print("Icon-Path: "); GO.lcd.println(weather_icon_path);
}


// **************************************************************
void display_from_to(uint8_t from, uint8_t to)
{
  char buf[50];
  GO.lcd.setCursor(280, 220);
  GO.lcd.setTextSize(1);
  GO.lcd.setTextColor(WHITE);  
  sprintf(buf, "%d/%d", from, to);
  GO.lcd.println(buf);	
	
}

// **************************************************************
void display_sensors(void)
{
  char buf[50];
  // Ueberschrift
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(2);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("My sensors");
  if (sensors.count > 0) {
  // einzelne Werte
    //sprintf(buf, "%s (%s)", sensors.val[sensors_idx].node_name, sensors.val[sensors_idx].node_alias);
    display_entry(BLUE, sensors.val[sensors_idx].node_name, WHITE, sensors.val[sensors_idx].node_alias);
    sprintf(buf, "%s C", sensors.val[sensors_idx].temperature);
    display_entry(BLUE, "temperature:", WHITE, buf);
    sprintf(buf, "%s%%", sensors.val[sensors_idx].humidity);
    display_entry(BLUE, "humidity:", WHITE, buf);
    sprintf(buf, "%s", sensors.val[sensors_idx].readable_ts);
    display_entry(BLUE, "last update:", WHITE, buf); 
  } else {
    GO.lcd.setTextSize(1);
    GO.lcd.setTextColor(WHITE);
    GO.lcd.println("No data!");	  
  }
  display_from_to(sensors_idx+1, sensors.count);  
}


// **************************************************************
void display_forecast(void)
{
  char buf[50];
  // Ueberschrift
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(2);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("Forecast openweathermap");
  // einzelne Werte
  GO.lcd.setCursor(0, 40);
  if (forecast.count) {
    sprintf(buf, "%s (%s)", forecast.val[forecast_idx].day, forecast.val[forecast_idx].date);
    display_entry(BLUE, "day:", WHITE, buf);
    sprintf(buf, "%s/%s C (%s, %s, %s, %s)", 
			forecast.val[forecast_idx].temp_low, forecast.val[forecast_idx].temp_high,
			forecast.val[forecast_idx].temp_morn, forecast.val[forecast_idx].temp_day,
			forecast.val[forecast_idx].temp_eve, forecast.val[forecast_idx].temp_night
			);
    display_entry(BLUE, "temperature (min., max., morn, day, eve, night):", WHITE, buf);
    sprintf(buf, "%shPa, %s%%", forecast.val[forecast_idx].pressure, forecast.val[forecast_idx].humidity);
    display_entry(BLUE, "pressure, humidity:", WHITE, buf);
    sprintf(buf, "%sm/s, %s", forecast.val[forecast_idx].speed, forecast.val[forecast_idx].deg);
    display_entry(BLUE, "wind (speed, deg):", WHITE, buf);
    sprintf(buf, "%s%%, %smmm, %smm", 
            forecast.val[forecast_idx].clouds, 
            forecast.val[forecast_idx].rain, 
            forecast.val[forecast_idx].snow);
    display_entry(BLUE, "clouds, rain, snow:", WHITE, buf);
    sprintf(buf, "%s", forecast.val[forecast_idx].text);
    display_entry(BLUE, "text:", WHITE, buf);
    sprintf(buf, "%s%s", weather_icon_path, forecast.val[forecast_idx].image);
    GO.lcd.drawJpgFile(SD, buf, 180, 100);
  } else {
    GO.lcd.setTextSize(1);
    GO.lcd.setTextColor(WHITE);
    GO.lcd.println("No data!");	  
  }
  display_from_to(forecast_idx+1, FORECASTDAYS_MAX);  
}

// **************************************************************
void display_sysinfo(void)
{
  char buf[50];
  // Ueberschrift
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(2);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("Sysinfo computers");
  // einzelne Werte
  GO.lcd.setCursor(0, 40);
  if (sysinfo.count) {
    sprintf(buf, "%s (%s)", sysinfo.val[sysinfo_idx].hostname, sysinfo.val[sysinfo_idx].uptime);
    display_entry(BLUE, "hostname (uptime):", WHITE, buf);
    sprintf(buf, "%s", sysinfo.val[sysinfo_idx].timestamp);
    display_entry(BLUE, "last seen:", WHITE, buf);
    sprintf(buf, "%s, %s, %s", sysinfo.val[sysinfo_idx].load_1m, sysinfo.val[sysinfo_idx].load_5m, sysinfo.val[sysinfo_idx].load_15m);
    display_entry(BLUE, "load (1m, 5m, 15m):", WHITE, buf);
    sprintf(buf, "%s, %s (%s, %s)", 
            sysinfo.val[sysinfo_idx].ram_total, sysinfo.val[sysinfo_idx].ram_free, 
            sysinfo.val[sysinfo_idx].ram_buffer, sysinfo.val[sysinfo_idx].ram_share
            );
    display_entry(BLUE, "ram total, free (buffer, share) [kByte]:", WHITE, buf);

    sprintf(buf, "%s, %s", sysinfo.val[sysinfo_idx].swap_free, sysinfo.val[sysinfo_idx].swap_total);
    display_entry(BLUE, "swap (free, total) [kByte]:", WHITE, buf);
    sprintf(buf, "%s", sysinfo.val[sysinfo_idx].processes);
    display_entry(BLUE, "processes:", WHITE, buf);
  } else {
    GO.lcd.setTextSize(1);
    GO.lcd.setTextColor(WHITE);
    GO.lcd.println("No data!");	  
  }
  display_from_to(sysinfo_idx+1, sysinfo.count);  
}

// **************************************************************
void display_location(void)
{
  char buf[50];
  // Ueberschrift
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(2);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("Weather openweathermap");
  // einzelne Werte
  GO.lcd.setCursor(0, 40);
  if (forecast.count) {
    sprintf(buf, "%s (%s/%s)", forecast.city, forecast.longitude, forecast.latitude);
    display_entry(BLUE, "city (lon./lat.):", WHITE, buf);
    sprintf(buf, "%s - %s", forecast.sunrise, forecast.sunset);
    display_entry(BLUE, "sun:", WHITE, buf);
    sprintf(buf, "%s C, %shPa, %s%%", forecast.current_temp, forecast.current_pressure, forecast.current_humidity);
    display_entry(BLUE, "temp., press., hum.:", WHITE, buf);
    sprintf(buf, "%sm/s, %s", forecast.current_wind_speed, forecast.current_wind_deg);
    display_entry(BLUE, "wind (speed, deg):", WHITE, buf);
    sprintf(buf, "%s%%, %s", forecast.current_clouds, forecast.current_visibility);
    display_entry(BLUE, "clouds, visibility:", WHITE, buf);
    display_entry(BLUE, "last update:", WHITE, forecast.current_date);
  } else {
    GO.lcd.setTextSize(1);
    GO.lcd.setTextColor(WHITE);
    GO.lcd.println("No data!");
  }
}

// **************************************************************
void display_entry(int label_color, char* label, int text_color, const char* text)
{
  GO.lcd.setTextColor(label_color);
  GO.lcd.setTextSize(1);
  GO.lcd.println(label);
  GO.lcd.setTextColor(text_color);
  GO.lcd.setTextSize(1);
  GO.lcd.println(text);	
}

// **************************************************************
void display_myweather(void)
{
  char buf[50];
  // Ueberschrift
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(2);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("My weather");
  // einzelne Werte
  GO.lcd.setCursor(0, 40);
  if (myweather.present) {
    display_entry(BLUE, "timestamp:", WHITE, myweather.timestamp);
    sprintf(buf, "%s C", myweather.temperature);
    display_entry(BLUE, "temperature:", WHITE, buf);
    sprintf(buf, "%s%%", myweather.humidity);
    display_entry(BLUE, "humidity:", WHITE, buf);
    sprintf(buf, "%shPa (%s)", myweather.pressure_rel, myweather.pressure_rising);
    display_entry(BLUE, "pressure (rel):", WHITE, buf);
  } else {
    GO.lcd.setTextSize(1);
    GO.lcd.setTextColor(WHITE);
    GO.lcd.println("No data!");
  }
}

// **************************************************************
void display(void)
{
  switch (current_display) {
	case DISPLAY_MYWEATHER:
	  display_myweather();
	  break;
	case DISPLAY_FORECAST:
	  display_forecast();
	  break;
	case DISPLAY_LOCATION:
	  display_location();
	  break;
	case DISPLAY_SENSORS:
	  display_sensors();
	  break;
	case DISPLAY_SYSINFO:
	  display_sysinfo();
	  break;
	default:
	  display_myweather();
	  break;  
  }	
}

// **************************************************************
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  uint8_t i, idx;
  uint8_t found;
  char *t[20];
  char buf[256];
  // Topic separieren
  strlcpy(buf, topic, sizeof(buf));
  i = 0;
  t[i] = strtok(buf,"/");
  while(t[i]!=NULL) t[++i] = strtok(NULL,"/");
  StaticJsonBuffer<4096> jsonBuffer;
  // Payload als JSON-String interpretieren
  JsonObject& root = jsonBuffer.parseObject(payload);  
  // kein JSON?
  if (!root.success()) {
    return;
  } 
  // nach abonnierten Topic verzweigen und entspr. Daten extrahieren
  // ...myweather
  if (strcmp(topic, TOPIC_MYWEATHER_JSON) == 0) {
	strlcpy(myweather.timestamp, root["timestamp"], sizeof(myweather.timestamp));
	strlcpy(myweather.temperature, root["temperature_out"], sizeof(myweather.temperature));
	strlcpy(myweather.humidity, root["humidity_out"], sizeof(myweather.humidity));
	strlcpy(myweather.pressure_rel, root["pressure_rel"], sizeof(myweather.pressure_rel));
	if (root["pressure_rising"] == "0") {
	  strlcpy(myweather.pressure_rising, "falling", sizeof(myweather.pressure_rising));
    } else {
	  strlcpy(myweather.pressure_rising, "rising", sizeof(myweather.pressure_rising));
    }
    myweather.present = 1;
    if (current_display == DISPLAY_MYWEATHER) display();
  }
  else
  // ...forecast
  if (strcmp(topic, TOPIC_FORECAST_JSON) == 0) {
  	strlcpy(forecast.city, root["city"], sizeof(forecast.city));
  	strlcpy(forecast.longitude, root["longitude"], sizeof(forecast.longitude));
  	strlcpy(forecast.latitude, root["latitude"], sizeof(forecast.latitude));
  	strlcpy(forecast.current_date, root["current_date"], sizeof(forecast.current_date));
  	strlcpy(forecast.current_temp, root["current_temp"], sizeof(forecast.current_temp));
  	strlcpy(forecast.current_pressure, root["current_pressure"], sizeof(forecast.current_pressure));
  	strlcpy(forecast.current_humidity, root["current_humidity"], sizeof(forecast.current_humidity));
  	strlcpy(forecast.current_wind_speed, root["current_wind_speed"], sizeof(forecast.current_wind_speed));
  	strlcpy(forecast.current_wind_deg, root["current_wind_deg"], sizeof(forecast.current_wind_deg));
  	strlcpy(forecast.current_clouds, root["current_clouds"], sizeof(forecast.current_clouds));
  	strlcpy(forecast.current_visibility, root["current_visibility"], sizeof(forecast.current_visibility));
  	strlcpy(forecast.sunrise, root["sunrise"], sizeof(forecast.sunrise));
  	strlcpy(forecast.sunset, root["sunset"], sizeof(forecast.sunset));
  	JsonArray& forecast_day = root["forecast"];
  	forecast.count = 0;
    for (i=0; i < FORECASTDAYS_MAX; i++) {
	  strlcpy(forecast.val[i].day, forecast_day[i]["day"], sizeof(forecast.val[0].day));
	  strlcpy(forecast.val[i].date, forecast_day[i]["date"], sizeof(forecast.val[0].date));
	  strlcpy(forecast.val[i].temp_low, forecast_day[i]["temp_low"], sizeof(forecast.val[0].temp_low));
	  strlcpy(forecast.val[i].temp_high, forecast_day[i]["temp_high"], sizeof(forecast.val[0].temp_high));
	  strlcpy(forecast.val[i].temp_day, forecast_day[i]["temp_day"], sizeof(forecast.val[0].temp_day));
	  strlcpy(forecast.val[i].temp_night, forecast_day[i]["temp_night"], sizeof(forecast.val[0].temp_night));
	  strlcpy(forecast.val[i].temp_eve, forecast_day[i]["temp_eve"], sizeof(forecast.val[0].temp_eve));
	  strlcpy(forecast.val[i].temp_morn, forecast_day[i]["temp_morn"], sizeof(forecast.val[0].temp_morn));
	  strlcpy(forecast.val[i].pressure, forecast_day[i]["pressure"], sizeof(forecast.val[0].pressure));
	  strlcpy(forecast.val[i].humidity, forecast_day[i]["humidity"], sizeof(forecast.val[0].humidity));
	  strlcpy(forecast.val[i].speed, forecast_day[i]["speed"], sizeof(forecast.val[0].speed));
	  strlcpy(forecast.val[i].deg, forecast_day[i]["deg"], sizeof(forecast.val[0].deg));
	  strlcpy(forecast.val[i].clouds, forecast_day[i]["clouds"], sizeof(forecast.val[0].clouds));
	  strlcpy(forecast.val[i].rain, forecast_day[i]["rain"], sizeof(forecast.val[0].rain));
	  strlcpy(forecast.val[i].snow, forecast_day[i]["snow"], sizeof(forecast.val[0].snow));
	  strlcpy(forecast.val[i].text, forecast_day[i]["text"], sizeof(forecast.val[0].text));
	  strlcpy(forecast.val[i].code, forecast_day[i]["code"], sizeof(forecast.val[0].code));
	  strlcpy(forecast.val[i].image, forecast_day[i]["image"], sizeof(forecast.val[0].image));
	  forecast.count++;
    }
    if (current_display == DISPLAY_FORECAST) display();
  }
  else
  // "sensors/+/json"
  if (strcmp(t[0], "sensors") == 0) {
    // bereits ein Eintrag im Array fuer diesen Sensor?
    found=0;
    idx=0;
    for (i=0; i<sensors.count; i++) {
      if (!found && strcmp(sensors.val[i].node_name, t[1])==0) {
        found=1;
        idx=i;
      }
    }
    // ...nein, also Eintrag einrichten
    if (!found) {
      idx=sensors.count;
      if (sensors.count < MAX_SENSORS) sensors.count++;
    } 
    // Werte uebernehmen
    strlcpy(sensors.val[idx].node_name, root["node_name"], sizeof(sensors.val[0].node_name));
    strlcpy(sensors.val[idx].node_alias, root["node_alias"], sizeof(sensors.val[0].node_alias));
    strlcpy(sensors.val[idx].node_type, root["node_type"], sizeof(sensors.val[0].node_type));
    strlcpy(sensors.val[idx].temperature, root["temperature"], sizeof(sensors.val[0].temperature));
    strlcpy(sensors.val[idx].humidity, root["humidity"], sizeof(sensors.val[0].humidity));
    strlcpy(sensors.val[idx].readable_ts, root["readable_ts"], sizeof(sensors.val[0].readable_ts));
    if ((current_display == DISPLAY_SENSORS) && (idx == sensors_idx)) display();
  }
  else
  // "+/sysinfo/json"
  if (strcmp(t[1], "sysinfo") == 0) {
    // bereits ein Eintrag im Array fuer diesen Sensor?
    found=0;
    idx=0;
    for (i=0; i<sysinfo.count; i++) {
      if (!found && strcmp(sysinfo.val[i].hostname, t[0])==0) {
        found=1;
        idx=i;
      }
    }
    // ...nein, also Eintrag einrichten
    if (!found) {
      idx=sysinfo.count;
      if (sysinfo.count < MAX_SYSINFO) sysinfo.count++;
    } 
    // Werte uebernehmen
    strlcpy(sysinfo.val[idx].hostname, root["hostname"], sizeof(sysinfo.val[0].hostname));
    strlcpy(sysinfo.val[idx].uptime, root["uptime"], sizeof(sysinfo.val[0].uptime));
    strlcpy(sysinfo.val[idx].processes, root["processes"], sizeof(sysinfo.val[0].processes));
  	JsonArray& sysinfo_load = root["load"];
	strlcpy(sysinfo.val[idx].load_1m, sysinfo_load[0], sizeof(sysinfo.val[0].load_1m));
	strlcpy(sysinfo.val[idx].load_5m, sysinfo_load[1], sizeof(sysinfo.val[0].load_5m));
	strlcpy(sysinfo.val[idx].load_15m, sysinfo_load[2], sizeof(sysinfo.val[0].load_15m));
  	JsonObject& sysinfo_ram = root["ram"];
    strlcpy(sysinfo.val[idx].ram_free, sysinfo_ram["free"], sizeof(sysinfo.val[0].ram_free));
    strlcpy(sysinfo.val[idx].ram_buffer, sysinfo_ram["buffer"], sizeof(sysinfo.val[0].ram_buffer));
    strlcpy(sysinfo.val[idx].ram_share, sysinfo_ram["share"], sizeof(sysinfo.val[0].ram_share));
    strlcpy(sysinfo.val[idx].ram_total, sysinfo_ram["total"], sizeof(sysinfo.val[0].ram_total));
  	JsonObject& sysinfo_swap = root["swap"];
    strlcpy(sysinfo.val[idx].swap_free, sysinfo_swap["free"], sizeof(sysinfo.val[0].swap_free));
    strlcpy(sysinfo.val[idx].swap_total, sysinfo_swap["total"], sizeof(sysinfo.val[0].swap_total));
  	JsonObject& sysinfo_time = root["time"];
    strlcpy(sysinfo.val[idx].timestamp, sysinfo_time["readable"], sizeof(sysinfo.val[0].timestamp));
    if ((current_display == DISPLAY_SYSINFO) && (idx == sysinfo_idx)) display();
  }  
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
  GO.lcd.println("Connected!");
  GO.lcd.println(WiFi.localIP());
}


// **************************************************************
void setup() {
  // falls man es irgendwo mal brauchen sollte... 
  Serial.begin(115200);
  // ODROID-Zeugs initialisieren
  GO.begin();
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
}

// **************************************************************
void mqtt_reconnect ()
{
  // MQTT...
  // ...initialisieren
  mqtt_client.setServer(mqttServer, mqttPort);
  // ...Callback setzen
  mqtt_client.setCallback(mqtt_callback);
  // ...verbinden
  while (!mqtt_client.connected()) {
    GO.lcd.println("Connecting to MQTT...");
    if (mqtt_client.connect(mqttClientId, mqttUser, mqttPassword )) {
       GO.lcd.println("Connected!");  
    } else {
      GO.lcd.print("failed with state ");
      GO.lcd.print(mqtt_client.state());
      delay(500);
    }
  }
  // ...Topic abonnieren
  mqtt_client.subscribe(TOPIC_MYWEATHER_JSON);
  mqtt_client.subscribe(TOPIC_FORECAST_JSON);
  mqtt_client.subscribe(TOPIC_SENSORS_JSON);
  mqtt_client.subscribe(TOPIC_SYSINFO_JSON);
}

// **************************************************************
void button_loop(void) 
{
  // Buttons verarzten
  if (GO.JOY_Y.wasAxisPressed() == 2) {
	 current_display--;
	 if (current_display < DISPLAY_FIRST) current_display = DISPLAY_LAST;
  display();
  }
  if (GO.JOY_Y.wasAxisPressed() == 1) {
	 current_display++;
	 if (current_display > DISPLAY_LAST) current_display = DISPLAY_FIRST;
  display();
  }
  if (GO.JOY_X.wasAxisPressed() == 1) {
	 if (current_display == DISPLAY_FORECAST) {
	   forecast_idx++;
	   if (forecast_idx > (FORECASTDAYS_MAX-1)) forecast_idx = 0;
	 } else
	 if (current_display == DISPLAY_SENSORS) {
	   sensors_idx++;
	   if (sensors_idx > (sensors.count-1)) sensors_idx = 0;
	 } else
	 if (current_display == DISPLAY_SYSINFO) {
	   sysinfo_idx++;
	   if (sysinfo_idx > (sysinfo.count-1)) sysinfo_idx = 0;
	 }	 
  display();
  }
  if (GO.JOY_X.wasAxisPressed() == 2) {
	 if (current_display == DISPLAY_FORECAST) {
	    forecast_idx--;
	    if (forecast_idx < 0) forecast_idx = (FORECASTDAYS_MAX-1);
     } else 
	 if (current_display == DISPLAY_SENSORS) {
	    sensors_idx--;
	    if (sensors_idx < 0) sensors_idx = (sensors.count-1);
     } else
	 if (current_display == DISPLAY_SYSINFO) {
	    sysinfo_idx--;
	    if (sysinfo_idx < 0) sysinfo_idx = (sysinfo.count-1);
     }     
  display();
  }
}

// **************************************************************
// **************************************************************
// **************************************************************
void loop() {
  GO.update();
  // MQTT-Loop
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
  // Botton-Abfrage
  button_loop();
}
