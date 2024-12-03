#include <WiFi.h>
#define LED 18

// LoRa stuff
HardwareSerial LoRaSerial(2); // TX on 17, RX on 16
void setParameter(int SF, int CR, int TP);

float soloCorriente();
float sendLoRa2(String comando, bool mostrar);

// Constants
const char *ssid = "ESP32_node1";
WiFiServer server(80);

// Parameters and flags
int TPmin, TPmax, SFmin, SFmax, CRmin, CRmax;
int TPeval, SFeval, CReval;
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
        Serial.println("Confirmation received. Starting evaluation...");
        evaluationPage(client);

        TPeval = TPmin;
        SFeval = SFmax;
        CReval = CRmin;

        safeConfig();
        sendMessage("PARAMS|SF" + String(SFeval) + "|CR" + String(CReval) + "|TP" + String(TPeval));
        sendMessage("PARAMS|SF" + String(SFeval) + "|CR" + String(CReval) + "|TP" + String(TPeval));
        sendMessage("PARAMS|SF" + String(SFeval) + "|CR" + String(CReval) + "|TP" + String(TPeval));
        setParameter(SFeval, CReval, TPeval);
        send20Packets(client);

        unsigned long startTime = millis();
        while (millis() - startTime < convertTimeToMillis(evaluationTime))
        {
            if (LoRaSerial.available())
            {
                String response = LoRaSerial.readString();
                if (response.indexOf("CONFIRM|") != -1)
                {
                    // MANEJAR PARAMETROS DE INGRESO
                    Serial.println("Recibi: " + response);
                    String params = response.substring(response.indexOf("CONFIRM|") + 8);
                    int tpIndex = params.indexOf("TP");
                    int sfIndex = params.indexOf("|SF");
                    int crIndex = params.indexOf("|CR");

                    TPeval = params.substring(tpIndex + 2, sfIndex).toInt();
                    SFeval = params.substring(sfIndex + 3, crIndex).toInt();
                    CReval = params.substring(crIndex + 3).toInt();
                    safeConfig();
                    sendMessage("PARAMS|SF" + String(SFeval) + "|CR" + String(CReval) + "|TP" + String(TPeval));
                    sendMessage("PARAMS|SF" + String(SFeval) + "|CR" + String(CReval) + "|TP" + String(TPeval));
                    sendMessage("PARAMS|SF" + String(SFeval) + "|CR" + String(CReval) + "|TP" + String(TPeval));
                    setParameter(SFeval, CReval, TPeval);
                    send20Packets(client);
                }
            }
        }
        safeConfig();
        delay(150);
        sendMessage("FIN");
        sendMessage("FIN");
        sendMessage("FIN");
        resultsPage(client);
        evaluating = false;
    }
    else
    {
        Serial.println("No confirmation received.");
        evaluating = false;
        configurePage(client, "alert('Error: Confirmation not received.');");
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
    float totalCurrent = 0.0;
    float totalTemperature = 0.0;
    float totalHumidity = 0.0;

    for (int i = 0; i < 20; i++)
    {
        digitalWrite(LED, HIGH);
        delay(150);
        digitalWrite(LED, LOW);
        totalCurrent += sendLoRa2("AT+SEND=2,50,**************************************************", true); // Send dummy data of 50 bytes
        NodeData nodeData = getNodeData(); // Get current, temperature, and humidity data
        totalTemperature += nodeData.temperature;
        totalHumidity += nodeData.humidity;        
        Serial.print(".");
        // client.println("<p>Packet " + String(i + 1) + " sent.</p>");
    }

    float avgCurrent = totalCurrent / 20;
    float avgTemperature = totalTemperature / 20;
    float avgHumidity = totalHumidity / 20;

    delay(150);
    sendMessage("FINPAQUETES" + String(avgCurrent));
    sendMessage("FINPAQUETES" + String(avgCurrent));
    sendMessage("FINPAQUETES" + String(avgCurrent));
    safeConfig();

    Serial.println("\nAverage: Current = " + String(avgCurrent) + " mA, Temperature = " +
                   String(avgTemperature) + " °C, Humidity = " + String(avgHumidity) + " % rH");
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
            String startMessage = "START," + String(TPmin) + "-" + String(TPmax) + "," + String(SFmin) + "-" + String(SFmax) + "," + String(CRmin) + "-" + String(CRmax);
            sendMessage(startMessage);
            sendMessage(startMessage);
            sendMessage(startMessage);
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