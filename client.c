#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_SHT31.h>
#include <MQ135.h>
#include <ArduinoJson.h>

#define EXHAUST_PIN    		2u
#define HUMIDIFIER_PIN 		3u
#define LIGHT_PIN      		6u
#define HEATER_PIN     		8u
#define MQ135_INT_PIN	    A3
#define MQ135_EXT_PIN	    A4
#define SHT31_INT_ADDR 	    0x44u
//#define SHT31_EXT_ADDR 	0x45u

#define READ_MS 	 		1000u
#define SERIALIZE_MS 	 	1000u
#define PERIPHERALS_MS	 	1000u
#define AUTO_MS		 	 	1000u
#define AUTO_CLIMATE_MS		1000u * 15u

#define EEPROM_ADDR 		0u

#define HEATER_MAX_SEC		2u * 60u
#define HEATER_COOLDOWN_SEC	1u * 60u

#define RX_BUF_SIZE     	256u
#define RX_TIMEOUT_MS   	3000u

const float targetTemperatures[] = {26.5f, 20.f};
const float targetHumidities[] = {65.0f, 67.25f};
const float targetTemperatureDrifts[] = {0.5f, 0.5f};
const float targetHumidityDrifts[] = {2.5f, 2.5f};

struct NVMData_t {
	uint8_t lightBeginHour;
	uint8_t lightBeginMinute;
	uint32_t lightPeriodSeconds;
};

struct Peripheral_t {
	const char* name;
	uint8_t pin;
  
	bool currentState;
  
	bool autoState;
  
	bool forceMode;
	bool forceState;
};

struct Time_t {
	unsigned long epochBase;
	unsigned long millisBase;
	bool timeValid;
};

struct EnvironmentSensor_t {
  Adafruit_SHT31 sht31;
  MQ135 mq135;

  float temp;
  float humidity;
  float ppm;
};


EnvironmentSensor_t internalSensor = { Adafruit_SHT31(), MQ135(MQ135_INT_PIN), 24.f, 60.f, 0.f };
EnvironmentSensor_t externalSensor = { Adafruit_SHT31(), MQ135(MQ135_EXT_PIN), 24.f, 60.f, 0.f };

struct NVMData_t nvmData;
Time_t currentTime;

// Easy Accessors
bool exhaustOverride = false;
Peripheral_t light = {"Light", LIGHT_PIN, false, false, false};
Peripheral_t humidifier = {"Humidifier", HUMIDIFIER_PIN, false, false, false};
Peripheral_t exhaust = {"Exhaust", EXHAUST_PIN, false, false, false};
Peripheral_t heater = {"Heater", HEATER_PIN, false, false, false};
Peripheral_t* peripherals[] = {&light, &humidifier, &exhaust, &heater};

void loadNVMData()
{
	EEPROM.get(EEPROM_ADDR, nvmData);
	if (nvmData.lightPeriodSeconds == 0 || nvmData.lightPeriodSeconds > 86400)
	{
		nvmData = {06, 30, 64800};
		saveNVMData();
	}
}

void saveNVMData()
{
	EEPROM.put(EEPROM_ADDR, nvmData);
}

void syncTime(unsigned long epoch)
{
	currentTime.epochBase = epoch;
	currentTime.millisBase = millis();
	currentTime.timeValid = true;
}

unsigned long nowEpoch()
{
	if (!currentTime.timeValid)
		return millis();
	return currentTime.epochBase + (millis() - currentTime.millisBase) / 1000 + 3UL * 3600UL;
}

unsigned long lastLightStartEpoch()
{
    const unsigned long DAY = 86400UL;
	const unsigned long now = nowEpoch();
    unsigned long todayStart = (now - (now % DAY)) + (unsigned long)nvmData.lightBeginHour * 3600UL + (unsigned long)nvmData.lightBeginMinute * 60UL;
    if (todayStart > now)
        todayStart -= DAY;
    return todayStart;
}

void setup()
{
	loadNVMData();
	
	Serial.begin(9600);
	Wire.begin();
	internalSensor.sht31.begin(SHT31_INT_ADDR);
	//externalSensor.sht31.begin(SHT31_EXT_ADDR);

	for (auto r : peripherals)
    	pinMode(r->pin, INPUT);
}

void readSensors()
{
	static unsigned long nextRead = 0U;
	if(nextRead < millis())
	{
		nextRead = millis() + READ_MS;

		internalSensor.temp = internalSensor.sht31.readTemperature();
		internalSensor.humidity = internalSensor.sht31.readHumidity();
		//internalSensor.ppm = internalSensor.mq135.getPPM();

		// externalSensor.temp = externalSensor.sht31.readTemperature();
		// externalSensor.humidity = externalSensor.sht31.readHumidity();
		// externalSensor.ppm = externalSensor.mq135.getPPM();
	}
	
}

void serialize()
{
	static unsigned long nextSerialize = 0U;
	if(nextSerialize < millis())
	{
		nextSerialize = millis() + SERIALIZE_MS;
		
		StaticJsonDocument<256> tx;
		tx["ts"] = nowEpoch();
		tx["temperature"] = internalSensor.temp;
		tx["humidity"] = internalSensor.humidity;
		tx["ppm"] = internalSensor.ppm;
		// tx["temperature_ext"] = externalSensor.temp;
		// tx["humidity_ext"] = externalSensor.humidity;
		// tx["ppm_ext"] = externalSensor.ppm;
		tx["lightBeginHour"] = nvmData.lightBeginHour;
		tx["lightBeginMinute"] = nvmData.lightBeginMinute;
		tx["lightPeriodSeconds"] = nvmData.lightPeriodSeconds;
		
		JsonObject rs = tx.createNestedObject("relays");
		for (auto r : peripherals)
    		rs[r->name] = r->currentState; 
		
		serializeJson(tx, Serial);
		Serial.println();
	}
}

void receive()
{
	static char rxLine[RX_BUF_SIZE];
	static uint8_t rxPos = 0;
	static unsigned long lastRx = 0UL;

	while (Serial.available())
	{
		char c = Serial.read();
		lastRx = millis();

		if (c == '\n')
		{
			rxLine[rxPos] = '\0';
			rxPos = 0;

			StaticJsonDocument<256> rx;
			if (!deserializeJson(rx, rxLine))
			{
				if (rx.containsKey("epoch"))
					syncTime(rx["epoch"]);

				if (rx.containsKey("lightBeginHour"))
				{
					nvmData.lightBeginHour = rx["lightBeginHour"];
					nvmData.lightBeginMinute = rx["lightBeginMinute"];
					nvmData.lightPeriodSeconds = rx["lightPeriodSeconds"];
					saveNVMData();
				}

				for (uint8_t i = 0; i < 4; i++)
				{
					if (rx.containsKey(peripherals[i]->name))
					{
						bool newMode = !rx[peripherals[i]->name].isNull();
						if(peripherals[i]->forceMode != newMode)
						{
							peripherals[i]->forceMode = newMode;
						}

						if (peripherals[i]->forceMode)
						{
							if (peripherals[i]->forceState != rx[peripherals[i]->name])
							{
								peripherals[i]->forceState = rx[peripherals[i]->name];
							}
						}
					}
				}
			}
		}
		else if (rxPos < RX_BUF_SIZE - 1)
		{
			rxLine[rxPos++] = c;
		}
		else
		{
			rxPos = 0;
		}
	}

	if (rxPos && (millis() - lastRx) > RX_TIMEOUT_MS)
		rxPos = 0;
}

void autoPilot_Climate()
{
	static unsigned long heaterStopAt = 0UL;
	static unsigned long heaterCooldownUntil = 0UL;
	static bool heaterState = false;

	const unsigned long now = nowEpoch();
	const bool isDay = light.autoState;

	const float tTemp      = isDay ? targetTemperatures[0]       : targetTemperatures[1];
	const float tTempDrift = isDay ? targetTemperatureDrifts[0]  : targetTemperatureDrifts[1];
	const float tHum       = isDay ? targetHumidities[0]         : targetHumidities[1];
	const float tHumDrift  = isDay ? targetHumidityDrifts[0]     : targetHumidityDrifts[1];

	const float temp = internalSensor.temp;
	const float hum  = internalSensor.humidity;

	// Heater
	if (heaterState)
	{
		if (temp > tTemp - tTempDrift || now >= heaterStopAt)
		{
			heaterState = false;
			heaterCooldownUntil = now + HEATER_COOLDOWN_SEC;
		}
	}
	else
	{
		if (temp < tTemp - tTempDrift && now >= heaterCooldownUntil)
		{
			heaterState = true;
			heaterStopAt = now + HEATER_MAX_SEC;
		}
	}
	heater.autoState = heaterState;

	// Humidifier
	humidifier.autoState = humidifier.autoState ? hum < tHum : hum < tHum - tHumDrift;

	// Exhaust
	exhaust.autoState = ((temp > tTemp + tTempDrift) || (hum > tHum + tHumDrift));

	if(exhaustOverride && !exhaust.forceMode)
		exhaust.autoState = true;
}

void autopilot()
{
	
	static unsigned long nextAuto = 0U;
	if(nextAuto < millis())
	{
		nextAuto = millis() + AUTO_MS;
		
		if(currentTime.timeValid)
		{
			const unsigned long now = nowEpoch();
			const unsigned long hh = now / 3600UL;
			const unsigned long mm = (now % 3600UL) / 60UL;
			const unsigned long ss = now % 60UL;

			// Light
			{
				const unsigned long lightStart = lastLightStartEpoch();
				unsigned long lightEnd = lightStart + nvmData.lightPeriodSeconds;
				const unsigned long nextLightStart = lightStart + 24UL * 3600UL;

				bool isLightOn = (now >= lightStart) && (now < lightEnd);
				
				light.autoState = isLightOn;
			}
		}
		else
		{
			light.autoState = true;
		}
	}

	// Climate
	static unsigned long nextClimate = 0UL;
	if (nextClimate < millis())
	{
		nextClimate = millis() + AUTO_CLIMATE_MS;

		if(currentTime.timeValid)
		{
			autoPilot_Climate();
		}
	}
}

void relays()
{
	static unsigned long nextPeripherals = 0U;
	if(nextPeripherals < millis())
	{
		nextPeripherals = millis() + PERIPHERALS_MS;
		
		const uint8_t peripheral_count = sizeof(peripherals) / sizeof(peripherals[0u]);
		for(uint8_t i = 0u ; i < peripheral_count ; i++)
		{
			peripherals[i]->currentState = !peripherals[i]->forceMode ? peripherals[i]->autoState : peripherals[i]->forceState;
			if(peripherals[i]->currentState)
			{
				pinMode(peripherals[i]->pin, OUTPUT);
				digitalWrite(peripherals[i]->pin, LOW);
			}
			else
			{
				pinMode(peripherals[i]->pin, INPUT);
			}
		}
	}
}

void loop()
{
	readSensors();
	
	receive();
	
	serialize();
  
	autopilot();
  
	relays();
}
