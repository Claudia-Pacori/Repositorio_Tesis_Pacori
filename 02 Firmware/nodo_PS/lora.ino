String myAddress = "1";
String otherAddress = "2";

void sendLoRa(String comando, bool mostrar = false)
{
    if (mostrar)
    {
        Serial.println(comando);

        LoRaSerial.print(comando + "\r\n");
        unsigned long startTime = millis();

        while (millis() - startTime < 100) // verify answer from the module
        {
            if (LoRaSerial.available())
            {
                String response = LoRaSerial.readString();
                Serial.println("Response: " + response);
                break;
            }
        }
    }
    else
    {
        LoRaSerial.print(comando + "\r\n");
    }
    // delay(100);
}

void sendMessage(String message)
{
    sendLoRa("AT+SEND=" + otherAddress + "," + String(message.length()) + "," + message, true);
}

void sendParameter(int SF = 9, int CR = 4, int TP = 22)
{
    sendLoRa("AT+PARAMETER=" + String(SF) + ",9," + String(CR) + ",12", true); // SF, 500 KHz, CR, Preamble 12
    sendLoRa("AT+CRFOP=" + String(TP), true);
}

void safeConfig()
{
    sendParameter();
    Serial.println("\nSafe configuration established.");
}

void beginLoRa()
{
    sendLoRa("AT+ADDRESS=" + myAddress, true); // NODO PS ADDRESS: 1
    sendLoRa("AT+NETWORKID=1", true);
    safeConfig();
}