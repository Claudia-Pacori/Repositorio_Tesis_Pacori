#define AuxBUTpin 13
#define AuxLEDpin 18

void initialize()
{
    beginLoRa();

    pinMode(AuxBUTpin, INPUT);
    pinMode(AuxLEDpin, OUTPUT);
    digitalWrite(AuxLEDpin, LOW);
}