/*---------------------------------------------
  HACKberryプログラム
  exiii Inc. 小笠原佑樹
  最終更新日：2016/8/22

  Board:Arduino/Genuino Micro
  Programmer:AVRISP mkⅡ
  ---------------------------------------------*/

//serial communication setting
#include <SoftwareSerial.h>
int bluetoothRx = 8;
int bluetoothTx = 9;

SoftwareSerial arm2tooth(bluetoothRx,bluetoothTx);

//そのうちアプリケーションが出来たらString型のデータでやりとりしたい
/*
void stringWrite(String txt)
{
  int i = 0;
  for(i = 0;i < txt.length(); i++)
  {
    arm2tooth.write(txt.charAt(i);
  }
}

String stringRead()
{
  String txt = "";
  while(arm2tooth.available())
  {
    delay(50);
    char inputhcar = arm2tooth.read();
    txt = String(txt + inputchar);
  }
}
*/

//Motor angle setting
#include <Servo.h>

#define thumbExtend   158
#define thumbPinch    60
#define IndexExtend   150   //extend
#define IndexFlex     27    //flex
#define middleExtend  102   //extend
#define middleFlex    35    //flex

const int calibPin0     = A6;   //For calibration
//const int calibPin1     = A5;  //not in use
const int thumbPin      = A4;   //change the thumb position among two preset values
const int fingerPin     = A3;   //lock or unlock the position of middle finger, ring finger and pinky
const int analogInPin0  = A0;   //sensor input

float target = 0;
boolean thumbPinState   = 1;
boolean fingerPinState  = 1;

int swCount0    = 0;  //キャリブスイッチ用カウント変数
//int swCount1    = 0;  //未使用
int swCount2    = 0;  //ThumbPinの入力時に使うカウント用変数
int swCount3    = 0;  //FingerPinの入力時に使うカウント用変数
int swCountThr  = 6; //スイッチが押されたと判断するループ数

int sensorValue = 0;  // value read from the sensor
int sensorMax   = 700;
int sensorMin   = 0;
int threshold   = 0;

//speed settings
int speedMax  = 7;
int speedMin  = 0;
int speedRev  = -3;
int speed     = 0;

int positionMax = 150;
int positionMin = 0;
int position    = 0;
int prePosition = 0;

int thumbPos  = 90;
int indexPos  = 90;
int middlePos = 90;

int sensorMin_temp;   //キャリブレーション関数で使用

Servo myservo0;   //controls index finger
Servo myservo1;   //controls middle finger, ring finger and pinky
Servo myservo2;   //controls thumb


void setup() {
  
  //Serial.begin(9600);
  arm2tooth.begin(115200);

  pinMode(calibPin0, INPUT);
  digitalWrite(calibPin0, HIGH);

  pinMode(thumbPin, INPUT);
  digitalWrite(thumbPin, HIGH);

  pinMode(fingerPin, INPUT);
  digitalWrite(fingerPin, HIGH);

  myservo0.attach(3); //index
  myservo1.attach(5); //middle
  myservo2.attach(6); //thumb

}


void loop() {

  //初期のキャリブレーション処理
  while (1) {
    myservo0.write(IndexExtend);    //indexサーボを初期位置にする。
    myservo1.write(middleExtend);   //middleサーボを初期位置にする。
    myservo2.write(thumbExtend);    //Thumb open
    //Serial.println("Waiting for Calibration...");
    arm2tooth.println("Waiting for Calibration...");
    delay(10);
    if (digitalRead(calibPin0) == LOW) {
      calibration();
      break;
    }
  }

  while (1) { //通常動作ループ

    sensorValue = ReadSens_and_Condition();
    delay(25);

    if (digitalRead(calibPin0) == LOW) swCount0 += 1;
    else swCount0 = 0;

    if (swCount0 == swCountThr) {
      swCount0 = 0;
      calibration();
    }

    if (digitalRead(thumbPin) == LOW) swCount2 += 1;
    else swCount2 = 0;

    if (swCount2 == swCountThr) {
      swCount2 = 0;
      thumbPinState = !thumbPinState;
      while (digitalRead(thumbPin) == LOW) {
        delay(1); //ボタンを押している間すすまない
      }
    }

    if (digitalRead(fingerPin) == LOW) swCount3 += 1;
    else swCount3 = 0;

    if (swCount3 == swCountThr) {
      swCount3 = 0;
      fingerPinState = !fingerPinState;
      while (digitalRead(fingerPin) == LOW) {
        delay(1);
      }
    }

    //status
    arm2tooth.print("Min=");
    arm2tooth.print(sensorMin);
    arm2tooth.print(",Max=");
    arm2tooth.print(sensorMax);
    arm2tooth.print(",Value=");
    arm2tooth.print(sensorValue);
    arm2tooth.print(",thumb=");
    arm2tooth.print(thumbPinState);
    arm2tooth.print(",finger=");
    arm2tooth.print(fingerPinState);
    arm2tooth.print(",indexPos=");
    arm2tooth.print(indexPos);
    arm2tooth.print(",thumb=");
    arm2tooth.print(swCount3);
    arm2tooth.print(",speed=");
    arm2tooth.println(speed);

    /*
    Serial.print("Min=");
    Serial.print(sensorMin);
    Serial.print(",Max=");
    Serial.print(sensorMax);
    Serial.print(",Value=");
    Serial.print(sensorValue);
    Serial.print(",thumb=");
    Serial.print(thumbPinState);
    Serial.print(",finger=");
    Serial.print(fingerPinState);
    Serial.print(",indexPos=");
    Serial.print(indexPos);
    Serial.print(",thumb=");
    Serial.print(swCount3);
    Serial.print(",speed=");
    Serial.print(speed);
    */

    //calculate speed
    if (sensorValue < (sensorMin + (sensorMax - sensorMin) / 8)) speed = speedRev;
    else if (sensorValue < (sensorMin + (sensorMax - sensorMin) / 4)) speed = 0;
    else speed = map(sensorValue, sensorMin, sensorMax, speedMin, speedMax);

    //calculate position
    position = prePosition + speed;
    if (position < positionMin) position = positionMin;
    if (position > positionMax) position = positionMax;
    prePosition = position;
    //motor
    indexPos = map(position, positionMin, positionMax, IndexExtend, IndexFlex);
    myservo0.write(indexPos);

    //三指のスイッチ・オン状態だったらサーボを動かす。それ以外は動かない。
    if (fingerPinState == HIGH) {
      middlePos = map(position, positionMin, positionMax, middleExtend, middleFlex);
      myservo1.write(middlePos);
    }

    //親指の駆動
    switch (thumbPinState) {
      case 0://pinch
        myservo2.write(thumbPinch);
        break;
      case 1://open
        myservo2.write(thumbExtend);
        break;
      default:
        break;
    }
  
  } //while
}   //Main loop


//以下各種関数群

//センサ読み取り
int ReadSens_and_Condition() {
  
  int i;
  int sval;
  for (i = 0; i < 20; i++) {
    sval = sval + abs(1023 - analogRead(analogInPin0)); //for SensorBoard_v1_1
  }
  sval = sval / 20;
  return sval;
  
}

//センサ入力範囲キャリブレーション
void calibration() {

  myservo0.write(IndexExtend);  //indexサーボを初期位置にする。
  myservo1.write(middleFlex);   //middleサーボを初期位置にする。
  myservo2.write(thumbExtend);  //Thumb open

  arm2tooth.println("please wait...");
  //Serial.println("please wait...");
  delay(500);
  arm2tooth.println("start");
  //Serial.println("start");

  sensorMin = ReadSens_and_Condition();
  sensorMax = sensorMin + 1;

  unsigned long time = millis();

  while ( millis() < time + 5000 ) {

    //距離を取得
    sensorValue = ReadSens_and_Condition();
    delay(25);

    //最大値最小値の更新
    if ( sensorValue < sensorMin ) {
      sensorMin = sensorValue;
      sensorMin_temp = sensorMin + (sensorMax - sensorMin) / 4;
    }
    else if ( sensorValue > sensorMax )sensorMax = sensorValue;
    else;

    //calculate speed
    if (sensorValue < (sensorMin_temp + (sensorMax - sensorMin) / 8)) speed = speedRev;
    else if (sensorValue < (sensorMin_temp + (sensorMax - sensorMin) / 4)) speed = 0;
    else speed = map(sensorValue, sensorMin_temp, sensorMax, speedMin, speedMax);

    //calculate position
    position = prePosition + speed;
    if (position < positionMin) position = positionMin;
    if (position > positionMax) position = positionMax;
    prePosition = position;
    //motor
    indexPos = map(position, positionMin, positionMax, IndexExtend, IndexFlex);
    myservo0.write(indexPos);

    arm2tooth.print("Min=");
    arm2tooth.print(sensorMin);
    arm2tooth.print(",Min_temp=");
    arm2tooth.print(sensorMin_temp);
    arm2tooth.print(",Max=");
    arm2tooth.print(sensorMax);
    arm2tooth.print(",Value=");
    arm2tooth.print(sensorValue);
    arm2tooth.print(",time=");
    arm2tooth.print(time);
    arm2tooth.print(",millis=");
    arm2tooth.print(millis());
    arm2tooth.print("Button=");
    arm2tooth.println(digitalRead(calibPin0));

    /*
    Serial.print("Min=");
    Serial.print(sensorMin);
    Serial.print(",Min_temp=");
    Serial.print(sensorMin_temp);
    Serial.print(",Max=");
    Serial.print(sensorMax);
    Serial.print(",Value=");
    Serial.print(sensorValue);
    Serial.print(",time=");
    Serial.print(time);
    Serial.print(",millis=");
    Serial.print(millis());
    Serial.print("Button=");
    Serial.println(digitalRead(calibPin0));
    */
  } //while
  
  sensorMin += (sensorMax - sensorMin) / 4;

}
