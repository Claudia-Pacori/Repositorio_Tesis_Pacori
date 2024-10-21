#include <WiFi.h>

// LoRa stuff
HardwareSerial LoRaSerial(2); // TX on 17, RX on 16
void sendParameter(int SF, int CR, int TP);

// Constants
const char *ssid = "ESP32_wifi";
WiFiServer server(80);

// Parameters and flags
int TPmin, TPmax, SFmin, SFmax, CRmin, CRmax;
String evaluationTime;
bool evaluating = false;

struct NodeData
{
    float current_mA;
    float temperature;
    float humidity;
};

void setup()
{
    Serial.begin(115200);
    LoRaSerial.begin(115200);

    WiFi.softAP(ssid);
    server.begin();
    Serial.println("\nAccess Point initialized");
    Serial.println(WiFi.softAPIP());

    Serial.println("\nInitializing...");
    initialize();
    Serial.println("\nINICIO");
}

void loop()
{
    WiFiClient client = server.available();
    if (client)
    {
        while (client.connected())
        {
            if (evaluating)
            {
                performEvaluation(client); // Perform evaluation within the same client connection
            }

            if (client.available())
            {
                String request = client.readStringUntil('\r');
                if (request.indexOf("GET") != -1)
                {
                    Serial.println(request);
                    processRequest(request, client); // Process the request
                    break;
                }
            }
        }
        client.stop();
    }
}

void performEvaluation(WiFiClient client)
{
    if (waitForConfirmation())
    {
        evaluationPage(client);
        delay(100);
        Serial.println("Confirmation received. Starting evaluation...");
        client.println("<p>Evaluation started. Sending initial packets...</p>");

        // sendParameter(SFmax, CRmax, TPmax);
        sendMessage("SF: " + String(9) + ", CR: " + String(4) + ", TP: " + String(22));
        sendParameter(9, 4, 22);
        send20Packets(client);

        unsigned long startTime = millis();
        while (millis() - startTime < convertTimeToMillis(evaluationTime))
        {
            if (LoRaSerial.available())
            {
                String response = LoRaSerial.readStringUntil('\n');
                if (response.indexOf("PARAMS:") != -1)
                {
                    // MANEJAR PARAMETROS DE INGRESO
                    sendMessage("SF: " + String(9) + ", CR: " + String(4) + ", TP: " + String(22));
                    sendParameter(9, 4, 22);
                    send20Packets(client);
                }
            }
        }
        safeConfig();
        sendMessage("FIN");
        client.println("<p>Evaluation finished.</p>");
        evaluating = false;
    }
    else
    {
        Serial.println("No confirmation received.");
        configurePage(client, "alert('Error: Confirmation not received.');");
        evaluating = false;
    }
}

unsigned long convertTimeToMillis(String time)
{
    if (time == "1min")
        return 60000;
    if (time == "10min")
        return 600000;
    if (time == "1hr")
        return 3600000;
    if (time == "5hrs")
        return 18000000;
    return 0;
}

void send20Packets(WiFiClient client)
{
    for (int i = 0; i < 20; i++)
    {
        delay(100);
        sendMessage("**************************************************"); // Send dummy data of 50 bytes
        client.println("<p>Packet " + String(i + 1) + " sent.</p>");
    }
}

void processRequest(String request, WiFiClient client)
{
    if (request.indexOf("set_ranges") > 0)
    {
        Serial.println("Setting ranges...");
        int pos;
        pos = request.indexOf("TPmin=");
        TPmin = request.substring(pos + 6, request.indexOf('&', pos)).toInt();
        pos = request.indexOf("TPmax=");
        TPmax = request.substring(pos + 6, request.indexOf('&', pos)).toInt();
        pos = request.indexOf("SFmin=");
        SFmin = request.substring(pos + 6, request.indexOf('&', pos)).toInt();
        pos = request.indexOf("SFmax=");
        SFmax = request.substring(pos + 6, request.indexOf('&', pos)).toInt();
        pos = request.indexOf("CRmin=");
        CRmin = request.substring(pos + 6, request.indexOf('&', pos)).toInt();
        pos = request.indexOf("CRmax=");
        CRmax = request.substring(pos + 6, request.indexOf('&', pos)).toInt();
        pos = request.indexOf("time=");
        evaluationTime = request.substring(pos + 5, request.indexOf(' ', pos));

        // If parameters are valid, set evaluating to true
        if (TPmin < TPmax && SFmin < SFmax && CRmin < CRmax)
        {
            Serial.println("Received Parameters:");
            Serial.print("TP: " + String(TPmin) + "-" + String(TPmax));
            Serial.print(", SF: " + String(SFmin) + "-" + String(SFmax));
            Serial.print(", CR: " + String(CRmin) + "-" + String(CRmax));
            Serial.println(", Time: " + evaluationTime + "\n");

            evaluating = true;
            safeConfig();
            sendMessage("START");
        }
        else
        {
            configurePage(client, "alert('Error: Min values should be less than Max values.');");
        }
    }
    else if (request.indexOf("stop_evaluation") > 0)
    {
        Serial.println("Stopping evaluation...");
        evaluating = false;
        sendMessage("CANCEL");
        configurePage(client, "alert('Evaluation stopped.');");
    }
    else
    {
        Serial.println("Default page...");
        configurePage(client, "");
    }
}