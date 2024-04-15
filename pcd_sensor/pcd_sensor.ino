#include <NewPing.h>
#include <SPI.h>
#include <mcp2515_can.h>

#define BAUD_RATE 115200 // Serial baud rate
#define SONAR_NUM 3 // Number of ultrasonic sensors.
#define MEASURE_ITERATION 3 // Number of measuring iterations for each sensor.
#define MAX_DISTANCE 200 // Maximum distance (in cm) that can be measured.
#define CAN_SAMPLE_RATE CAN_500KBPS // CAN bus speed
#define CAN_HAT_CS_PIN 9 // Pin number for the CAN controller chip select

// Pins for the ultrasonic sensors
#define SENSOR_FRONT_LEFT_TRIG_PIN 3
#define SENSOR_FRONT_LEFT_ECHO_PIN 4
#define SENSOR_FRONT_MIDDLE_TRIG_PIN 5
#define SENSOR_FRONT_MIDDLE_ECHO_PIN 6
#define SENSOR_FRONT_RIGHT_TRIG_PIN 7
#define SENSOR_FRONT_RIGHT_ECHO_PIN 8

unsigned short cm[SONAR_NUM]; // Array to store distances measured by sensors.
uint8_t data_buf[SONAR_NUM * 2]; // Data buffer for CAN message
const unsigned long can_id = 0x200; // CAN device address
mcp2515_can CAN(CAN_HAT_CS_PIN); // CAN bus object
float temp = 25; // Temperature of air
float factor = sqrt(1 + temp / 273.15) / 60.368; // Factor to convert milliseconds to distance

// Ultrasonic sensor objects
NewPing sonar[SONAR_NUM] = {
  NewPing(SENSOR_FRONT_LEFT_TRIG_PIN, SENSOR_FRONT_LEFT_ECHO_PIN, MAX_DISTANCE),
  NewPing(SENSOR_FRONT_MIDDLE_TRIG_PIN, SENSOR_FRONT_MIDDLE_ECHO_PIN, MAX_DISTANCE),
  NewPing(SENSOR_FRONT_RIGHT_TRIG_PIN, SENSOR_FRONT_RIGHT_ECHO_PIN, MAX_DISTANCE)
};

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) {}; // Wait for the serial port to connect. Necessary for native USB port only.
  if (CAN_OK != CAN.begin(CAN_SAMPLE_RATE)) {
    Serial.println("CAN BUS Shield init fail"); // Initialization failed
    delay(100);
  } else {
    Serial.println("CAN BUS Shield init ok!"); // Initialization successful
  }
}

void loop() {
  oneSensorCycle();
  delay(100);
}

void oneSensorCycle() { 
  readSensors(); // Read measurements from each sensor

  // Load data_buf with sensor data
  for (int i = 0, j = 0; i < SONAR_NUM; i++, j += 2) {
    data_buf[j] = highByte(cm[i]);
    data_buf[j + 1] = lowByte(cm[i]);
  }

  // Print sensor data to the serial monitor in hexadecimal format
  Serial.println("-----------------------------");
  Serial.print("Set data from ID: 0x");
  Serial.println(can_id, HEX); // Print CAN ID
  for (int i = 0; i < sizeof(data_buf); i++) { 
    Serial.print(data_buf[i], HEX); // Print data bytes
    Serial.print("\t");
  }
  Serial.println();

  // Send CAN message
  CAN.sendMsgBuf(can_id, 0, sizeof(data_buf), data_buf);                   
}

void readSensors() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    cm[i] = (short)(sonar[i].ping_median(MEASURE_ITERATION, MAX_DISTANCE) * factor); // Read distance from each sensor and store it in the cm array
  }
}
