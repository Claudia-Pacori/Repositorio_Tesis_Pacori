String myAddress = "1";
String otherAddress = "2";


void sendLoRa(String comando, bool mostrar = false)
{
    if (mostrar)
    {
        //Serial.println(comando);
        LoRaSerial.print(comando + "\r\n");
        unsigned long startTime = millis();

        while (millis() - startTime < 300) // verify answer from the module
        {
            if (LoRaSerial.available())
            {
                String response = LoRaSerial.readString();
                //Serial.println("Response: " + response);
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

void setParameter(int SF = 9, int CR = 4, int TP = 22)
{
    sendLoRa("AT+PARAMETER=" + String(SF) + ",9," + String(CR) + ",12", true); // SF, 500 KHz, CR, Preamble 12
    sendLoRa("AT+CRFOP=" + String(TP), true);
}

float sendLoRa2(String comando, bool mostrar = false)
{
    unsigned long startTime = millis(); // Para medir duración del envío
    float currentSum = 0.0;
    int readings = 0;

    if (mostrar)
    {
        LoRaSerial.print(comando + "\r\n");
        
        while (millis() - startTime < 300) // Esperar respuesta del módulo
        {
            if (LoRaSerial.available())
            {
                String response = LoRaSerial.readString();
                break;
            }

            // Leer corriente mientras espera
            float current_mA = soloCorriente();
            currentSum += current_mA;
            readings++;
        }
    }
    else
    {
        LoRaSerial.print(comando + "\r\n");

        while (millis() - startTime < 100) // Agregamos una breve espera por consistencia
        {
            // Leer corriente durante envío en modo silencioso
            float current_mA = soloCorriente();
            currentSum += current_mA;
            readings++;
        }
    }

    // Calcular y mostrar promedio de corriente para este comando
    if (readings > 0)
    {
        float avgCurrent = currentSum / readings;
        return avgCurrent;
    }
    return 0;
}

void safeConfig()
{
    setParameter();
    //Serial.println("\nSafe configuration established.");
}

void beginLoRa()
{
    sendLoRa("AT+ADDRESS=" + myAddress, true); // NODO PS ADDRESS: 1
    sendLoRa("AT+NETWORKID=1", true);
    safeConfig();
}