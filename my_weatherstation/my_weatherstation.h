/* *************************************************************
           my_weatherstation.h
           ===================
             Uwe Berger; 2018
 
...ein paar Typ-Deklarationen, Defines, etc. fuer 
my_weatherstation.ino 

Have fun!
---------

***************************************************************
*/

// Konfigurationsfile auf SD-Karte
#define CONFIG_FILE "/config/weather.cfg"


// WLAN-Konfiguration
#define WLAN_SSID	"ssid"
#define WLAN_PWD	"pwd"
#define HOSTNAME	"odroid-hostname"

// MQTT-Konfiguration
#define MQTT_BROKER		"broker"
#define MQTT_PORT		42                           // ;-)
#define MQTT_USER		""
#define MQTT_PWD    	""
#define MQTT_CLIENT_ID	"odroid_mqtt_client_id"

// max. Anzahl Tage Wettervorhersage 
#define FORECASTDAYS_MAX	7

// max. Anzahl verwaltbarer Sensoren
#define MAX_SENSORS 		20

// max. Anzahl verwaltbarer Computer (sysinfo)
#define MAX_SYSINFO			20

// MQTT-Topics
#define TOPIC_MYWEATHER_JSON "myweather/all/json"
#define TOPIC_FORECAST_JSON  "weatherforecast/all/json"
#define TOPIC_SENSORS_JSON   "sensors/+/json"
#define TOPIC_SYSINFO_JSON   "+/sysinfo/json"

// Screens
#define DISPLAY_MYWEATHER 0
#define DISPLAY_LOCATION  1
#define DISPLAY_FORECAST  2
#define DISPLAY_SENSORS   3
#define DISPLAY_SYSINFO   4
#define DISPLAY_FIRST     DISPLAY_MYWEATHER
#define DISPLAY_LAST      DISPLAY_SYSINFO

// Verzeichnis Wetter-Icons auf SD-Karte
#define ICON_PATH "/icons/owm/"



// diverse Typ-Definitionen

typedef struct myweather_t myweather_t;
struct myweather_t
{
  uint8_t present;
  char    timestamp[30];
  char    temperature[10];
  char    humidity[10];
  char 	  pressure_rel[10];
  char 	  pressure_rising[10];
};

typedef struct forecast_val_t forecast_val_t;
struct forecast_val_t
{
	char    day[4];
	char    date[11];
	char    temp_low[5];
	char 	temp_high[5];
	char 	temp_day[5];
	char 	temp_night[5];
	char 	temp_eve[5];
	char 	temp_morn[5];
	char    pressure[5];
	char    humidity[5];
	char    speed[10];
	char    deg[5];
	char    clouds[5];
	char    rain[10];
	char    snow[10];
	char 	text[50];
	char 	code[3];
	char    image[20];
};

typedef struct forecast_t forecast_t;
struct forecast_t
{
	uint8_t	count;
	char city[50];
	char longitude[8];
	char latitude[8];
	char current_date[25];
	char current_temp[5];
	char current_pressure[10];
	char current_humidity[5];
	char current_wind_speed[10];
	char current_wind_deg[5];
	char current_clouds[5];
	char current_visibility[10];
	char sunrise[6];
	char sunset[6];
	forecast_val_t val[7];
}; 

typedef struct sensor_val_t sensor_val_t;
struct sensor_val_t
{
	char    node_name[20];
	char    node_alias[20];
	char    node_type[10];
	char 	temperature[10];
	char 	humidity[10];
	char 	pressure_rel[10];
    char    unixtime[15];
    char    readable_ts[25];
};

typedef struct sensors_t sensors_t;
struct sensors_t
{
	uint8_t	count;
	sensor_val_t val[MAX_SENSORS];
};


typedef struct sysinfo_val_t sysinfo_val_t;
struct sysinfo_val_t
{
	char hostname[21];
	char uptime[20];
	char load_1m[8];
	char load_5m[8];
	char load_15m[8];
	char ram_free[10];
	char ram_share[10];
	char ram_buffer[10];
	char ram_total[10];
	char swap_free[10];
	char swap_total[10];
	char processes[6];
	char timestamp[25];
};

typedef struct sysinfo_t sysinfo_t;
struct sysinfo_t
{
	uint8_t	count;
	sysinfo_val_t val[MAX_SYSINFO];
};
