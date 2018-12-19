/* *************************************************************
           my_weatherstation.ino
           =====================
             Uwe Berger; 2018
 
ODROID-Go (https://wiki.odroid.com/odroid_go/odroid_go)

Im WIFI anmelden und Topics von einem MQTT-Broker abonnieren und ent-
sprechend darstellen.
 
Topic:
------
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
	"city":"Brandenburg",
	"longitude":"12.56",
	"latitude":"52.41",
	"current_date":"Wed, 19.12.2018, 19:07",
	"sunrise":"08:17",
	"sunset":"15:57",
	"forecast":[
				{"day":"Wed","date":"19.12.2018","temp_low":"0","temp_high":"3","text":"Scattered Showers","code":"39"},
				{"day":"Thu","date":"20.12.2018","temp_low":"1","temp_high":"5","text":"Rain","code":"12"},
				{"day":"Fri","date":"21.12.2018","temp_low":"4","temp_high":"8","text":"Rain","code":"12"},
				{"day":"Sat","date":"22.12.2018","temp_low":"5","temp_high":"10","text":"Scattered Showers","code":"39"},
				{"day":"Sun","date":"23.12.2018","temp_low":"5","temp_high":"7","text":"Rain","code":"12"},
				{"day":"Mon","date":"24.12.2018","temp_low":"5","temp_high":"7","text":"Rain","code":"12"},
				{"day":"Tue","date":"25.12.2018","temp_low":"3","temp_high":"4","text":"Mostly Cloudy","code":"28"}
			   ]
}


Have fun!
---------

***************************************************************
*/


#include <odroid_go.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
 
 
#define TOPIC_MYWEATHER_JSON "myweather/all/json"
#define TOPIC_FORECAST_JSON  "weatherforecast/all/json"
 
const char* ssid =          "xyz";
const char* password =      "xyz42";
#define HOSTNAME 			"odroid"

const char* mqttServer =    "10.42.42.42";
const int   mqttPort =      1883;
const char* mqttUser =      "";
const char* mqttPassword =  "";

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

StaticJsonBuffer<1024> jsonBuffer;

// **************************************************************
void display_forecast(byte* payload)
{

}

// **************************************************************
void display_myweather_entry(int label_color, char* label, int text_color, const char* text)
{
  GO.lcd.setTextColor(label_color);
  GO.lcd.setTextSize(1);
  GO.lcd.println(label);
  GO.lcd.setTextColor(text_color);
  GO.lcd.setTextSize(2);
  GO.lcd.println(text);	
}

// **************************************************************
void display_myweather(byte* payload)
{
  char buf[50];
  // Payload als JSON-String interpretieren
  JsonObject& root = jsonBuffer.parseObject(payload);
  // kein JSON?
  if (!root.success()) {
    return;
  }  
  // Werte aus JSON-String extrahieren und auswerten
  const char* timestamp = root["timestamp"];
  const char* temperature = root["temperature_out"];
  const char* humidity = root["humidity_out"];
  const char* pressure_rel = root["pressure_rel"];
  char pressure_rising[10];
  if (root["pressure_rising"] == "0") {
	sprintf(pressure_rising, "falling");
  } else {
	sprintf(pressure_rising, "rising");
  }
  // Ueberschrift
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(2);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("My weather:");
  // einzelne Werte
  GO.lcd.setCursor(0, 40);
  display_myweather_entry(BLUE, "timestamp:", WHITE, timestamp);
  sprintf(buf, "%s C", temperature);
  display_myweather_entry(BLUE, "temperature:", WHITE, buf);
  sprintf(buf, "%s%%", humidity);
  display_myweather_entry(BLUE, "humidity:", WHITE, buf);
  sprintf(buf, "%shPa (%s)", pressure_rel, pressure_rising);
  display_myweather_entry(BLUE, "pressure (rel)::", WHITE, buf);
}

// **************************************************************
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  // nach abonnierten Topic verzweigen
  String topic_str(topic);
  if (topic_str.compareTo(TOPIC_MYWEATHER_JSON)==0) 
	display_myweather(payload);
  else  
  if (topic_str.compareTo(TOPIC_FORECAST_JSON)==0) 
	display_forecast(payload);
}

// **************************************************************
void wifi_connect()
{
  // ins WIFI anmelden 
  WiFi.begin(ssid, password);
  WiFi.setHostname(HOSTNAME);
  GO.lcd.print("Connect to WiFi: ") ;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    GO.lcd.print(".");
  }
  GO.lcd.println("");
  GO.lcd.printf("Connected to WiFi with IP (hostname): %s (%s)\n", WiFi.localIP(), HOSTNAME);
}



// **************************************************************
void setup() {
  // ODROID-Zeugs initialisieren(?)
  GO.begin();
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
    if (mqtt_client.connect("ESP32Client", mqttUser, mqttPassword )) {
       GO.lcd.println("connected");  
    } else {
      GO.lcd.print("failed with state ");
      GO.lcd.print(mqtt_client.state());
      delay(500);
    }
  }
  // ...Topic abonnieren
  mqtt_client.subscribe("myweather/all/json");
  mqtt_client.subscribe("weatherforecast/all/json");
}

// **************************************************************
// **************************************************************
// **************************************************************
void loop() {
  // MQTT-Loop
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
}
