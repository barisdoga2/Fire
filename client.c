#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_SHT31.h>
#include <MQ135.h>
#include <ArduinoJson.h>

#define EXHAUST_PIN    		2u
#define HUMIDIFIER_PIN 		3u
#define LIGHT_PIN      		6u
#define HEATER_PIN     		8u
#define MQ135_PIN  	   		A3
#define SHT31_ADDR 	   		0x44u

#define SERIALIZE_MS 	 	1000u
#define PERIPHERALS_MS	 	1000u
#define AUTO_MS		 	 	1000u

#define EEPROM_ADDR 		0u

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


struct NVMData_t nvmData;
Time_t currentTime;
Adafruit_SHT31 sht31;
MQ135 mq135(MQ135_PIN);
Peripheral_t peripherals[] = { {"Light", LIGHT_PIN, false, false, false}, {"Humidifier", HUMIDIFIER_PIN, false, false, false}, {"Exhaust", EXHAUST_PIN, false, false, false}, {"Heater", HEATER_PIN, false, false, false}};

void loadNVMData()
{
	EEPROM.get(EEPROM_ADDR, nvmData);
	if (nvmData.lightPeriodSeconds == 0 || nvmData.lightPeriodSeconds > 86400)
	{
		nvmData = {06,30,64800};
		saveNVMData();
	}
}

void saveNVMData()
{
	EEPROM.put(EEPROM_ADDR, nvmData);
	//EEPROM.commit();
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
	sht31.begin(SHT31_ADDR);

	for (auto &r : peripherals)
		pinMode(r.pin, INPUT);
}

void serialize()
{
	static unsigned long nextSerialize = 0U;
	if(nextSerialize < millis())
	{
		nextSerialize = millis() + SERIALIZE_MS;
		
		StaticJsonDocument<256> tx;
		tx["ts"] = nowEpoch();
		tx["temperature"] = sht31.readTemperature();
		tx["humidity"] = sht31.readHumidity();
		tx["ppm"] = mq135.getPPM();
		tx["lightBeginHour"] = nvmData.lightBeginHour;
		tx["lightBeginMinute"] = nvmData.lightBeginMinute;
		tx["lightPeriodSeconds"] = nvmData.lightPeriodSeconds;
		
		JsonObject rs = tx.createNestedObject("relays");
		for (auto &r : peripherals)
			rs[r.name] = r.currentState;
		
		serializeJson(tx, Serial);
		Serial.println();
	}
}

void receive()
{
	static char rxLine[256];
    static uint8_t rxPos = 0;
	while (Serial.available())
	{
		char c = Serial.read();
		if (c == '\n')
		{
			rxLine[rxPos] = '\0';
			rxPos = 0;
		   
			StaticJsonDocument<256> rx;
			DeserializationError err = deserializeJson(rx, rxLine);
			if (!err)
			{
				if (rx.containsKey("epoch"))
				{
					syncTime(rx["epoch"]);
				}
				if (rx.containsKey("lightBeginHour") && rx.containsKey("lightBeginMinute") && rx.containsKey("lightPeriodSeconds"))
				{
					nvmData.lightBeginHour = rx["lightBeginHour"];
					nvmData.lightBeginMinute = rx["lightBeginMinute"];
					nvmData.lightPeriodSeconds = rx["lightPeriodSeconds"];
					saveNVMData();
				}
				
				for (uint8_t i = 0; i < 4; i++)
				{
					if (rx.containsKey(peripherals[i].name))
					{
						peripherals[i].forceMode = !rx[peripherals[i].name].isNull();
						if(peripherals[i].forceMode)
							peripherals[i].forceState = rx[peripherals[i].name];
					}
				}
			}
		}
		else if (rxPos < sizeof(rxLine) - 1)
		{
			rxLine[rxPos++] = c;
		}
	}
}

void autopilot()
{
	Peripheral_t* light = &peripherals[0u];
	Peripheral_t* humidifier = &peripherals[1u];
	Peripheral_t* exhaust = &peripherals[2u];
	Peripheral_t* heater = &peripherals[3u];

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

			const unsigned long lightStart = lastLightStartEpoch();
			unsigned long lightEnd = lightStart + nvmData.lightPeriodSeconds;
			const unsigned long nextLightStart = lightStart + 24UL * 3600UL;

			bool isLightOn = (now >= lightStart) && (now < lightEnd);
			
			light->autoState = isLightOn;
			
			// if(light->autoState)
			// {
				// const unsigned long remaining = lightEnd - now;
				// const unsigned long h = remaining / 3600UL;
				// const unsigned long m = (remaining % 3600UL) / 60UL;
				// const unsigned long s = remaining % 60UL;
				// Serial.print(F("Lights OFF in "));
				// Serial.print(h); Serial.print(F(" Hours, "));
				// Serial.print(m); Serial.print(F(" Minutes, "));
				// Serial.print(s); Serial.println(F(" Seconds."));
			// }
			// else
			// {
				// const unsigned long remaining = nextLightStart - now;
				// const unsigned long h = remaining / 3600UL;
				// const unsigned long m = (remaining % 3600UL) / 60UL;
				// const unsigned long s = remaining % 60UL;
				// Serial.print(F("Lights ON in "));
				// Serial.print(h); Serial.print(F(" Hours, "));
				// Serial.print(m); Serial.print(F(" Minutes, "));
				// Serial.print(s); Serial.println(F(" Seconds."));
			// }
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
			peripherals[i].currentState = !peripherals[i].forceMode ? peripherals[i].autoState : peripherals[i].forceState;
			if(peripherals[i].currentState)
			{
				pinMode(peripherals[i].pin, OUTPUT);
				digitalWrite(peripherals[i].pin, LOW);
			}
			else
			{
				pinMode(peripherals[i].pin, INPUT);
			}
		}
	}
}

void loop()
{
	receive();

	serialize();
  
	autopilot();
  
	relays();
}
