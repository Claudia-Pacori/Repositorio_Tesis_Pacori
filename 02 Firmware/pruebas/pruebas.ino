// Libraries
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"
#include "Adafruit_INA219.h"

// Constants
#define LED 18

// Parameters
String request;
String receivedMessages = "";
const char *ssid = "ESP32_Local_Network";

struct NodeData
{
    float current_mA;
    float temperature;
    float humidity;
};

// Objects
WiFiServer server(80);
WiFiClient client;
HardwareSerial LoRaSerial(1);
Adafruit_SHT4x sht4;
Adafruit_INA219 ina219;

// Function prototypes
void beginI2C();
void beginSensors();
NodeData getNodeData();

void setup()
{
    // Init Serial USB
    Serial.begin(115200);
    Serial.println(F("Initialize System"));

    // Init LoRa Serial
    LoRaSerial.begin(115200, SERIAL_8N1, 16, 17);
    Serial.println(F("LoRa Serial initialized"));

    // Set up sensors
    beginI2C();
    beginSensors();

    // Init ESP32 as Access Point
    Serial.println("Setting up Access Point...");
    WiFi.softAP(ssid);

    // Start the server
    server.begin();
    Serial.println(F("Access Point initialized"));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.softAPIP());

    pinMode(LED, OUTPUT);
}

void loop()
{
    // Listen for incoming clients
    WiFiClient client = server.available();
    if (client)
    {
        while (client.connected())
        {
            if (client.available())
            {
                String request = client.readStringUntil('\r');
                Serial.println(request);
                handleRequest(request, client);
            }
            webpage(client);
            break;
        }
        client.stop();
    }

    // Leer y mostrar mensajes recibidos vía LoRa Serial
    if (LoRaSerial.available())
    {
        String loRaMessage = LoRaSerial.readString();
        Serial.print("Received from LoRa: ");
        Serial.println(loRaMessage);
        receivedMessages += "<p><b>Received:</b> " + loRaMessage + "</p>";
    }
}

String urlDecode(String input)
{
    String decoded = "";
    char temp[] = "00";
    unsigned int len = input.length();
    unsigned int i = 0;

    while (i < len)
    {
        if (input[i] == '%')
        {
            if (i + 2 < len)
            {
                temp[0] = input[i + 1];
                temp[1] = input[i + 2];
                decoded += (char)strtol(temp, NULL, 16);
                i += 3;
            }
        }
        else if (input[i] == '+')
        {
            decoded += ' ';
            i++;
        }
        else
        {
            decoded += input[i];
            i++;
        }
    }
    return decoded;
}

void beginI2C()
{
    Wire.begin();
    Wire.beginTransmission(0x15);
    int endValue = Wire.endTransmission();
    if (endValue == 2)
        Serial.println("Success: I2C Bus OK");
    else
        Serial.println("Error: I2C Bus Not Responding");
}

void beginSensors()
{
    Serial.println("Adafruit SHT4x test");
    if (!sht4.begin())
    {
        Serial.println("Couldn't find SHT4x");
        while (1)
            delay(1);
    }
    Serial.println("Found SHT4x sensor");
    Serial.print("Serial number 0x");
    Serial.println(sht4.readSerial(), HEX);
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);

    if (!ina219.begin())
    {
        Serial.println("Failed to find INA219 chip");
        while (1)
        {
            delay(10);
        }
    }
}

NodeData getNodeData()
{
    NodeData nodeData;
    float busvoltage = ina219.getBusVoltage_V();
    float current = ina219.getCurrent_mA();
    nodeData.current_mA = (current < 0) ? -current : current;
    float power_mW = ina219.getPower_mW();

    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    nodeData.temperature = temp.temperature;
    nodeData.humidity = humidity.relative_humidity;

    return nodeData;
}

void handleRequest(String request, WiFiClient client)
{
    if (request.indexOf("/dig0on") > 0)
    {
        digitalWrite(LED, HIGH);
    }
    if (request.indexOf("/dig0off") > 0)
    {
        digitalWrite(LED, LOW);
    }

    if (request.indexOf("/send?msg=") > 0)
    {
        String encodedMsg = request.substring(request.indexOf("msg=") + 4);
        String decodedMsg = urlDecode(encodedMsg);
        int spaceIndex = decodedMsg.indexOf(' ');
        String command = (spaceIndex == -1) ? decodedMsg : decodedMsg.substring(0, spaceIndex);
        LoRaSerial.println(command);
        receivedMessages += "<p><b>Sent:</b> " + command + "</p>";
    }
}

void webpage(WiFiClient client)
{
    NodeData nodeData = getNodeData();

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>Hardware testing</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<style>");
    client.println("body { background-color: #f4f4f4; font-family: Arial, sans-serif; color: #333; text-align: center; }");
    client.println(".button { display: inline-block; padding: 15px 30px; font-size: 12px; margin: 10px; cursor: pointer; color: white; background-color: #008CBA; border: none; border-radius: 10px; }");
    client.println(".button:hover { background-color: #005f73; }");
    client.println(".send-box { padding: 10px; font-size: 12px; width: 80%; margin: 20px auto; border-radius: 10px; border: 1px solid #333; }");
    client.println(".send-btn { padding: 10px 20px; font-size: 12px; cursor: pointer; background-color: #3AAA35; color: white; border: none; border-radius: 10px; }");
    client.println(".message-box { padding: 20px; font-size: 12px; width: 80%; margin: 20px auto; background-color: #fff; border-radius: 10px; border: 1px solid #333; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Hardware testing</h1>");
    client.println("<hr/>");

    client.println("<h2>Sensor Readings</h2>");
    client.println("<p><b>Temperature:</b> " + String(nodeData.temperature) + " &deg;C</p>");
    client.println("<p><b>Humidity:</b> " + String(nodeData.humidity) + " %</p>");
    client.println("<p><b>Current:</b> " + String(nodeData.current_mA) + " mA</p>");

    client.println("<h2>Send Command to LoRa</h2>");
    client.println("<form action='/send'>");
    client.println("<input type='text' name='msg' class='send-box' placeholder='Type your command here...'>");
    client.println("<input type='submit' value='Send' class='send-btn'>");
    client.println("</form>");
    client.println("<div class='message-box'>");
    client.println(receivedMessages);
    client.println("</div>");

    client.println("<h2>Control LED</h2>");
    client.println("<a href='/dig0on'><button class='button'>ON</button></a>");
    client.println("<a href='/dig0off'><button class='button'>OFF</button></a>");

    client.println("</body>");
    client.println("</html>");
    client.println();
}