
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// header files for MQTT
#include <PubSubClient.h>// Connect and publish to the MQTT broker
#include <WiFiClient.h>

//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP32.h>

// WiFi network info.
const char* ssid = "TP-LINK_B25006";
const char* password = "EDB25006";

// Sensor pin configuration
const int moisturePin = 39;
const int tempPin = 36;
const int rainCapturePin = 15;

//Variables to store values from sensors
int moistureSensorVal = 0;
double tempSensorVal = 0;
int rainSensorVal = 0;

const int dry = 4200; // Highest value when sensor is dry
const int wet = 1995;  // Least value when sensor is wet

//OpenweatherApi Variables
const String openWeatherMapApiKey = "ba2b773d3edc2c25af7009e6b9e3998b";
const String city = "Passau";
const String countryCode = "DE";

String moistureApiValue = "";
double tempApiVal = 0;
String rainSensorApivalue = "";

String jsonBuffer;

// MQTT
const char* mqtt_server = "100.65.3.83";  // IP of the MQTT broker
const char* moisture_topic = "esp32/moisture";
const char* temperature_topic = "esp32/temperature";
const char* rain_topic = "esp32/rain";
const char* mqtt_username = "kakashi"; // MQTT username
const char* mqtt_password = "kakashi"; // MQTT password
const char* clientID_PI = "RIoT_B2"; // MQTT client ID

String rain;
String temperature;
String moisture;

int moistureFinal = 0;
double tempFinal = 0;
int rainFinal = 0;



// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char cayenneUsername[] = "a9f02ea0-e3e8-11eb-883c-638d8ce4c23d";
char cayennePassword[] = "103f14cf14a78fb58117be5f8ca1d2effed3c00a";
char cayenneClientID[] = "6033ba60-e3e9-11eb-b767-3f1a8f1211ba";

WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient);

void setup() {
  Serial.begin(115200);
  connectWifi();
}

void connectWifi(){
 WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void getOpenWeatherApiData()
{
  if(WiFi.status()== WL_CONNECTED){
        String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
        jsonBuffer = httpGETRequest(serverPath.c_str());
        // JSON.typeof(jsonVar) can be used to get the type of the var
        JSONVar myObject = JSON.parse(jsonBuffer);
        if (JSON.typeof(myObject) == "undefined") {
              Serial.println("Parsing input failed!");
              return;
        }
      tempApiVal = myObject["main"]["temp"]; 
      rainSensorApivalue = myObject["weather"][0]["main"];
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET(); // Send HTTP POST request
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    http.end();
  return payload;
  }
}

void connectMQTT(){

  if (client.connect(clientID_PI, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  } 
}

void sendDataToMQTT(){
  if (client.publish(rain_topic, String(rain).c_str())) {
      Serial.println("Rain value sent!");
    } else {
      Serial.println("Rain value failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID_PI, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(rain_topic, String(rain).c_str());
    }
    
    // Temperature
    if (client.publish(temperature_topic, String(temperature).c_str())) {
      Serial.println("Temperature sent!");
    } else {
      Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID_PI, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(temperature_topic, String(temperature).c_str());
    } 
    
    // Moisture
    if (client.publish(moisture_topic, String(moisture).c_str())) {
      Serial.println("Moisture sent!");
    } else {
      Serial.println("Moisture failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID_PI, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
     
      client.publish(moisture_topic, String(moisture).c_str());
      
    } 
  
  }

void getData(){
  rainSensorVal=digitalRead(rainCapturePin);
  //Serial.println(rainSensorVal);
  tempSensorVal = analogRead(tempPin);
  //Serial.println(tempSensorVal);
  moistureSensorVal =  map(analogRead(moisturePin),wet,dry,100,0);
  //Serial.println(moistureSensorVal);
  getOpenWeatherApiData();
  //Serial.println(tempApiVal);
  //Serial.println(rainSensorApivalue);

  
  if(tempSensorVal > 500 || tempSensorVal <= 0 ){
    Serial.println("Temperature sensor failed, Getting data from OpenweatherAPI: ");
   // Serial.print("Temperature API: ");
   // Serial.println(tempApiVal);
    tempFinal = tempApiVal;
    temperature = " - Temperature: " + String((double)tempApiVal) +" F";
   }else{
   // Serial.print("Temperature sensors: ");
   // Serial.println(tempSensorVal);
    tempFinal = tempSensorVal;
    temperature = " - Temperature: " + String((double)tempSensorVal)+" F";
  }
    
  if(moistureSensorVal == 0){
    Serial.print("Moisture sensor failed, Getting data from OpenweatherAPI: ");
    //Serial.print("Moisture: ");
    if(rainSensorApivalue=="rain"){
      //Serial.println("80%");
      moisture = " - Humidity: 80% ";
      moistureFinal = 80;
    }else{
      moisture = " - Humidity: 10% ";
       moistureFinal = 10;
      }
  }else{
      /*Serial.println("Moisture: ");
      Serial.print(moistureSensorVal);
      Serial.println(" %");*/
      moistureFinal = moistureSensorVal;
      moisture = " - Moisture: " +String((int)moistureSensorVal) + " %";
  }
    
  if(rainSensorVal == 1){
    //Serial.println("Clouds");
    rain =" - Rain: Clouds";
    rainFinal = 1;
  }else{
     // Serial.println("Rain");
      rain =" - Rain: Rain";
      rainFinal = 0;
  }
  
  
  }

  
void loop() {

  delay(5000);
  getData();
  delay(2000);
  connectMQTT();
  sendDataToMQTT();
  Cayenne.begin(cayenneUsername, cayennePassword, cayenneClientID, ssid, password);
  Cayenne.loop();
  
  delay(2000);
  
 

}

// Default function for sending sensor data at intervals to Cayenne.
// You can also use functions for specific channels, e.g CAYENNE_OUT(1) for sending channel 1 data.
CAYENNE_OUT_DEFAULT()
{
  // Write data to Cayenne here. This example just sends the current uptime in milliseconds on virtual channel 0.
  Cayenne.virtualWrite(0, moistureFinal,"rel_hum","p");
  // Some examples of other functions you can use to send data.
  Cayenne.virtualWrite(1, tempFinal, "temp", "f");
  Cayenne.virtualWrite(5, rainFinal, "analog_sensor", "null");
 
  
  //Cayenne.luxWrite(2, 700);
  //Cayenne.virtualWrite(3, 50, TYPE_PROXIMITY, UNIT_CENTIMETER);
}

// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
  CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}
