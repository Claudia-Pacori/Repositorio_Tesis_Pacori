#define AuxBUTpin 13
#define AuxLEDpin 18

void initialize()
{
    beginI2C();
    beginLoRa();
    beginSensors();

    pinMode(AuxBUTpin, INPUT);
    pinMode(AuxLEDpin, OUTPUT);
    digitalWrite(AuxLEDpin, LOW);
}

// Send initial HTML page to client
void configurePage(WiFiClient client, String jsAlert)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>Range Selector</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; text-align: center; }");
    client.println(".form-group { display: flex; justify-content: center; align-items: center; margin-top: 10px; }");
    client.println("select, input[type='submit'] { margin: 5px; padding: 5px; font-size: 16px; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Range Selection</h1>");
    if (!jsAlert.isEmpty())
    {
        client.println("<script>" + jsAlert + "</script>");
    }
    client.println("<form action='/set_ranges' method='GET'>");
    client.println("<h3>Transmission Power</h3>");
    client.println("<div class='form-group'>");
    client.println("Min: <select name='TPmin'>");
    for (int i = 0; i <= 22; i++)
        client.println("<option value='" + String(i) + "'>" + String(i) + "</option>");
    client.println("</select>");
    client.println("Max: <select name='TPmax'>");
    for (int i = 0; i <= 22; i++)
        client.println("<option value='" + String(i) + "'" + (i == 22 ? " selected" : "") + ">" + String(i) + "</option>");
    client.println("</select>");
    client.println("</div>");
    client.println("<h3>Spreading Factor</h3>");
    client.println("<div class='form-group'>");
    client.println("Min: <select name='SFmin'>");
    for (int i = 7; i <= 11; i++)
        client.println("<option value='" + String(i) + "'>" + String(i) + "</option>");
    client.println("</select>");
    client.println("Max: <select name='SFmax'>");
    for (int i = 7; i <= 11; i++)
        client.println("<option value='" + String(i) + "'" + (i == 11 ? " selected" : "") + ">" + String(i) + "</option>");
    client.println("</select>");
    client.println("</div>");
    client.println("<h3>Coding Rate</h3>");
    client.println("<div class='form-group'>");
    client.println("Min: <select name='CRmin'>");
    for (int i = 1; i <= 4; i++)
        client.println("<option value='" + String(i) + "'>" + String(i) + "</option>");
    client.println("</select>");
    client.println("Max: <select name='CRmax'>");
    for (int i = 1; i <= 4; i++)
        client.println("<option value='" + String(i) + "'" + (i == 4 ? " selected" : "") + ">" + String(i) + "</option>");
    client.println("</select>");
    client.println("</div>");
    client.println("<h3>Evaluation Time</h3>");
    client.println("<div class='form-group'>");
    client.println("<select name='time'>");
    client.println("<option value='1min'>1 Minute</option>");
    client.println("<option value='10min'>10 Minutes</option>");
    client.println("<option value='1hr'>1 Hour</option>");
    client.println("<option value='5hrs'>5 Hours</option>");
    client.println("</select>");
    client.println("</div>");
    client.println("<br>");
    client.println("<input type='submit' value='Start Evaluation'>");
    client.println("</form>");

    if (evaluating)
    {
        client.println("<br>");
        client.println("<form action='/stop_evaluation' method='GET'>");
        client.println("<input type='submit' value='Stop Evaluation'>");
        client.println("</form>");
    }

    client.println("</body>");
    client.println("</html>");
}

// Send evaluation page
void evaluationPage(WiFiClient client)
{
    NodeData nodeData = getNodeData(); // Get current, temperature, and humidity data

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>Evaluation</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; text-align: center; }");
    client.println("table { margin: auto; border-collapse: collapse; width: 80%; }");
    client.println("th, td { border: 1px solid black; padding: 8px; text-align: center; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Evaluation</h1>");
    client.println("<table>");
    client.println("<tr><th>Parameter</th><th>Value</th></tr>");
    client.println("<tr><td>Current</td><td>" + String(nodeData.current_mA) + " mA</td></tr>");
    client.println("<tr><td>Temperature</td><td>" + String(nodeData.temperature) + " °C</td></tr>");
    client.println("<tr><td>Humidity</td><td>" + String(nodeData.humidity) + " % rH</td></tr>");
    client.println("<tr><td>TP range</td><td>" + String(TPmin) + " - " + String(TPmax) + " dB</td></tr>");
    client.println("<tr><td>SF range</td><td>" + String(SFmin) + " - " + String(SFmax) + "</td></tr>");
    client.println("<tr><td>CR range </td><td>" + String(CRmin) + " - " + String(CRmax) + "</td></tr>");
    client.println("<tr><td>Evaluation Time</td><td>" + evaluationTime + "</td></tr>");
    client.println("</table>");
    client.println("<br>");
    client.println("<form action='/stop_evaluation' method='GET'>");
    client.println("<input type='submit' value='Stop Evaluation'>");
    client.println("</form>");
    client.println("</body>");
    client.println("</html>");
}

// Function to wait for confirmation
bool waitForConfirmation()
{
    unsigned long startTime = millis();
    while (millis() - startTime < 5000)
    {
        if (LoRaSerial.available())
        {
            String response = LoRaSerial.readString();
            if (response.indexOf("CONFIRM") > 0)
            {
                return true;
            }
        }
    }
    return false;
}