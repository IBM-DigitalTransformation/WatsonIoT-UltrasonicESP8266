#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>

// Configuration
#define TRIGGER_PIN D1
#define ECHO_PIN D0
#define WIFI_SSID "<wifi_ssid>"
#define WIFI_PASS "<wifi_pass>"
#define BUFFER_SIZE 500
#define WATSON_IOT_ORG_ID "<org_name>"
#define WATSON_IOT_DEVICE_TYPE "<device_type>"
#define WATSON_IOT_DEVICE_ID "<device_id>"
#define WATSON_IOT_TOKEN "<auth_token>"
#define WATSON_IOT_OUTPUT_EVENT_ID "distance"
#define WATSON_IOT_STATUS_EVENT_ID "status"

// Globals
long sequence = 0;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
const char *mqttHostname = WATSON_IOT_ORG_ID ".messaging.internetofthings.ibmcloud.com";
const char *mqttDeviceId = "d:" WATSON_IOT_ORG_ID ":" WATSON_IOT_DEVICE_TYPE ":" WATSON_IOT_DEVICE_ID;
const char *mqttUsername = "use-token-auth";
const char *mqttPassword = WATSON_IOT_TOKEN;
const char *mqttCommandTopic = "iot-2/cmd/+/fmt/+";
const char *mqttOutputTopic = "iot-2/evt/" WATSON_IOT_OUTPUT_EVENT_ID "/fmt/json";
const char *mqttStatusTopic = "iot-2/evt/" WATSON_IOT_STATUS_EVENT_ID "/fmt/txt";

void connect()
{
    while (!mqttClient.connected())
    {
        if (mqttClient.connect(mqttDeviceId, mqttUsername, mqttPassword))
        {
            // Create our Status Announcement
            String status = "(" + WiFi.macAddress() + "/" + WiFi.localIP() + ") " + WiFi.hostname();
            // Once connected, notify the Serial Monitor
            Serial.println("Connected to Watson IoT: " + status);
            // ... publish an announcement
            mqttClient.publish(mqttStatusTopic, status.c_str());
            // ... and resubscribe
            mqttClient.subscribe(mqttCommandTopic);
        }
        else
        {
            Serial.print("Connection to Watson IoT failed. Reason ");
            switch (mqttClient.state())
            {
            case MQTT_CONNECTION_TIMEOUT:
                Serial.print("(-4) MQTT_CONNECTION_TIMEOUT");
                break;
            case MQTT_CONNECTION_LOST:
                Serial.print("(-3) MQTT_CONNECTION_LOST");
                break;
            case MQTT_CONNECT_FAILED:
                Serial.print("(-2) MQTT_CONNECT_FAILED");
                break;
            case MQTT_DISCONNECTED:
                Serial.print("(-1) MQTT_DISCONNECTED");
                break;
            case MQTT_CONNECTED:
                Serial.print("(0) MQTT_CONNECTED");
                break;
            case MQTT_CONNECT_BAD_PROTOCOL:
                Serial.print("(1) MQTT_CONNECT_BAD_PROTOCOL");
                break;
            case MQTT_CONNECT_BAD_CLIENT_ID:
                Serial.print("(2) MQTT_CONNECT_BAD_CLIENT_ID");
                break;
            case MQTT_CONNECT_UNAVAILABLE:
                Serial.print("(3) MQTT_CONNECT_UNAVAILABLE");
                break;
            case MQTT_CONNECT_BAD_CREDENTIALS:
                Serial.print("(4) MQTT_CONNECT_BAD_CREDENTIALS");
                break;
            case MQTT_CONNECT_UNAUTHORIZED:
                Serial.print("(5) MQTT_CONNECT_UNAUTHORIZED");
                break;
            }

            Serial.println(" trying again in 5 seconds...2");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void publish(JsonObject &jsonObject)
{
    connect();
    char jsonString[BUFFER_SIZE];
    jsonObject.printTo(jsonString, sizeof(jsonString));
    mqttClient.publish(mqttOutputTopic, jsonString);
    Serial.println(jsonString);
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1')
    {
        digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
                                        // but actually the LED is on; this is because
                                        // it is acive low on the ESP-01)
    }
    else
    {
        digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
    }
}

// Called Once before loop()
void setup()
{
    // Setup our Serial Connection
    Serial.begin(9600);
    Serial.println();

    // Setup our GPIO Pins
    pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    pinMode(TRIGGER_PIN, OUTPUT); // Initialize the TRIGGER_PIN as an Output
    pinMode(ECHO_PIN, INPUT);     // Initialize the ECHO_PIN as an Input

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting WiFi to '");
    Serial.print(WIFI_SSID);
    Serial.print("'");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.hostname());

    // Configure MQTT
    mqttClient.setServer(mqttHostname, 1883);
    mqttClient.setCallback(callback);    
}

// Used for IoT Identification
void readWifiInfo(JsonObject &jsonObject)
{
    jsonObject["mac"] = WiFi.macAddress();
    jsonObject["rssi"] = WiFi.RSSI();
}

void readUltrasonicDetector(JsonObject &jsonObject)
{
    // Clears the trigPin
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    long duration = pulseIn(ECHO_PIN, HIGH);
    // Calculating the distance
    int distance = duration * 0.034 / 2;
    // Prints the distance on the Serial Monitor
    jsonObject["distance"] = distance;
}

void loop()
{
    jsonBuffer.clear();
    JsonObject &jsonObject = jsonBuffer.createObject();
    JsonObject &dataObject = jsonBuffer.createObject();
    jsonObject["d"] = dataObject;
    dataObject["sequence"] = sequence;
    readWifiInfo(dataObject);
    readUltrasonicDetector(dataObject);
    publish(jsonObject);
    sequence += 1;
}
