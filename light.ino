

#define ADDRESS_SENSOR 0b0100011
#define MODE_CONT_HRES 0b00010000
#define READ_ERROR 0xFFFF

void setupSensorComms(){
    Wire.begin();
    
    Wire.beginTransmission(ADDRESS_SENSOR);
    Wire.write(MODE_CONT_HRES);
    Wire.endTransmission();
    delay(10);
}

void setup() {
    
    Serial.begin(9600);
    Serial.println("Powered on.");
    
    setupSensorComms();
}

uint16_t readSensor(){
    
    Wire.beginTransmission(ADDRESS_SENSOR);
    
    int readStatus = Wire.requestFrom(ADDRESS_SENSOR, 2);
    if(readStatus != 2) return READ_ERROR;
    
    unsigned char highByte = Wire.read();
    unsigned char lowByte = Wire.read();
    
    uint16_t reading = 0;
    reading |= highByte;
    reading <<= 8;
    reading |= lowByte;
    
    Wire.endTransmission();
    
    return reading;
}

double readingToLux(uint16_t reading){
    return double(reading) / 1.2; 
}

#define LIGHT_THRESHOLD 10

bool lightPulseActive = false;
int lightPulseStart = 0;
uint16_t lightPulseMin = 0;
uint16_t lightPulseMax = 0;

void handleLightPulseStart(uint16_t reading){
    lightPulseActive = true;
    lightPulseStart = millis() / 1000;
    lightPulseMax = lightPulseMin = reading;
    
    Particle.publish("bright");
}

void handleLightPulseEnd(){
    lightPulseActive = false;
    int lightPulseEnd = millis() / 1000;
    
    unsigned int lightPulseLength = lightPulseEnd - lightPulseStart;
    
    String lengthString;
    
    if(lightPulseLength == 0)
        lengthString = "under a second";
    
    if(lightPulseLength > 0 && lightPulseLength < 60) 
        lengthString = String::format("%u seconds", lightPulseLength);
        
    if(lightPulseLength >= 60 && lightPulseLength < 60*60)
        lengthString = String::format("%u minutes", lightPulseLength / 60);
        
    if(lightPulseLength >= 60*60)
        lengthString = String::format("%.1f hours", double(lightPulseLength) / (60.0*60.0));
    
    Particle.publish("dark", String::format(
        "{\"length\": \"%s\", \"lux\": %.2f}", 
        lengthString.c_str(), readingToLux(lightPulseMax)
    ));
}

void loop() {
    
    uint16_t reading = readSensor();
    
    if(lightPulseActive){
        if(reading <= LIGHT_THRESHOLD){
            handleLightPulseEnd();
            
        } else {
            if(reading < lightPulseMin) 
                lightPulseMin = reading;
                
            if(reading > lightPulseMax) 
                lightPulseMax = reading;
        }
    } else {
        if(reading > LIGHT_THRESHOLD){
            handleLightPulseStart(reading);
        }
    }
    
}

