#include <Arduino.h>
#include <esp_task_wdt.h>
TaskHandle_t task1Handle;
// Motor A
int IN2 = 18; 
int IN1 = 19; 
int ENA = 2; 

// Motor B
int IN4 = 16; 
int IN3 = 17; 
int ENB = 15; 

//Entradas
int sensor = 14;
int botao = 36;
volatile bool buttonState = false;
const int pinoPotenciometro = 39;  // Pino analógico GPIO34 no ESP32


//pré sets pwm
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

//inicialização das variaveis 
float t1 = 0, t2 = 0, delta = 0, uz= 0, uz1=0, ez=0, ez1=0, v=0;//tempos
float vel = 0;

float velocidade(){
 delta = t2 - t1;
 if (delta > 150)
 {
  delta = 150;
 }
 if(delta < 5 ){
  delta = 5;
 }

 vel = 60000/(delta * 15); //tepos de uma volta em (ms)
 return vel;
}
 
void task1(void *parameter) 
{
  volatile bool buttonPressed = false;
  volatile bool encoderin = false;
  volatile bool lastencoderin = false;

    while (true) {
      encoderin = digitalRead(sensor);
      if (encoderin == HIGH && lastencoderin == LOW) {
        t1 = t2;
        t2 = (esp_timer_get_time()/1000);
        buttonPressed = true;
      }

      lastencoderin = encoderin;
      vTaskDelay(pdMS_TO_TICKS(1)); // Aguarda 10 milissegundos
    esp_task_wdt_reset();
    }
  
}

void setup() {

  // sets the pins as outputs:
  pinMode(IN2, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(sensor, INPUT_PULLUP);
  pinMode(botao, INPUT_PULLUP);

  xTaskCreatePinnedToCore(task1, "Task 1", 4096, NULL, 1, &task1Handle, 0);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
   ledcAttachPin(ENA, pwmChannel);
   ledcAttachPin(ENB, pwmChannel); 


  Serial.begin(115200);

}

void loop() {
  buttonState = digitalRead(botao);
  int v = map(analogRead(pinoPotenciometro), 0, 4095, 150, 220);

 if (buttonState == LOW) {

      ez =  v-velocidade();
      uz = uz1 + 0.002092172678595 * ez * 1.5 - 0.001944587 * ez1 * 1.5;

      digitalWrite(IN2, LOW);
      digitalWrite(IN1, HIGH); 
      digitalWrite(IN4, LOW);
      digitalWrite(IN3, HIGH); 
      uz1 = uz;
      ez1 = ez;

     
      dutyCycle =uz;

      if(dutyCycle >= 255){
        dutyCycle = 255;
        uz = 0.99;
      }
      if(dutyCycle <=150 ){
        dutyCycle = 150;
        uz = 0.588;
      }

      ledcWrite(pwmChannel, dutyCycle);
      Serial.println("set");
      Serial.println(v);
       //Serial.print(delta);
    //   Serial.print(" ------------ ");
      Serial.println("velocidade");
      Serial.println(vel);

  } 
  else 
  {
      digitalWrite(IN2, LOW);
      digitalWrite(IN1, LOW); 
      digitalWrite(IN4, LOW);
      digitalWrite(IN3, LOW); 
      dutyCycle = 0;
      uz = 0;
      uz1 = 0;
      ez = 0;
      ez1 = 0;
    
  }
 // delay(10);
 
}