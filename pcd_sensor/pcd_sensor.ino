#include <NewPing.h>
#include <SPI.h>
#include <mcp2515_can.h>
// serial monitor 
#define BAUD_RATE 115200
// sonar 
#define SONAR_NUM     3 // Number of sensors.
#define MAX_DISTANCE 400 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 33 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
// can hat
#define CAN_SAMPLE_RATE CAN_500KBPS   // CAN bus speed
#define CAN_HAT_CS_PIN 9
// sensor pins - Note: Sensor Slligment out of driver's perspective & counting clockwise. 
// FRONT LEFT SENSOR: CAN FRAME BYTE 0&1
// FRONT MIDDLE SENSOR: CAN FRAME BYTE 2&3
// FRONT RIGHT SENSOR: CAN FRAME BYTE 4&5
#define SENSOR_FRONT_LEFT_TRIG_PIN 5           
#define SENSOR_FRONT_LEFT_ECHO_PIN 5
#define SENSOR_FRONT_MIDDLE_TRIG_PIN 6
#define SENSOR_FRONT_MIDDLE_ECHO_PIN 6
#define SENSOR_FRONT_RIGHT_TRIG_PIN 7
#define SENSOR_FRONT_RIGHT_ECHO_PIN 7
// sonar
unsigned long pingTimer[SONAR_NUM];           // Holds the times when the next ping should happen for each sensor.
unsigned short cm[SONAR_NUM];                 // Where the ping distances are stored.
uint8_t currentSensor = 0;                    // Keeps track of which sensor is active.
NewPing sonar[SONAR_NUM] = {                  // Sensor object array with each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(SENSOR_FRONT_LEFT_TRIG_PIN, SENSOR_FRONT_LEFT_ECHO_PIN, MAX_DISTANCE), 
  NewPing(SENSOR_FRONT_MIDDLE_TRIG_PIN, SENSOR_FRONT_MIDDLE_ECHO_PIN, MAX_DISTANCE), 
  NewPing(SENSOR_FRONT_RIGHT_TRIG_PIN, SENSOR_FRONT_RIGHT_ECHO_PIN, MAX_DISTANCE), 
};
// can 
const unsigned long can_id = 0x200;             // CAN Device address
const int  ext_frame = 0;                       // extended can frame
const int len_frame = SONAR_NUM*sizeof(unsigned short);  // CAN message data length (number of bytes in frame)
uint8_t data_buf[len_frame];                    // CAN message payload with buffer can_dlc = SONAR_NUM
mcp2515_can CAN(CAN_HAT_CS_PIN);                // CAN Bus object

void setup() {
  Serial.begin(BAUD_RATE);
  while(!Serial){};
  while (CAN_OK != CAN.begin(CAN_SAMPLE_RATE)) {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
    }
  Serial.println("CAN BUS Shield init ok!");
  // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  pingTimer[0] = millis() + 75;         
  // Set the starting time for each sensor.  
  for (uint8_t i = 1; i < SONAR_NUM; i++) {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }
}

void loop() {
  // Loop through all the sensors.
  for (uint8_t i = 0; i < SONAR_NUM; i++) { 
    // Is it this sensor's time to ping?
    if (millis() >= pingTimer[i]) {         
      // Set next time this sensor will be pinged.
      pingTimer[i] += PING_INTERVAL * SONAR_NUM;  
      // Sensor ping cycle complete, do something with the results.
      if (i == 0 && currentSensor == SONAR_NUM - 1){  
        oneSensorCycle();
      } 
      // Make sure previous timer is canceled before starting a new ping (insurance).
      sonar[currentSensor].timer_stop();  
      // Sensor being accessed.        
      currentSensor = i;              
      // Make distance zero in case there's no ping echo for this sensor.            
      cm[currentSensor] = 0;                      
      // Do the ping (processing continues, interrupt will call echoCheck to look for echo).
      sonar[currentSensor].ping_timer(echoCheck); 
    }
  }
}

/* If ping received, set the sensor distance to array. */
void echoCheck() { 
  if (sonar[currentSensor].check_timer())
    cm[currentSensor] = sonar[currentSensor].ping_result / US_ROUNDTRIP_CM;   
}

/* Sensor ping cycle complete, do something with the results. */
void oneSensorCycle() { 
  // print sensor data to serial monitor in dec 
  // for (uint8_t i = 0; i < SONAR_NUM; i++) {
  //   Serial.print(i); 
  //   Serial.print(":");
  //   Serial.println(cm[i]);
  // }      
  // load data_buf with sensor data
  data_buf[0] = highByte(cm[0]);
  data_buf[1] = lowByte(cm[0]);
  data_buf[2] = highByte(cm[1]);
  data_buf[3] = lowByte(cm[1]);
  data_buf[4] = highByte(cm[2]);
  data_buf[5] = lowByte(cm[2]); 
  //print to serial monitor in hex
  Serial.println("-----------------------------");
  Serial.print("Set data from ID: 0x");
  Serial.println(can_id, HEX);
  for (int i = 0; i < len_frame; i++) { 
    Serial.print(data_buf[i], HEX);
    Serial.print("\t");
  }
  Serial.println();
  //send CAN message
  CAN.sendMsgBuf(can_id, ext_frame, len_frame, data_buf);                   
}