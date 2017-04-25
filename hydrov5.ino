
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#include "DHT.h"



#define DHTPIN 12
#define DHTTYPE DHT22

//EDIT THESE LINES TO MATCH YOUR SETUP
#define MQTT_SERVER "192.168.1.2"
const char* ssid = "ifca";
const char* password = "efyb45qj";

DHT dht(DHTPIN, DHTTYPE);

byte sensorInterrupt = 5;  // 0 = digital pin 2
byte sensorPin       = 2;
byte lightbulb = 13;
byte fanin = 14;
byte fanout = 16;
byte pump = 4;





// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;  

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;


void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
String macToStr(const uint8_t* mac);
unsigned long currentMillis = 0;


//topic for sensors
char* hydroTemperature = "/ifca/hydrotemperature";
char* hydroHumidity = "/ifca/hydrohumidity";
char* hydroFlowrate = "/ifca/hydroflow";

//topic for status
char* hydroFanin = "/ifca/fanin";
char* hydroFanout = "/ifca/fanout";
char* hydroLightbulb = "/ifca/lightbulb";
char* aquafilter = "/ifca/aquafilter";

//variables
float tempup = 27;
float templow = 24;
float tempthreshup = 27;
float tempthreshlow = 26;
float filterstatus = 0;

float humup = 80;
float humlow = 70;
float humthreshup = 77;
float humthreshlow = 73;

float flowlow = 9.00;
boolean tempcorrect = false;
boolean humcorrect = false;

float faninstatus = 0;
float fanoutstatus = 0;
float lightbulbstatus = 0;




WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);


void setup()
{
  
  // Initialize a serial connection for reporting values to the host
  Serial.begin(115200);
  delay(100);
  //pins
  pinMode(fanout, OUTPUT);  //fan out
  pinMode(fanin, OUTPUT);  //fan in
  pinMode(lightbulb, OUTPUT);  //lightbulb
  pinMode(pump, OUTPUT);   //pump
  dht.begin();
  WiFi.begin(ssid, password);
  
  Serial.print("ok");
  
  reconnect();  
     delay(2000);
  
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

/**
 * Main program loop
 */
void loop()
{
  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(1000);
    
  //waterflow
  if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    detachInterrupt(sensorInterrupt);
    flowRate = ((2000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    pulseCount = 0;
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING); 
  }
  //DHt READING
  float currentHumFloat = dht.readHumidity();
  float currentTempFloat = dht.readTemperature();

  Serial.print("temp reading");

  //correcting temp
  if(currentTempFloat >= tempup)
  {
    digitalWrite(fanin, HIGH);
    Serial.println("fan on");
    tempcorrect = true;
    
  }
  if(currentTempFloat <= templow)
  {
    digitalWrite(lightbulb, HIGH);
    Serial.println("bulb on");
    tempcorrect = true;
  }
  if(tempcorrect == true)
  {
    if(currentTempFloat <= tempthreshup)
    {
      digitalWrite(fanin, LOW);
  
      Serial.println("fan off");
    }
    if(currentTempFloat >= tempthreshlow)
    {
      digitalWrite(lightbulb, LOW);
      Serial.println("lightbulb off");
    }
  }

  //correcting humidity
  if(currentHumFloat >= humup)
  {
    digitalWrite(fanout, HIGH);
    Serial.println("fanout on");
    humcorrect = true;
    
  }
//  if(currentHumFloat <= humlow)
//  {
//    digitalWrite(fanin, HIGH);
//    Serial.println("fanin on");
//    humcorrect = true;
//  }
  if(humcorrect == true)
  {
    if(currentHumFloat <= humthreshup)
    {
      digitalWrite(fanout, LOW);
      Serial.println("fanout off");
    }
  }
  if(flowRate < 9)
  {
    digitalWrite(pump, HIGH);
    filterstatus = 1;
  }
  else {
    filterstatus = 0;
  }


// status update
  if (fanin == HIGH)
  {
    faninstatus = 1;
  }
  else {
    faninstatus = 0;
  }
  if (fanout == HIGH)
  {
    fanoutstatus = 1;
  }
  else{
    fanoutstatus = 0;
  }
  if (lightbulb == HIGH)
  {
    lightbulbstatus = 1;
  }
  else {
    lightbulbstatus = 0;
  }
    if (pump == HIGH)
  {
    filterstatus = 1;
  }
  else {
    filterstatus = 0;
  }
  // printing and sending
  Serial.print("printing");
  char flowmeter[10];
  dtostrf(flowRate,4,1,flowmeter);
  client.publish(hydroFlowrate, flowmeter);
  char humidity[10];
  dtostrf(currentHumFloat,4,1,humidity);
  client.publish(hydroHumidity, humidity);
  char temperature[7];
  dtostrf(currentTempFloat,4,1,temperature);
  client.publish(hydroTemperature, temperature);

  //status
  char faninstatuschar[7];
  dtostrf(faninstatus,4,1,faninstatuschar);
  client.publish(hydroFanin, faninstatuschar);
  char fanoutstatuschar[7];
  dtostrf(fanoutstatus,4,1,fanoutstatuschar);
  client.publish(hydroFanout, fanoutstatuschar);
  char lightbulbstatuschar[7];
  dtostrf(lightbulbstatus,4,1,lightbulbstatuschar);
  client.publish(hydroLightbulb, lightbulbstatuschar);

  //flow publish
  char filter[7];
  dtostrf(filterstatus,4,1,filter);
  client.publish(aquafilter, filter);
  

//Print the flow rate for this second in litres / minute
  Serial.print("Flow rate: ");
  Serial.print(int(flowRate));  // Print the integer part of the variable
  Serial.println(" ");             // Print the decimal point

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" ");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" ");
//  delay(2000);
  

  
}



/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}



//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {

  

}

//networking functions

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMTQQ Connected");
        client.subscribe(hydroFlowrate);
        client.subscribe(hydroHumidity);
        client.subscribe(hydroTemperature);
        client.subscribe(aquafilter);
        client.subscribe(hydroFanin);
        client.subscribe(hydroFanout);
        client.subscribe(hydroLightbulb);
      }



      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
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






