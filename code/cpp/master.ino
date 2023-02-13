#include <Wire.h> // подключаем библиотеку
#include <DHT.h>
// #include <RtcDS3231.h>
#include "defs.h"

DHT dht_0(ARDUINO0_D10, DHTTYPE);
DHT dht_1(ARDUINO0_D11, DHTTYPE);

// bool temperatureOK false;

uint8_t x = 0;

float temperature_0 = 0;
float temperature_1 = 0;
float humidity_0 = 0;
float humidity_1 = 0;

void setup()
{
    digitalWrite(LED_BUILTIN, HIGH); // Arduino waked up
    Wire.begin(0);                   // Master
    Wire.onReceive(receiveEvent);    // Get events
    Wire.onRequest(sendEvent);
    while (!Serial)
        ;               // Leonardo: wait for serial monitor
    Serial.begin(9600); // TODO: c cahnger to 115200 after add VCC ort 10 kOm resistor to DCL SDA lines
    Serial.println("\nI2C Scanner");
    dht_0.begin();
    dht_1.begin();
}

void scanner()
{
    int nDevices = 0;

    Serial.println("Scanning...");

    for (byte address = 1; address < 127; ++address)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
            {
                Serial.print("0");
            }
            Serial.print(address, HEX);
            Serial.println("  !");

            ++nDevices;
        }
        else if (error == 4)
        {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
            {
                Serial.print("0");
            }
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
    {
        Serial.println("No I2C devices found\n");
    }
    else
    {
        Serial.println("done\n");
    }
    delay(5000); // Wait 5 seconds for next scan
}

void loop()
{

    scanner();
    // turn the LED on (HIGH is the voltage level)
    Wire.beginTransmission(ARDUINO1_I2C_ADDRESS); // начало передачи на устройство номер 1

    Wire.write("W");        // отправляем цепочку текстовых байт
    Wire.write(x);          // отправляем байт из переменной
    Wire.endTransmission(); // останавливаем передачу
    x++;                    // увеличиваем значение переменной на 1

    byte temperatureOK = temperature();
    if (temperatureOK == 1)
    {
    }

    delay(5000); // ждем 2 секунды
}

byte temperature()
{

    temperature_0 = dht_0.readTemperature();
    temperature_1 = dht_1.readTemperature();
    humidity_0 = dht_0.readHumidity();
    humidity_1 = dht_1.readHumidity();

    if (isnan(humidity_0) || isnan(temperature_0))
    {
        Serial.println("Failed to read from DHT sensor #1 !");
        return 0;
    }
    if (isnan(humidity_1) || isnan(temperature_1))
    {
        Serial.println("Failed to read from DHT sensor #1 !");
        return 0;
    }

    Serial.print("Humidity #0: ");
    Serial.print(humidity_0);
    Serial.print(" %\t");
    Serial.print("Temperature #0: ");
    Serial.print(temperature_0);
    Serial.println(" *C ");

    Serial.print("Humidity #1: ");
    Serial.print(humidity_1);
    Serial.print(" %\t");
    Serial.print("Temperature #1: ");
    Serial.print(temperature_1);
    Serial.println(" *C ");

    return 1;
}

void sendEvent()
{
}

void receiveEvent(int howMany)
{
    while (1 < Wire.available()) // Bytes in message
    {
        uint8_t c = Wire.read(); // read char
        Serial.print(c);         // print char
    }
    int x = Wire.read(); // read int
    Serial.println(x);   // Print it
}

// structure of events
// id of