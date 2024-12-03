#include <WiFi.h>
#define LED 18

// LoRa Stuff
HardwareSerial LoRaSerial(2); // TX on 17, RX on 16
void setParameter(int SF, int CR, int TP);

// Constants
const char *ssid = "ESP32_node2";
WiFiServer server(80);

// Parameters
int TPmin, TPmax, SFmin, SFmax, CRmin, CRmax;
int TPeval = 22, SFeval = 9, CReval = 4;
bool evaluating = false;

float currentRSSI, currentSNR, currentPDR, currentConsumption;
String history = ""; // Buffer para el historial de actualizaciones

void setup()
{
    Serial.begin(115200);
    LoRaSerial.begin(115200);

    WiFi.softAP(ssid);
    server.begin();
    Serial.println("\nAccess Point initialized");
    Serial.println(WiFi.softAPIP());

    // Initial Configuration
    initialize();
}

void loop()
{
    WiFiClient client = server.available();

    if (LoRaSerial.available())
    {
        String command = LoRaSerial.readString();
        Serial.println("Received: " + command);
        processCommand(command);
    }

    if (evaluating)
    {
        listenForPackets(client);
    }
    if (client)
    {
        if (client.connected() && client.available())
        {
            String request = client.readStringUntil('\r');
            if (request.startsWith("GET"))
            {
                Serial.println(request);
                processRequest(request, client);
            }
        }
        client.stop();
    }
}

void processCommand(String command)
{
    if (command.indexOf("START") != -1)
    {
        String params = command.substring(16);

        // Dividir el string por comas
        int firstComma = params.indexOf(',');
        int secondComma = params.indexOf(',', firstComma + 1);

        String tpRange = params.substring(0, firstComma);
        String sfRange = params.substring(firstComma + 1, secondComma);
        String crRange = params.substring(secondComma + 1);

        // Obtener los valores min y max para cada parámetro
        TPmin = tpRange.substring(0, tpRange.indexOf('-')).toInt();
        TPmax = tpRange.substring(tpRange.indexOf('-') + 1).toInt();

        SFmin = sfRange.substring(0, sfRange.indexOf('-')).toInt();
        SFmax = sfRange.substring(sfRange.indexOf('-') + 1).toInt();

        CRmin = crRange.substring(0, crRange.indexOf('-')).toInt();
        CRmax = crRange.substring(crRange.indexOf('-') + 1).toInt();

        Serial.println("Start command received. Sending confirmation...");
        Serial.println("TP Range: " + String(TPmin) + " - " + String(TPmax));
        Serial.println("SF Range: " + String(SFmin) + " - " + String(SFmax));
        Serial.println("CR Range: " + String(CRmin) + " - " + String(CRmax));

        TPeval = TPmin;
        SFeval = SFmax;
        CReval = CRmin;
        sendConfirmation();
        evaluating = true;
    }
}

void listenForPackets(WiFiClient client)
{
    Serial.println("Listening for packets...\n");
    int packetCount = 0;
    float totalRSSI = 0.0;
    float totalSNR = 0.0;

    unsigned long startTime = millis();

    while (evaluating && (millis() - startTime < 15000))
    {
        if (LoRaSerial.available())
        {
            String command = LoRaSerial.readStringUntil('\n');
            startTime = millis();

            if (command.indexOf("PARAMS|") != -1)
            {
                String params = command.substring(command.indexOf("PARAMS|") + 7);
                int sfIndex = params.indexOf("SF");
                int crIndex = params.indexOf("|CR");
                int tpIndex = params.indexOf("|TP");

                SFeval = params.substring(sfIndex + 2, crIndex).toInt();
                CReval = params.substring(crIndex + 3, tpIndex).toInt();
                TPeval = params.substring(tpIndex + 3).toInt();
                safeConfig();
                setParameter(SFeval, CReval, TPeval);
            }
            else if (command.indexOf("*") != -1)
            {
                digitalWrite(LED, HIGH);
                int lastCommaIndex = command.lastIndexOf(',');
                int secondLastCommaIndex = command.lastIndexOf(',', lastCommaIndex - 1);

                float rssi = command.substring(secondLastCommaIndex + 1, lastCommaIndex).toFloat();
                float snr = command.substring(lastCommaIndex + 1).toFloat();

                totalRSSI += rssi;
                totalSNR += snr;
                packetCount++;
                digitalWrite(LED, LOW);
            }
            else if (command.indexOf("FINPAQUETES") != -1)
            {
                int startIndex = command.indexOf("FINPAQUETES") + String("FINPAQUETES").length();
                int endIndex = command.indexOf(",", startIndex);
                currentConsumption = command.substring(startIndex, endIndex).toFloat();

                if (packetCount > 0)
                {
                    currentRSSI = totalRSSI / packetCount;
                    currentSNR = totalSNR / packetCount;
                    currentPDR = packetCount / 20.0;
                }
                else
                {
                    currentRSSI = 0;
                    currentSNR = 0;
                    currentPDR = 0;
                }

                packetCount = 0;
                totalRSSI = 0.0;
                totalSNR = 0.0;

                evaluateNewParameters(currentRSSI, currentSNR, currentPDR, currentConsumption, client);
                sendConfirmation();
            }
            else if (command.indexOf("FIN") != -1)
            {
                Serial.println("Finish command received");
                evaluating = false;
                break;
            }
        }
        delay(10);
    }
    safeConfig();
    if ((millis() - startTime > 10000))
    {
        Serial.println("Time limit reached...");
        evaluating = false;
        return;
    }
}

// Variables globales para almacenar el mejor estado conocido
int bestTPeval = 0;
int bestSFeval = 0;
int bestCReval = 0;
float bestAvgPDR = 0;
int stableIterationCount = 0; // Contador de iteraciones sin mejora

// Función para evaluar nuevos parámetros
void evaluateNewParameters(float avgRSSI, float avgSNR, float avgPDR, float avgCurrent, WiFiClient client)
{
    String actualizacion = "TP: " + String(TPeval) + ", SF: " + String(SFeval) + ", CR: " + String(CReval) +
                           " | RSSI: " + String(avgRSSI) + " dBm, SNR: " + String(avgSNR) + " dB, PDR: " + String(avgPDR * 100) + "%, Current: " + String(avgCurrent) + " mA";
    history += "<p>" + actualizacion + "</p>";
    Serial.println(actualizacion);
    client.println("<p>" + actualizacion + "</p>");

    // Almacenar el mejor estado conocido si mejora el PDR
    if (avgPDR > bestAvgPDR || (avgPDR == 1 && bestAvgPDR >= 0.95))
    {
        bestTPeval = TPeval;
        bestSFeval = SFeval;
        bestCReval = CReval;
        bestAvgPDR = avgPDR;
        stableIterationCount = 0;
    }
    else
    {
        // Aumenta el contador si no hay mejora
        if (bestTPeval == TPeval && bestSFeval == SFeval && bestCReval == CReval) // modificar el best AVG si es que está probando con el mismo juego
        {
            bestAvgPDR = (bestAvgPDR + avgPDR) / 2;
        }
        stableIterationCount++;
    }

    // Revisar después de varias iteraciones sin mejora
    const int maxStableIterations = 3; // Número de iteraciones permitidas sin mejora significativa
    if (stableIterationCount >= maxStableIterations)
    {
        if (avgPDR < bestAvgPDR - 0.1) // Si el PDR es significativamente peor que el mejor
        {
            Serial.println("PDR ha empeorado significativamente. Restaurando la mejor configuración anterior.");
            TPeval = bestTPeval;
            SFeval = bestSFeval;
            CReval = bestCReval;
            return;
        }
        else if (bestTPeval == TPeval && bestSFeval == SFeval && bestCReval == CReval)
        {
            stableIterationCount = 0;
            Serial.println("PDR se ha mantenido. Manteniendo configuracion.");
            return;
        }
        else
        {
            Serial.println("PDR no mejora. Intentando un cambio significativo en los parámetros.");
            TPeval += 1; // Intento agresivo de mejora
            SFeval = constrain(SFeval + 1, SFmin, SFmax);
        }
        stableIterationCount = 0; // Resetear el contador tras decisión
    }

    // Define thresholds
    const float rssiThreshold = -70.0;
    const float snrThreshold = 5.0;
    const float pdrThreshold = 0.95; // Umbral para considerar PDR adecuado
    const float currentConsumptionThreshold = 100.0;

    bool rssiCondition = avgRSSI < rssiThreshold;
    bool snrCondition = avgSNR < snrThreshold;
    bool pdrCondition = avgPDR < pdrThreshold;
    bool currentCondition = avgCurrent > currentConsumptionThreshold;

    // Salir temprano si se alcanzan condiciones óptimas
    if (!rssiCondition && !snrCondition && !pdrCondition && !currentCondition)
    {
        Serial.println("Condiciones óptimas alcanzadas. Deteniendo ajustes.");
        return;
    }

    // Ajustar parámetros
    if (rssiCondition || snrCondition)
    {
        SFeval = constrain(SFeval + 1, SFmin, SFmax);
    }
    else
    {
        SFeval = constrain(SFeval - 1, SFmin, SFmax);
    }

    if (pdrCondition)
    {
        TPeval = constrain(TPeval + 1, TPmin, TPmax);
        CReval = constrain(CReval + 1, CRmin, CRmax);
    }
    else if (currentCondition)
    {
        TPeval = constrain(TPeval - 1, TPmin, TPmax);
        CReval = constrain(CReval - 1, CRmin, CRmax);
    }
    else
    {
        CReval = constrain(CReval - 1, CRmin, CRmax);
    }

    Serial.println("\nParámetros evaluados:");
    Serial.println("TP: " + String(TPeval) + ", SF: " + String(SFeval) + ", CR: " + String(CReval) + "\n\n");
}

void processRequest(String request, WiFiClient client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>Node Status</title>");
    client.println("<meta http-equiv='refresh' content='5'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; margin: 0; padding: 20px; }");
    client.println("h1 { color: #333; }");
    client.println(".history { background: #fff; padding: 15px; margin: 20px auto; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); max-width: 600px; text-align: left; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>LoRa Parameters and Metrics</h1>");
    client.println("<div class='history'>");
    client.println(history); // Muestra el historial completo
    client.println("</div>");
    client.println("</body>");
    client.println("</html>");
    client.stop();
}
