#define ADDRESS_SENSOR 0b00100011
#define MODE_CONT_HRES 0b00010000
#define READ_ERROR 0xFFFF

void setupSensorComms(){
    Wire.begin();
    
    Wire.beginTransmission(ADDRESS_SENSOR);
    Wire.write(MODE_CONT_HRES);
    Wire.endTransmission();
    
    delay(10);
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

//////////////////////////////////////////
//////////////////////////////////////////

#define LIGHT_THRESHOLD 10

bool lightPeriodActive = false;
int lightPeriodStart = 0;
uint16_t lightPeriodMin = 0;
uint16_t lightPeriodMax = 0;

void handleLightPeriodStart(uint16_t reading){
    lightPeriodActive = true;
    lightPeriodStart = millis() / 1000;
    lightPeriodMax = lightPeriodMin = reading;
    
    Particle.publish("bright");
}

void handleLightPeriodEnd(){
    lightPeriodActive = false;
    int lightPeriodEnd = millis() / 1000;
    
    unsigned int lightPeriodLength = lightPeriodEnd - lightPeriodStart;
    
    String lengthString;
    
    if(lightPeriodLength == 0)
        lengthString = "under a second";
    
    if(lightPeriodLength > 0 && lightPeriodLength < 60) 
        lengthString = String::format("%u seconds", lightPeriodLength);
        
    if(lightPeriodLength >= 60 && lightPeriodLength < 60*60)
        lengthString = String::format("%u minutes", lightPeriodLength / 60);
        
    if(lightPeriodLength >= 60*60)
        lengthString = String::format("%.1f hours", double(lightPeriodLength) / (60.0*60.0));
    
    Particle.publish("dark", String::format(
        "{\"length\": \"%s\", \"lux\": %.2f}", 
        lengthString.c_str(), readingToLux(lightPeriodMax)
    ));
}

//////////////////////////////////////////
//////////////////////////////////////////

void setup() {
    
    Serial.begin(9600);
    Serial.println("Powered on.");
    
    setupSensorComms();

}

#define SCHEDULE_PERIOD 1800
unsigned int lastScheduled = millis();

void loop() {
    
    uint16_t reading = readSensor();
    Serial.printlnf("%u", reading);
    
    if(millis() - lastScheduled > SCHEDULE_PERIOD){
        
        lastScheduled = millis();
        
        if(lightPeriodActive){
            
            if(reading <= LIGHT_THRESHOLD){
                handleLightPeriodEnd();
            } else {
                if(reading < lightPeriodMin) lightPeriodMin = reading;
                if(reading > lightPeriodMax) lightPeriodMax = reading;
            }
            
        } else {
            
            if(reading > LIGHT_THRESHOLD) {
                handleLightPeriodStart(reading);
            }   
            
        }
        
    }
    
}

