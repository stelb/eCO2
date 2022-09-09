#include <M5Unified.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_SHT31.h>

#include <Preferences.h>

#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "blink.h"
#include "colors.h"

// 12h+5m
#define TVOC_FIRSTSAVE 1000 * 60 * 60 * 12 + 5 * 60 * 1000
// 1h
#define TVOC_SAVEINTERVAL 1000 * 60 * 60

namespace sensor
{

    // prefs
    Preferences prefs;

    // sgp30 TVOC
    Adafruit_SGP30 sgp;

    // sht30
    bool sht30EnableHeater = false;
    Adafruit_SHT31 sht = Adafruit_SHT31();

    bool initialBaseline;
    unsigned long nextSave;
    uint16_t tvocBaseline, eco2Baseline;

    // mqtt
    WiFiClient espClient;
    PubSubClient mqttClient(espClient);
    String server;
    long port;

    // json
    StaticJsonDocument<512> config;
    StaticJsonDocument<128> values;

    // JsonArray arr;
    char json[512];
    char configTopic[70];
    char valueTopic[70];
    char stringBuffer[128];
    bool rc;

    uint32_t getAbsoluteHumidity(float temperature, float humidity)
    {
        // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
        const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
        const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                // [mg/m^3]
        return absoluteHumidityScaled;
    }

    void startsgp(boolean full = false)
    {
        sgp.softReset();
        sgp.begin();

        // stored prefs
        prefs.begin("eCO2");
        initialBaseline = prefs.getBool("init", false);
        Serial.println("got baseline");

        if (full)
        {
            Serial.println("reset baseline");
            initialBaseline = false;
            prefs.putBool("init", false);
        }
        if (initialBaseline)
        {
            // read existing baselines
            eco2Baseline = prefs.getUInt("eco2");
            tvocBaseline = prefs.getUInt("tvoc");
            // load into sensor
            sgp.setIAQBaseline(eco2Baseline, tvocBaseline);
            // save every hour
            nextSave = millis() + TVOC_SAVEINTERVAL;
        }
        else
        {
            // first baseline after 12h
            nextSave = millis() + TVOC_FIRSTSAVE; // now + 12h + 5min
        }
        prefs.end();
    }

    void setup(char *sensorID)
    {

        // M5
        M5.begin();
        // i2c
        Wire.begin();

        prefs.begin("mqtt");
        server = prefs.getString("server");
        port = prefs.getString("port").toInt();
        prefs.end();

        mqttClient.setServer(server.c_str(), port); //
        mqttClient.connect("eCO2");
        mqttClient.setBufferSize(512);

        // temperature config
        config["unit_of_measurement"] = "°C";
        config["device_class"] = "temperature";
        sprintf(stringBuffer, "homeassistant/sensor/%s/values", sensorID);
        config["state_topic"] = stringBuffer;
        sprintf(stringBuffer, "%s_T", sensorID);
        config["unique_id"] = stringBuffer;
        config["name"] = "Temperature";
        config["value_template"] = "{{ value_json.temperature }}";
        serializeJson(config, json);
        Serial.println(json);

        sprintf(stringBuffer, "homeassistant/sensor/%s_T/config", sensorID);
        rc = mqttClient.publish(stringBuffer, json);

        // humidity config
        config["unit_of_measurement"] = "%";
        config["device_class"] = "humidity";
        sprintf(stringBuffer, "%s_H", sensorID);
        config["unique_id"] = stringBuffer;
        config["name"] = "Humidity";
        config["value_template"] = "{{ value_json.humidity }}";
        serializeJson(config, json);
        Serial.println(json);

        sprintf(stringBuffer, "homeassistant/sensor/%s_H/config", sensorID);
        rc = mqttClient.publish(stringBuffer, json);

        // co2 config
        config["unit_of_measurement"] = "ppm";
        config["device_class"] = "carbon_dioxide";
        sprintf(stringBuffer, "%s_CO2", sensorID);
        config["unique_id"] = stringBuffer;
        config["name"] = "CO2";
        config["value_template"] = "{{ value_json.co2 }}";
        serializeJson(config, json);
        Serial.println(json);

        sprintf(stringBuffer, "homeassistant/sensor/%s_CO2/config", sensorID);
        rc = mqttClient.publish(stringBuffer, json);

        sprintf(valueTopic, "homeassistant/sensor/%s/values", sensorID);
        // SGP30 setup
        if (!sgp.begin())
        {
            Serial.println("Sensor not found :(");
            // do some error blinking
        }
        Serial.print("Found SGP30 serial #");
        Serial.print(sgp.serialnumber[0], HEX);
        Serial.print(sgp.serialnumber[1], HEX);
        Serial.println(sgp.serialnumber[2], HEX);

        startsgp();

        Serial.println("finished sgp setup");

        // sht setup
        if (!sht.begin(0x44))
        { // Set to 0x45 for alternate i2c addr
            Serial.println("Couldn't find SHT30");
        }
        else
        {
            Serial.print("Heater Enabled State: ");
            if (sht.isHeaterEnabled())
                Serial.println("ENABLED");
            else
                Serial.println("DISABLED");
        }
        Serial.println("finished sgp setup");
    }

    void publish(float t, float h, uint16_t co2)
    {
        char payload[60];

        values["temperature"] = (int)(t * 100 + 0.5) / 100.0;
        values["humidity"] = (int)(h * 100 + 0.5) / 100.0;
        values["co2"] = co2;
        serializeJson(values, payload);

        if (!mqttClient.connected())
        {
            mqttClient.connect("eCO2");
            mqttClient.setBufferSize(512);
        }
        rc = mqttClient.publish(valueTopic, payload);
    }

    void read()
    {

        float t = sht.readTemperature();
        float h = sht.readHumidity();

        if (!isnan(t))
        { // check if 'is not a number'
            Serial.print("Temp *C = ");
            Serial.print(t);
            Serial.print("\t\t");
        }
        else
        {
            Serial.println("Failed to read temperature");
        }

        if (!isnan(h))
        { // check if 'is not a number'
            Serial.print("Hum. % = ");
            Serial.println(h);
        }
        else
        {
            Serial.println("Failed to read humidity");
        }

        // if (!isnan(t) && !isnan(h))
        //{
        sgp.setHumidity(getAbsoluteHumidity(t, h));
        //}

        if (!sgp.IAQmeasure()) // tvoc/eco2
        {
            Serial.println("Measurement failed");
            return;
        }
        if (!sgp.IAQmeasureRaw()) // h2/ethanol
        {
            Serial.println("Raw Measurement failed");
            return;
        }

        // reset sensor on very high values
        if (sgp.eCO2 > 57000)
        {
            startsgp();
        }

        if (sgp.eCO2 >= 1000 & sgp.eCO2 < 1250)
        {
            blinkNS::set(GREEN, YELLOW, initialBaseline ? GREEN : BLUE);
        }
        else if (sgp.eCO2 >= 1250 & sgp.eCO2 < 1750)
        {
            blinkNS::set(YELLOW, YELLOW, initialBaseline ? YELLOW : BLUE);
        }
        else if (sgp.eCO2 >= 1750 & sgp.eCO2 < 2000)
        {
            blinkNS::set(ORANGE, ORANGE, initialBaseline ? ORANGE : BLUE);
        }
        else if (sgp.eCO2 >= 2000)
        {
            blinkNS::set(RED, RED, initialBaseline ? RED : BLUE);
        }
        else if (sgp.eCO2 < 1000)
        {
            blinkNS::set(GREEN, GREEN, initialBaseline ? GREEN : BLUE);
        }

        /*
                CO2 ppm < 1000 : grün
        1000 <= CO2 ppm < 1250 : grün-gelb
        1250 <= CO2 ppm < 1750: gelb
        1750 <= CO2 ppm < 2000: gelb-rot
        2000 <= CO2 ppm: rot
        */

        Serial.print("TVOC ");
        Serial.print(sgp.TVOC);
        Serial.print(" ppb\t");
        Serial.print("eCO2 ");
        Serial.print(sgp.eCO2);
        Serial.println(" ppm");

        if (millis() > nextSave)
        {
            // save baseline, calc next save
            if (!sgp.getIAQBaseline(&eco2Baseline, &tvocBaseline))
            {
                Serial.print("get baselines failed!"); // handling?
            }
            else
            {
                prefs.putUInt("eco2", eco2Baseline);
                prefs.putUInt("tvoc", tvocBaseline);
                // prefs.end();
                nextSave = millis() + TVOC_SAVEINTERVAL;
                // if first time: init true
                if (!initialBaseline)
                {
                    prefs.putBool("init", true);
                    initialBaseline = true;
                }
            }
        }
        Serial.printf("Next Save in %lu minutes\n", (nextSave - millis()) / 1000 / 60);
        Serial.print("Raw H2 ");
        Serial.print(sgp.rawH2);
        Serial.print(" \t");
        Serial.print("Raw Ethanol ");
        Serial.print(sgp.rawEthanol);
        Serial.println("");
        publish(t, h, sgp.eCO2);
    }

}