#include <Servo.h>

// settings
int lockPosition = 0;
int unlockPosition = 115;
int serialSpeed = 9600;

// pins
int servoPin = 11;
int greenLedPin = 12;
int redLedPin = 13;

// global servo
Servo servo;

// runs on start, takes a second to run
void setup()
{
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  digitalWrite(redLedPin,HIGH);
  digitalWrite(greenLedPin,HIGH);
  Serial.begin(serialSpeed);
  servo.attach(servoPin);
  lock();
  delay(500);
  servo.detach();
  digitalWrite(redLedPin,LOW);
  digitalWrite(greenLedPin,LOW);
}

// runs always after start
void loop()
{
    byte a;
    while (Serial.available())
    {
      Serial.write('R');
      a = Serial.read();
      if (a == 'g') //good
      {
        Serial.write('G');
        youShallPass();
      }else if (a == 'b') //bad
      {
        Serial.write('B');
        youShallNotPass();
      }      
    }
}

// good auth
void youShallPass()
{
  servo.attach(servoPin);
  unlock();
  digitalWrite(greenLedPin,HIGH);
  delay(5000);
  digitalWrite(greenLedPin,LOW);
  lock();
  delay(500); //enough time for the lock to close
  servo.detach();
}

// bad auth
void youShallNotPass()
{
  digitalWrite(redLedPin,HIGH);
  delay(500);
  digitalWrite(redLedPin,LOW);
}


// move the servo to the locked position
void lock()
{
  servo.write(lockPosition);
}

// move the servo to the unlocked position
void unlock()
{
  servo.write(unlockPosition);
}
