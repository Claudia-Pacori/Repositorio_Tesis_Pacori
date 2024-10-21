#include <Wire.h>
#include "Adafruit_SHT4x.h"
#include "Adafruit_INA219.h"

// Create instances of the sensors
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_INA219 ina219;

// Function to initialize I2C
void beginI2C()
{
    Wire.begin();                 // Start I2C on core 1
    Wire.beginTransmission(0x15); // Dummy address
    int endValue = Wire.endTransmission();
    if (endValue == 2)
        Serial.println("Success: I2C Bus OK");
    else
        Serial.println("Error: I2C Bus Not Responding");
}

// Function to initialize sensors
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

// Function to read current
NodeData readCurrent() 
{
    NodeData nodeData; // Create an instance of NodeData
    
    float busvoltage = 0;
    busvoltage = ina219.getBusVoltage_V();
    float current = ina219.getCurrent_mA();
    nodeData.current_mA = (current < 0) ? -current : current;    
    float power_mW = ina219.getPower_mW();

    /*
    Serial.print("\nVoltage: ");
    Serial.print(busvoltage);
    Serial.print("V, Current: ");
    Serial.print(nodeData.current_mA);
    Serial.print("mA, Power: ");
    Serial.print(power_mW);
    Serial.println("mW");
    */
    return nodeData;
}

// Function to read temperature and humidity
NodeData readTH(NodeData nodeData)
{
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    /*
    Serial.print("\n\nTemperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degrees C");
    Serial.print("Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println("% rH");
    */
    nodeData.temperature = temp.temperature; // Store temperature in the struct
    nodeData.humidity = humidity.relative_humidity; // Store humidity in the struct
    return nodeData;
}

// Function to get combined NodeData
NodeData getNodeData()
{
    NodeData nodeData = readCurrent(); // Read current data
    nodeData = readTH(nodeData); // Read temperature and humidity
    return nodeData;
}