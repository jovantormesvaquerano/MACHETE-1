// Motor Assisted Commutator for High-fidelity Electro-optical Tethered Experiments (MACHETE)

// Developed to reduce twisting and torque on cables using a brushless motor and Hall effect angle sensor.

// This Arduino sketch controls a commutator system by monitoring the angle of a magnetic field generated by a magnet.
// The system utilizes an AS5600 Hall effect sensor to measure the angle of the field and a brushless motor to 
// adjust the cable's orientation, thereby minimizing twist and tension on the cable over time. The script provides 
// real-time angle tracking and motor response to counteract excessive twist, maintaining consistent cable alignment 
// and reducing torque stress.

// Key functionalities:
//  - Initialization and calibration of AS5600 Hall effect sensor to establish a reference angle.
//  - Continuous monitoring of magnetic field angle to track cable twist.
//  - Dynamic control of motor velocity based on angle deviation to reduce cable torsion.
//  - Real-time adjustment of target velocity with a maximum speed cap to prevent overcorrection.
//  - Control of motor state to maintain twist angle within a predefined limit (cap angle).

#include <SimpleFOC.h>
#include "AS5600.h"
#include "Wire.h"
AS5600 as5600;

BLDCMotor motor = BLDCMotor(7);
BLDCDriver6PWM driver = BLDCDriver6PWM(5, 6, 9, 10, 3, 11);
Commander command = Commander(Serial);

float target_velocity = 6;
long angleref = 0;
long turncount = 0;
long trueangle = 0;
long oldangle = 0;
const long ANGLE_RESOLUTION = 4096;
const long ANGLE_THRESHOLD = 2048;
const long capangle = 340;
const float max_velocity = 24.0;     
const float speedfactor = 5;

void setup() {
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  driver.voltage_power_supply = 5;
  driver.voltage_limit = 5;
  driver.pwm_frequency = 20000;
  driver.init();
  motor.linkDriver(&driver);
  motor.voltage_limit = 3;  // [V]
  motor.controller = MotionControlType::velocity_openloop;
  motor.init();
  as5600.begin(4);                         //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  // default, just be explicit.
  int b = as5600.isConnected();
  angleref = as5600.rawAngle();
}


void loop() {
  updateangle();
  if (abs(trueangle) < capangle) {
    motor.disable();
    command.run();
    while (abs(trueangle) < capangle) { updateangle(); }
    motor.enable();
    command.run();
  }

  if (trueangle > 0) {
    target_velocity = max_velocity * speedfactor * (trueangle - capangle) / 4096.0;}
  else {
    target_velocity = max_velocity * speedfactor * (trueangle + capangle) / 4096.0;}
  if (target_velocity > max_velocity) {
    target_velocity = max_velocity;}
  if (target_velocity < -max_velocity) {
    target_velocity = -max_velocity;}
  motor.move(-target_velocity);
  command.run();
}


void updateangle() {
  long rawangle = as5600.rawAngle();
  trueangle = turncount * ANGLE_RESOLUTION + rawangle - angleref;
  if (oldangle - trueangle > ANGLE_THRESHOLD) {
    turncount++;
    trueangle = trueangle + ANGLE_RESOLUTION;
  } else if (oldangle - trueangle < -ANGLE_THRESHOLD) {
    turncount--;
    trueangle = trueangle - ANGLE_RESOLUTION;}
  oldangle = trueangle;
}
