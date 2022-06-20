#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <Arduino_LSM6DSOX.h>
#include <PDM.h>

char ssid[] = "ncs" ; 
char pass[] = "12312300" ;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
                       
const char broker[] = "192.168.0.74";
int        port     = 1883;
const char topic_temp[]  = "temperature";
const char topic_accel[]  = "accelerometer";
const char topic_state[]  = "state";
const char topic_res[]  = "resister";
const char topic_gyro[]  = "gyro";
const char topic_pdm[]  = "pdm";

static const char channels = 1;
static const int frequency = 16000;
short sampleBuffer[512];
volatile int samplesRead;

const long interval = 500;
unsigned long previousMillis = 0;

int sensorValue = 0;        
int outputValue = 0;

int speed_count = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(channels, frequency)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }
  
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
  }

  Serial.println("You're connected to the network");
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  
  if (!IMU.begin())
  {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
}

void loop() {
  mqttClient.poll();

  unsigned long currentMillis = millis();

  if (samplesRead) {
    for (int i = 0; i < samplesRead; i++) {
      mqttClient.beginMessage(topic_pdm);
      mqttClient.println(sampleBuffer[i]);
      mqttClient.endMessage();
    }
    samplesRead = 0;
  }
  
  if (currentMillis - previousMillis >= interval) {
    sensorValue = analogRead(A3);
    outputValue = map(sensorValue, 0, 1023, 0, 255);
    
    mqttClient.beginMessage(topic_res);
    mqttClient.print(outputValue);
    mqttClient.endMessage();

    if (IMU.temperatureAvailable() || IMU.accelerationAvailable())
    {
      int temperature_deg = 0;
      float x, y, z, roll, pitch, yaw;
      IMU.readTemperature(temperature_deg);
      IMU.readAcceleration(x, y, z);
      IMU.readGyroscope(roll, pitch, yaw);

      mqttClient.beginMessage(topic_temp);
      mqttClient.print(temperature_deg);
      mqttClient.println(" °C");
      mqttClient.endMessage();

      mqttClient.beginMessage(topic_accel);
      mqttClient.print(x);
      mqttClient.print(" ");
      mqttClient.print(y);
      mqttClient.print(" ");
      mqttClient.print(z);
      mqttClient.endMessage();

      mqttClient.beginMessage(topic_gyro);
      mqttClient.print(roll);
      mqttClient.print(" ");
      mqttClient.print(pitch);
      mqttClient.print(" ");
      mqttClient.print(yaw);
      mqttClient.endMessage();

      mqttClient.beginMessage(topic_state);
      if((x>-0.02 &&x<0.02) || (y>-0.03&&y<0.03) || (z>0.98 && z<1.02)){
        if(speed_count == 0) {
          mqttClient.print("정지");
        }
        else {
          speed_count = 0;
          mqttClient.print("정지");
        }
      }
      else if((x>-1.0 && x<1.0) || (y>-1.0 && y<1.0) || (z>0.52 &&  z<-1.48)){
        if(speed_count == 1) {
          mqttClient.print("걷기");
        }
        else {
          speed_count = 1;
          mqttClient.print("걷기");
        }
      }
      else{
        if(speed_count == 2) {
          mqttClient.print("뛰기");
        }
        else {
          speed_count = 2;
          mqttClient.print("뛰기");
        }
      }
      mqttClient.endMessage();
      previousMillis = currentMillis;
  }
 }
}
void onPDMdata() {
  // Query the number of available bytes
  int bytesAvailable = PDM.available();

  // Read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
