#include <Wire.h>

/** 
  MPU6050의 기본 I2C Address
*/
const uint8_t MPU6050_I2C_ADDR  = 0x68; 

/**
  MPU6050의 레지스터 Address
*/
const uint8_t ACCEL_XOUT_H = 0x3B;
const uint8_t ACCEL_XOUT_L = 0x3C;
const uint8_t ACCEL_YOUT_H = 0x3D;
const uint8_t ACCEL_YOUT_L = 0x3E;
const uint8_t ACCEL_ZOUT_H = 0x3F;
const uint8_t ACCEL_ZOUT_L = 0x40;
const uint8_t TEMP_OUT_H = 0x41;
const uint8_t TEMP_OUT_L = 0x42;
const uint8_t GYRO_XOUT_H = 0x43;
const uint8_t GYRO_XOUT_L = 0x44;
const uint8_t GYRO_YOUT_H = 0x45;
const uint8_t GYRO_YOUT_L = 0x46;
const uint8_t GYRO_ZOUT_H = 0x47;
const uint8_t GYRO_ZOUT_L = 0x48;
const uint8_t PWR_MGMT_1 = 0x6B;

/** 
  MPU6050의 센서 데이터 구조체 선언
  */
typedef struct _MPU6050Data{
  uint16_t AcX; // X축 가속도(2바이트)
  uint16_t AcY; // Y축 가속도(2바이트)
  uint16_t AcZ; // Z축 가속도(2바이트)
  uint16_t Temp; // 온도(2바이트)
  uint16_t GyX; // X축 자이로(2바이트)
  uint16_t GyY; // Y축 자이로(2바이트)
  uint16_t GyZ; // Z축 자이로(2바이트)
} MPU6050Data;

/**
  MPU6050의 동작을 활성화 시키는 함수 (wakeup)
*/
void wakeup(){
  Wire.begin();
  Wire.beginTransmission(MPU6050_I2C_ADDR); // I2C통신으로 데이터 전송 시작 (MPU6050)
  Wire.write(PWR_MGMT_1); // 레지스터 위치를 PWR_MGMT_1로 지정
  Wire.write(0x00); // 0x00을 현재 레지스터(즉 PWR_MGMT_1 레지스터)에 기록
  Wire.endTransmission(true); // I2C통신으로 데이터 전송 종료 (연결 해제)
}

/**
  MPU-6050 센서 측정값 얻기
*/
MPU6050Data measure() {
  MPU6050Data sensingValue;

  Wire.beginTransmission(MPU6050_I2C_ADDR); // I2C통신으로 데이터 전송 시작 (MPU6050)
  Wire.write(ACCEL_XOUT_H); // 레지스터 위치를 ACCEL_XOUT_H로 지정
  Wire.endTransmission(false); // I2C통신으로 데이터 전송 종료 (연결 유지)
  // I2C통신으로 MPU6050의 현재 레지스터 위치로부터 14바이트의 데이터 수신 
  // (수신 완료 후 연결 종료)
  Wire.requestFrom(MPU6050_I2C_ADDR,(size_t)14,true); 
  sensingValue.AcX = Wire.read() << 8 | Wire.read();
  sensingValue.AcY = Wire.read() << 8 | Wire.read();
  sensingValue.AcZ = Wire.read() << 8 | Wire.read();
  sensingValue.Temp = Wire.read() << 8 | Wire.read();
  sensingValue.GyX = Wire.read() << 8 | Wire.read();
  sensingValue.GyY = Wire.read() << 8 | Wire.read();
  sensingValue.GyZ = Wire.read() << 8 | Wire.read();

  return sensingValue;
}

/**
  ESP32 보드의 setup 함수 (보드가 리셋될 때 한번만 실행되는 부분)
*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // 시리얼 통신 시작 (baudrate는 115200으로 설정)

  wakeup();
}

/**
  ESP32 보드에서 무한반복해서 호출되는 loop 함수
*/
void loop() {
  // put your main code here, to run repeatedly:
  MPU6050Data sensingValue = measure();

  Serial.printf("ACCEL_X : %d\n",sensingValue.AcX);
  Serial.printf("ACCEL_Y : %d\n",sensingValue.AcY);
  Serial.printf("ACCEL_Z : %d\n",sensingValue.AcZ);
  Serial.printf("TEMP : %d\n",sensingValue.Temp);
  Serial.printf("GYRO_X : %d\n",sensingValue.GyX);
  Serial.printf("GYRO_Y : %d\n",sensingValue.GyY);
  Serial.printf("GYRO_Z : %d\n",sensingValue.GyZ);
  Serial.print("\n");

  delay(1000); // 1초 대기
}
