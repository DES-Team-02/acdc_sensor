#include <NewPing.h>
#include <SPI.h>
#include <mcp2515_can.h>

#define BAUD_RATE 115200
#define SONAR_NUM 3 // Number of sensors.
#define MAX_DISTANCE 400 // Maximum distance (in cm) to ping.
#define CAN_SAMPLE_RATE CAN_500KBPS // CAN bus speed
#define CAN_HAT_CS_PIN 9

// Sensor pins
#define SENSOR_FRONT_LEFT_TRIG_PIN 5
#define SENSOR_FRONT_LEFT_ECHO_PIN 6
#define SENSOR_FRONT_MIDDLE_TRIG_PIN 7
#define SENSOR_FRONT_MIDDLE_ECHO_PIN 8
#define SENSOR_FRONT_RIGHT_TRIG_PIN 9
#define SENSOR_FRONT_RIGHT_ECHO_PIN 10

unsigned short cm[SONAR_NUM]; // Where the ping distances are stored.

// Sensor object array
NewPing sonar[SONAR_NUM] = {
  NewPing(SENSOR_FRONT_LEFT_TRIG_PIN, SENSOR_FRONT_LEFT_ECHO_PIN, MAX_DISTANCE),
  NewPing(SENSOR_FRONT_MIDDLE_TRIG_PIN, SENSOR_FRONT_MIDDLE_ECHO_PIN, MAX_DISTANCE),
  NewPing(SENSOR_FRONT_RIGHT_TRIG_PIN, SENSOR_FRONT_RIGHT_ECHO_PIN, MAX_DISTANCE)
};

const unsigned long can_id = 0x200; // CAN Device address
mcp2515_can CAN(CAN_HAT_CS_PIN); // CAN Bus object

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) {}; // Wait for serial port to connect.
  if (CAN_OK != CAN.begin(CAN_SAMPLE_RATE)) {
    Serial.println("CAN BUS Shield init fail");
    delay(100);
  } else {
    Serial.println("CAN BUS Shield init ok!");
  }
}

void loop() {
  triggerSensors();
  delay(100); // Delay to allow for sensor echo returns
  readSensors();
  sendCANMessage();
  delay(50); // Delay between readings
}

void triggerSensors() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    sonar[i].ping(); // Trigger each sensor to ping
  }
}

void readSensors() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    cm[i] = sonar[i].ping_cm(); // Read each sensor
  }
}

void sendCANMessage() {
  uint8_t len_frame = SONAR_NUM * sizeof(unsigned short);
  uint8_t data_buf[len_frame];

  // Load data_buf with sensor data
  for (int i = 0, j = 0; i < SONAR_NUM; i++, j += 2) {
    data_buf[j] = highByte(cm[i]);
    data_buf[j + 1] = lowByte(cm[i]);
  }

  // Send CAN message
  CAN.sendMsgBuf(can_id, 0, len_frame, data_buf);
}
