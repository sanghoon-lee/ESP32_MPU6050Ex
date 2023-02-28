#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid  = "sanghoon's a52s"  // 연결할 와이파이 공유기(AP)의 SSID명칭
const char* password = "w2f2password";  // 연결한 와이파이 공유기(AP)의 비밀번호
const char* server_ip = "192.168.20.87";  // MQTT Broker의 접속주소
const int server_port = 1883;           // MQTT Broker 접속 포트번호

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

/**
  MPU6050의 가속도 센서에서 사용할 Full Scale Range
*/
const uint8_t AFS_SEL = 0;

/** 
  MPU6050의 기본 I2C Address
*/
const uint8_t MPU6050_I2C_ADDR  = 0x68; 

/**
  MPU6050의 레지스터 Address
*/
const uint8_t ACCEL_CONFIG = 0x1C;
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
  int16_t AcX; // X축 가속도(2바이트)
  int16_t AcY; // Y축 가속도(2바이트)
  int16_t AcZ; // Z축 가속도(2바이트)
  int16_t Temp; // 온도(2바이트)
  int16_t GyX; // X축 자이로(2바이트)
  int16_t GyY; // Y축 자이로(2바이트)
  int16_t GyZ; // Z축 자이로(2바이트)
} MPU6050Data;

/**
 센서의 측정 값을 저장할 전역변수
*/
MPU6050Data sensingValue;

/**
 센서의 Callibration 값을 저장할 전역변수
*/
MPU6050Data callibrationValue;

/**
 센서의 G_UNIT을 저장할 전역변수
*/
int G_UNIT;

/**
 WIFI 공유기에 접속
*/
void connect(){
  WiFi.begin(ssid, password);

  while(WiFi.status()!=WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected"); // 연결이 완료되면 Connected 문자열을 시리얼로 출력
}

/**
  MQTT Broker에 연결하는 기능을 구현한 함수
*/
void mqtt_connect(){
  mqtt_client.setServer(server_ip,server_port);

  while(!mqtt_client.connected()){
    String client_id = String(random(0xffff),HEX);
    if(mqtt_client.connect(client_id.c_str())){
      Serial.println("MQTT Broker is connected");
    } else{
      Serial.print("failed, rc=");
      Serial.println(mqtt_client.state());
      delay(10000); // 10초 후 재 접속
    }
  }
}

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
  MPU6050의 Full Scale Range를 설정
*/
void setFullScaleRange(){
  Wire.beginTransmission(MPU6050_I2C_ADDR); // I2C통신으로 데이터 전송 시작 (MPU6050)
  Wire.write(ACCEL_CONFIG); // 레지스터의 위치를 ACCEL_CONFIG로 지정
  if(AFS_SEL==0){
    Wire.write(0x00); // Full Scale Range = +/- 2g
  } else if(AFS_SEL==1){
    Wire.write(0x08); // Full Scale Range = +/- 4g
  } else if(AFS_SEL==2){
    Wire.write(0x10); // Full Scale Range = +/- 8g
  } else if(AFS_SEL==3){
    Wire.write(0x18); // Full Scale Range = +/- 16g
  }
  Wire.endTransmission(true); // I2C통신으로 데이터 전송 종료 (연결 해제)
}

/**
  Full Scale Range에 따른 1G당 가속도 값을 반환
*/
uint16_t getGUnit(){
  int16_t g_unit = 0;

  if(AFS_SEL==0){
    g_unit = 16384; // Full Scale Range = +/- 2g
  } else if(AFS_SEL==1){
    g_unit = 8192; // Full Scale Range = +/- 4g
  } else if(AFS_SEL==2){
    g_unit = 4096; // Full Scale Range = +/- 8g
  } else if(AFS_SEL==3){
    g_unit = 2048; // Full Scale Range = +/- 16g
  }

  return g_unit;
}

/**
  MPU6050의 센서 측정값 얻기
*/
void measure() {
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
}

/**
  MPU6050의 가속도 센서 값 Callibration
*/
void callibration(){
  int counts = 100;

  int16_t AcX = 0;
  int16_t AcY = 0;
  int16_t AcZ = 0;
  
  for(int i=0;i<counts;i++){
    measure();
    AcX += sensingValue.AcX;
    AcY += sensingValue.AcY;
    AcZ += sensingValue.AcZ;
    delay(10);
  }

  callibrationValue.AcX = AcX / counts;
  callibrationValue.AcY = AcY / counts;
  callibrationValue.AcZ = AcZ / counts;
}

/**
  ESP32 보드의 setup 함수 (보드가 리셋될 때 한번만 실행되는 부분)
*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // 시리얼 통신 시작 (baudrate는 115200으로 설정)

  wakeup();
  setFullScaleRange();
  callibration();
  G_UNIT = getGUnit();
}

/**
  ESP32 보드에서 무한반복해서 호출되는 loop 함수
*/
void loop() {
  // put your main code here, to run repeatedly:
  measure();
  
  // callibration값에서 측정된 가속도 값을 차감
  sensingValue.AcX = (sensingValue.AcX-callibrationValue.AcX);
  sensingValue.AcY = (sensingValue.AcY-callibrationValue.AcY);
  sensingValue.AcZ = (sensingValue.AcZ-callibrationValue.AcZ);

  /**
  Serial.printf("ACCEL_X : %d\n",sensingValue.AcX);
  Serial.printf("ACCEL_Y : %d\n",sensingValue.AcY);
  Serial.printf("ACCEL_Z : %d\n",sensingValue.AcZ);
  Serial.printf("TEMP : %f\n",(sensingValue.Temp)/340.00+36.53);
  Serial.printf("GYRO_X : %d\n",sensingValue.GyX);
  Serial.printf("GYRO_Y : %d\n",sensingValue.GyY);
  Serial.printf("GYRO_Z : %d\n",sensingValue.GyZ);
  Serial.printf("\n");
  */

  // WIFI 공유기에 접속이 되지 않은 경우에는 연결시도
  if(WiFi.status()!=WL_CONNECTED){
    connect();
  }

  // MQTT Broker 서버로 연결이 되어 있은 경우에는 연결시도
  if(!mqtt_client.connected()){
    mqtt_connect();
  }

  double gAcX = ((double)sensingValue.AcX/(double)G_UNIT);
  double gAcY = ((double)sensingValue.AcY/(double)G_UNIT);
  double gAcZ = ((double)sensingValue.AcZ/(double)G_UNIT);
  
  // MQTT Broker에 메시지 발행
  char msg[80];
  memset(msg,0x00,sizeof(msg));
  sprintf(msg,"{\"vib\":[%lf,%lf,%lf]}", gAcX, gAcY, gAcZ);
  
  Serial.printf(msg);
  Serial.printf("\n");
  mqtt_client.publish("mpu6050",msg);
  Serial.println("Message is published.");

  delay(20); // 0.01초 대기
}
