#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//create 1-wire connection on pin 2 and connect it to the dallasTemp library
OneWire oneWire(12);
DallasTemperature sensors(&oneWire);


//EDIT THESE LINES TO MATCH YOUR SETUP
#define MQTT_SERVER "192.168.1.2"
const char* ssid = "ifca";
const char* password = "efyb45qj";


//sensor
char* tempTopic = "/ifca/aquatemperature";

char* levelTopic = "/ifca/waterlevel";

//correcting
char* waterBackupTopic = "/ifca/waterbackup";

//status
char* FeederTopic = "/ifca/aquaFeeder";
char* HeaterTopic = "/ifca/aquaHeater";


byte levellow = 5;
byte levelhigh = 16;
boolean levelcorrect = false;

byte feeder = 14;
byte backup = 13;
byte heater = 4;

//variable parameters
float templow =  22;
float tempthreshlow = 24;
boolean tempcorrect = false;

float levelstatus = 0;
float feederstatus = 0;
float backupstatus = 0;
float heaterstatus = 0;


WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);


void setup() {

  //start the serial line for debugging
  Serial.begin(115200);
  delay(100);
  pinMode(levellow, INPUT);
  pinMode(levelhigh, INPUT);
  pinMode(feeder, OUTPUT);
  pinMode(backup, OUTPUT);
  pinMode(heater, OUTPUT);

  //start wifi subsystem
  WiFi.begin(ssid, password);

  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //start the temperature sensors
  sensors.begin();

  //wait a bit before starting the main loop
      delay(2000);
}



void loop(){

  // Send the command to update temperatures
  sensors.requestTemperatures(); 

  //get the new temperature
  float currentTempFloat = sensors.getTempCByIndex(0);
  int levellowstate = digitalRead(levellow);
  int levelhighstate = digitalRead(levelhigh);
  if(curretTempFloat <= templow)
  {
    digitalWrite(heater, HIGH);
    Serial.println("heater on");
    tempcorrect = true;
  }
  
  if ((tempcorrect == true) && (currentTempFloat >= tempthreshlow))
  {
   digitalWrite(heater, LOW);
   Serial.println("lightbult off");
  }
  if (levellowstate == LOW)
  {
    digitalWrite(backup, HIGH);
    Serial.println("backup on");
    levelcorrect = true;
  }
  if ((levelcorrect == true) && (levelhigh == HIGH))
  {
    digitalWrite(backup, LOW);
    Serial.println("backup off");
    levelcorrect = false;
  }

  //status update
  if (heater == HIGH);
  {
    heaterstatus = 1;
  }
  else {
    heaterstatus = 0;
  }
  if (backup == HIGH);
  {
    backupstatus = 1;
  }
  else {
    backupstatus = 0;
  }
  if ( == HIGH);
  {
    heaterstatus = 0;
  }
  else {
    heaterstatus = 0;
  }



    
  //convert the temp float to a string and publish to the temp topic
  char temperature[10];
  dtostrf(currentTempFloat,4,1,temperature);
  client.publish(tempTopic, temperature);




  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}
  //maintain MQTT connection
  client.loop();
  //MUST delay to allow ESP8266 WIFI functions to run
  delay(2000); 
}

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length);





//networking functions

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMTQQ Connected");
        client.subscribe(tempTopic);
        client.subscribe(levelTopic);
        client.subscribe(waterBackupTopic);
        client.subscribe(FeederTopic);
        client.subscribe(HeaterTopic);

        
        //subscribe to topics here
      }
    }
  }
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}


