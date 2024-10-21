HardwareSerial LoRaSerial(2);

void setup() {
    // Initialize the main serial port (USB)
    Serial.begin(115200);
    
    // Initialize the Software Serial port
    LoRaSerial.begin(115200);
}

void loop() {
    // Read from Serial and send to Software Serial
    if (Serial.available()) {
        char c = Serial.read();
        LoRaSerial.write(c);
    }

    // Read from Software Serial and send to Serial
    if (LoRaSerial.available()) {
        char c = LoRaSerial.read();
        Serial.write(c);
    }
}