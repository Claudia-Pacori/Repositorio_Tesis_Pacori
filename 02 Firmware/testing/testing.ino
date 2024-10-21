// Libraries
#include <WiFi.h> //https://www.arduino.cc/en/Reference/WiFi

// Constants
#define LED 18

// Parameters
String request;
String receivedMessages = "";
const char *ssid = "ESP32_Local_Network"; // Nombre de la red local
String nom = "ESP32";

// Objects
WiFiServer server(80);
WiFiClient client;
HardwareSerial LoRaSerial(1); // Definir LoRaSerial con el hardware serial 1

void setup()
{
    // Init Serial USB
    Serial.begin(115200);
    Serial.println(F("Initialize System"));

    // Init LoRa Serial
    LoRaSerial.begin(115200, SERIAL_8N1, 16, 17); // RX en pin 16, TX en pin 17
    Serial.println(F("LoRa Serial initialized"));

    // Init ESP32 as Access Point (AP mode) without password
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
            webpage(client); // Return webpage with updated messages
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

        // Actualizar la variable de mensajes con lo que recibe de LoRa
        receivedMessages += "<p><b>Received:</b> " + loRaMessage + "</p>";
    }
}

String urlDecode(String input) {
    String decoded = "";
    char temp[] = "00";
    unsigned int len = input.length();
    unsigned int i = 0;

    while (i < len) {
        if (input[i] == '%') {
            if (i + 2 < len) {
                temp[0] = input[i + 1];
                temp[1] = input[i + 2];
                decoded += (char) strtol(temp, NULL, 16);
                i += 3;
            }
        }
        else if (input[i] == '+') {
            decoded += ' ';
            i++;
        }
        else {
            decoded += input[i];
            i++;
        }
    }
    return decoded;
}


void handleRequest(String request, WiFiClient client)
{
    // Handle web client request
    if (request.indexOf("/dig0on") > 0)
    {
        digitalWrite(LED, HIGH);
    }
    if (request.indexOf("/dig0off") > 0)
    {
        digitalWrite(LED, LOW);
    }

    // Handle LoRa command sent from webpage
    if (request.indexOf("/send?msg=") > 0)
    {
        // Extraer el mensaje desde el parámetro 'msg=' y decodificar
        String encodedMsg = request.substring(request.indexOf("msg=") + 4);
        String decodedMsg = urlDecode(encodedMsg); // Decodificar el mensaje

        // Split para obtener solo el primer comando
        int spaceIndex = decodedMsg.indexOf(' ');
        String command = (spaceIndex == -1) ? decodedMsg : decodedMsg.substring(0, spaceIndex);

        // Enviar solo el comando al LoRa
        LoRaSerial.println(command);

        // Actualizar la variable de mensajes con el comando enviado
        receivedMessages += "<p><b>Sent:</b> " + command + "</p>";

        // Confirmar que el comando fue enviado
        client.println("<p>Command sent to LoRa: " + command + "</p>");
    }
}

void webpage(WiFiClient client)
{
    // Send redesigned webpage to client
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>ESP32 Web Controller</title>");

    // Meta tag for mobile devices
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");

    // CSS for styling the webpage
    client.println("<style>");
    client.println("body { background-color: #f4f4f4; font-family: Arial, sans-serif; color: #333; text-align: center; }");
    client.println("h1 { color: #2E8B57; }");
    client.println(".button { display: inline-block; padding: 15px 30px; font-size: 24px; margin: 10px; cursor: pointer; color: white; background-color: #008CBA; border: none; border-radius: 10px; }");
    client.println(".button:hover { background-color: #005f73; }");
    client.println(".send-box { padding: 10px; font-size: 18px; width: 80%; margin: 20px auto; border-radius: 10px; border: 1px solid #333; }");
    client.println(".send-btn { padding: 10px 20px; font-size: 18px; cursor: pointer; background-color: #3AAA35; color: white; border: none; border-radius: 10px; }");
    client.println(".message-box { padding: 20px; font-size: 18px; width: 80%; margin: 20px auto; background-color: #fff; border-radius: 10px; border: 1px solid #333; }");
    client.println("</style>");

    client.println("</head>");
    client.println("<body>");
    client.println("<h1>ESP32 Web Controller - " + nom + "</h1>");
    client.println("<hr/>");

    // Digital Outputs Section
    client.println("<h2>Control LED</h2>");
    client.println("<a href='/dig0on'><button class='button'>Turn On LED</button></a>");
    client.println("<a href='/dig0off'><button class='button'>Turn Off LED</button></a>");
    client.println("<hr/>");

    // LoRa Command Section
    client.println("<h2>Send Command to LoRa</h2>");
    client.println("<form action='/send'>");
    client.println("<input type='text' name='msg' class='send-box' placeholder='Type your command here...'>");
    client.println("<input type='submit' value='Send' class='send-btn'>");
    client.println("</form>");
    client.println("<hr/>");

    // LoRa Messages Section
    client.println("<h2>Received Messages</h2>");
    client.println("<div class='message-box'>");
    client.println(receivedMessages); // Mostrar los mensajes enviados/recibidos
    client.println("</div>");

    client.println("</body>");
    client.println("</html>");
    client.println();
}