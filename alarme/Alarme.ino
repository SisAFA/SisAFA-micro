//Detector de movimento acelerometro
//Global Variables and constants
    const int Ativar = 10; // Alarm button ativation connected to Digital 10
    const int buzzerPin = A1; // buzzer Pin connected to Analog 1
    const int interruptor1 = A0; // interruptor do sensor de porta para Analog 0
    const int interruptor2 = A2; // interruptor do sensor de porta para Analog 2
    const int vibra = 2;
    
//Accelerometer Pins
    const int x = A3; // X pin connected to Analog 3
    const int y = A4; // Y pin connected to Analog 4
    const int z = A5; // Z pin connected to Analog 5
//Alarm LED
    const int ledPin = 8; // LED connected to Digital 8
    int tolerance=40; // Sensitivity of the Alarm
    boolean calibrate = false; // When accelerometer is calibrated - changes to true 
    boolean moveDetected = false; // When motion is detected - changes to true
//Accelerometer limits
    int xMin; //Minimum x Value
    int xMax; //Maximum x Value
    int xVal; //Current x Value
    int yMin; //Minimum y Value
    int yMax; //Maximum y Value
    int yVal; //Current y Value
    int zMin; //Minimum z Value
    int zMax; //Maximum z Value
    int zVal; //Current z Value
//Vibração
    boolean leitura_porta_vibra = false;
    boolean vibeRing = false;
    boolean doorRing = false;
 
void setup(){
    //Initilise LED Pin
    pinMode(ledPin, OUTPUT);
    pinMode (Ativar,INPUT);
    //Vibração
    pinMode(vibra, INPUT);
    delay(500);
    calibrateAccel();
}
void loop()
{  
     boolean ativa = digitalRead(Ativar);
     // If the button is pressed, initialise and recalibrate the Accelerometer limits.  
     if(ativa && !calibrate){
         calibrateAccel();
     }else if(!ativa && calibrate){
         calibrate = false;
     }
   
     vibeRing = false;
     doorRing = false;
     
       //configurar entrada da chave ativar (criar função) e nova para sensor de vibração
       while (digitalRead(Ativar)) {
           
           // Once the accelerometer is calibrated - check for movement 
           if(checkMotion()){
              moveDetected=true;
           }
           
           // If motion is detected - sound the alarm !
           if(moveDetected){
               ALARM();
//               moveDetected = false;
               delay(1000);
           }
           
           if(analogRead(interruptor1)>500 || analogRead(interruptor2)>500){
               doorRing = true;
           }
           
           if(doorRing){
             ALARM();
             delay(1000);
           }
           
           if(vibration()){
              vibeRing = true;
           }
           
           if(vibeRing){
               ALARM();
               delay(1000);
           }
       }
}
//This is the function used to sound the buzzer
  void buzz(int reps, int rate){
    for(int i=0; i<reps; i++){
      analogWrite(buzzerPin, 220);
      delay(100);
      analogWrite(buzzerPin, 0);
      delay(rate);
    }
} 
// Function used to calibrate the Accelerometer
void calibrateAccel(){
    // reset alarm
    moveDetected=false;
    // Calibration sequence initialisation sound - 3 seconds before calibration begins
    buzz(1,150);
    //calibrate the Accelerometer (should take about 0.5 seconds)
    for (int i=0; i<50; i++){
        // Calibrate X Values
        xVal = analogRead(x);
        xMax=xVal+tolerance;
        xMin=xVal-tolerance;
      
        //  Calibrate Y Values
        yVal = analogRead(y);
        yMax=yVal+tolerance;
        yMin=yVal-tolerance;
      
        // Calibrate Z Values
        zVal = analogRead(z);
        zMax=zVal+tolerance;
        zMin=zVal-tolerance;
      
        //Delay 10msec between readings
        delay(10);
    } 
    //End of calibration sequence sound. ARMED.
    buzz(2,40);
    printValues(); //Only useful when connected to computer- using serial monitor.
    calibrate=true;
}
//Function used to detect motion. Tolerance variable adjusts the sensitivity of movement detected.
boolean checkMotion(){
     boolean tempB=false;
     xVal = analogRead(x);
     yVal = analogRead(y);
     zVal = analogRead(z);
     
     if(xVal > xMax|| xVal < xMin){
         tempB=true;
     }
     
     if(yVal > yMax || yVal < yMin){
         tempB=true;
     }
     
     if(zVal > zMax || zVal < zMin){
         tempB=true;
     }
     return tempB;
}
// Prints the Sensor limits identified during Accelerometer calibration.
// Prints to the Serial monitor.
void printValues(){
}
//Function used to make the alarm sound, and blink the LED.
void ALARM(){
 
 // sound the alarm and blink LED
 digitalWrite(ledPin, HIGH);
 buzz(4,20);
 digitalWrite(ledPin, LOW);
}

boolean vibration(){
  leitura_porta_vibra = digitalRead(vibra);
  return(leitura_porta_vibra);
}


