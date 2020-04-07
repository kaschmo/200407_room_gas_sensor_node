/*
 * Room Sensor Node for Temperature, Pressure, Humidity, VOC
 * Sends and receives values via MQTT
 * BME680 sensor
 * https://www.bluedot.space/sensor-boards/bme680/
 * Copyright K.Schmolders 04/2020
 */

// wifi credentials stored externally and .gitignore
 //all wifi credential and MQTT Server importet through wifi_credential.h
 #include "wifi_credentials.h"

 //required for MQTT
 #include <ESP8266WiFi.h>
 //required for OTA updater
 #include <WiFiClient.h>
 #include <ESP8266WebServer.h>
 #include <ESP8266mDNS.h>
 #include <ESP8266HTTPUpdateServer.h>
 //end OTA requirements
 #include <PubSubClient.h>

 
 //#include <Adafruit_BME280.h>
 #include <Adafruit_BME680.h>
 
 
 
 // I2C for BME Sensor module
 uint16_t BME_SCL = 5; //NodeMCU D1 --> SCK on BME680
 uint16_t BME_SDA = 4; //NodeMCU D2 --> SDI on BME680
 
 //timer
 int timer_update_state_count;
 int timer_update_state = 60000; //update status via MQTT every minute
  
 //MQTT (see also wifi_credentials)
 WiFiClient espClient;
 PubSubClient client(espClient);
 
 const char* inTopic = "cmnd/room_sensor_voc/#";
 const char* outTopic = "stat/room_sensor_voc/";
 const char* mqtt_id = "room_sensor_voc";
 
 //BME680
 #define SEALEVELPRESSURE_HPA (1013.25)
 Adafruit_BME680 bme680; // I2C
 float bme680_temperature, bme680_pressure, bme680_humidity, bme680_gas;
 



 //OTA
 ESP8266WebServer httpServer(80);
 ESP8266HTTPUpdateServer httpUpdater;
 
 void setup_wifi() {
   delay(10);
   // We start by connecting to a WiFi network
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
   WiFi.persistent(false);
   WiFi.mode(WIFI_OFF);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
     
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
 
   httpUpdater.setup(&httpServer);
   httpServer.begin();
 }
 
 
 //callback function for MQTT client
 void callback(char* topic, byte* payload, unsigned int length) {
   payload[length]='\0'; // Null terminator used to terminate the char array
   String message = (char*)payload;
 
   Serial.print("Message arrived on topic: [");
   Serial.print(topic);
   Serial.print("]: ");
   Serial.println(message);
   
   //get last part of topic 
   char* cmnd = "test";
   char* cmnd_tmp=strtok(topic, "/");
 
   while(cmnd_tmp !=NULL) {
     cmnd=cmnd_tmp; //take over last not NULL string
     cmnd_tmp=strtok(NULL, "/"); //passing Null continues on string
     //Serial.println(cmnd_tmp);    
   }
   
   
 
    if (!strcmp(cmnd, "status")) {
        Serial.print("Received status request. sending status");
        send_status();
    }
    else if (!strcmp(cmnd, "reset")) {
        Serial.print(F("Reset requested. Resetting..."));
        //software_Reset();
    }
    
 }



//sends module status via MQTT
void send_status()
 {
   char outTopic_status[50];
   char msg[50];
   //IP Address
   strcpy(outTopic_status,outTopic);
   strcat(outTopic_status,"ip_address");
   
   //ESP IP
   WiFi.localIP().toString().toCharArray(msg,50);
   client.publish(outTopic_status,msg ); 
 }
 
 //send Sensor Values via MQTT
 void sendSensorValues(){
    
    char outTopic_status[50];
    char msg[50];
 
    
   //roomtemp from BME680
    strcpy(outTopic_status,outTopic);
    dtostrf(bme680_temperature,2,2,msg); 
    strcat(outTopic_status,"temperature");
    client.publish(outTopic_status, msg);
 
   //BME680 Humidity
    strcpy(outTopic_status,outTopic);
    dtostrf(bme680_humidity,2,2,msg); 
    strcat(outTopic_status,"humidity");
    client.publish(outTopic_status, msg);
 
    //BME680 Pressure
    strcpy(outTopic_status,outTopic);
    dtostrf(bme680_pressure,2,2,msg); 
    strcat(outTopic_status,"pressure");
    client.publish(outTopic_status, msg);
 
    //BME680 Pressure
    strcpy(outTopic_status,outTopic);
    dtostrf(bme680_gas,2,2,msg); 
    strcat(outTopic_status,"gas");
    client.publish(outTopic_status, msg);

     //IP Address
    strcpy(outTopic_status,outTopic);
    strcat(outTopic_status,"ip_address");
    WiFi.localIP().toString().toCharArray(msg,50);
    client.publish(outTopic_status,msg ); 

 
 }
 
 void reconnect() {
   // Loop until we're reconnected
   
   while (!client.connected()) {
     Serial.print("Attempting MQTT connection...");
     // Attempt to connect
     if (client.connect(mqtt_id)) {
       Serial.println("connected");
       
       client.publish(outTopic, "sensor node voc station booted");
       
       //send current Status via MQTT to world
       sendSensorValues();
       // ... and resubscribe
       client.subscribe(inTopic);
 
     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" try again in 5 seconds");      
       delay(5000);
     }
   }
 }
 
 void update_sensors() {
   bme680_temperature=bme680.temperature; //C
   bme680_pressure=bme680.pressure / 100.0F; //in hPA
   bme680_humidity=bme680.humidity; //%
   bme680_humidity=bme680.gas_resistance / 1000.0F; //kOhm

   //bme680_height=bme680.readAltitude(SEALEVELPRESSURE_HPA); //m

 }
 
 void setup() {
   // Status message will be sent to the PC at 115200 baud
   Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
   Serial.println("Sensor Node VOC Module");
   //INIT TIMERS
   timer_update_state_count=millis();

   //INIT bme680
   //SDA, SCL
   Serial.println("bme680 Init");
   //Wire.begin(BME_SDA, BME_SCL);
   bool status;
   status = bme680.begin();
   if (!status) {
       Serial.println("Could not find a valid bme680 sensor, check wiring!");
       delay(1000);
       while (1);
   }
   // Set up oversampling and filter initialization
   bme680.setTemperatureOversampling(BME680_OS_8X);
   bme680.setHumidityOversampling(BME680_OS_2X);
   bme680.setPressureOversampling(BME680_OS_4X);
   bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
   bme680.setGasHeater(320, 150); // 320*C for 150 ms
   update_sensors(); 
 
   //WIFI and MQTT
   setup_wifi();                   // Connect to wifi 
   client.setServer(mqtt_server, 1883);
   client.setCallback(callback);
 
  
 }
 
 
 void loop() {
   if (!client.connected()) {
     reconnect();
   }
   client.loop();
   
   
   update_sensors(); 
   
   //http Updater for OTA
   httpServer.handleClient(); 
 
   //send status update via MQTT every minute
   if(millis()-timer_update_state_count > timer_update_state) {
    //addLog_P(LOG_LEVEL_INFO, PSTR("Serial Timer triggerd."));
    timer_update_state_count=millis();
    sendSensorValues();
    
   }
   
 }
 