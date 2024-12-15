#include <Servo.h>
//Wind Turbine SCADA System || Liam Brunet || December 1st, 2024

Servo myServo;
//Arduino Pin Setup         
const int turbineTempPin = A14;
const int outsideTempPin = A15;
const int turbineVoltagevalue = A5;
const int controlPin = 6; 
const int encoderPinA = 2;  // Rotary encoder CLK (A) pin
const int encoderPinB = 3;  // Rotary encoder DT (B) pin
const int servoPin = 9;     // Servo control pin

//Variables
int turbineTemp = 0;  
float turbinevoltage = 0; 
float turbineCurrent = 0.0;   
float turbinePower = 0.0;     
int on_off = 0;
int manual = 0; 
int outsideTemp = 0; 
volatile int encoderPos = 90;
int lastEncoderPos = 90; 
const int pulsesPerRevolution = 20;// Number of pulses per revolution of the encoder
const float degreesPerPulse = 360.0 / pulsesPerRevolution;  // Degrees per pulse 
float TurbinthermistorResistance = 0;
float outsidethermistorResistance = 0;
float turbinetemperatureC = 0;
float TurbinetemperatureK = 0;

float outsidetemperatureC = 0;
float outsidetemperatureK = 0;
const float seriesResistor = 10000.0; // 10kΩ resistor in the voltage divider
// Steinhart-Hart coefficients for the thermistor
const float A = 1.009249522e-3;
const float B = 2.378405444e-4;
const float C = 2.019202697e-7;


void setup() {
 Serial.begin(9600);// Start serial communication

  myServo.attach(servoPin); // Attach servo to pin 9
  myServo.write(90);         // Set initial position to 90 degrees (middle)

  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);

  digitalWrite(encoderPinA, HIGH);
  digitalWrite(encoderPinB, HIGH);
  pinMode(6, OUTPUT); 

  pinMode(controlPin, OUTPUT);

  // Attach interrupt to the encoder CLK pin (encoderPinA)
  attachInterrupt(digitalPinToInterrupt(encoderPinA), updateEncoder, CHANGE);


}
void loop() {
  //Gets the voltage from the Turbine temp voltage divider then gets the value of the thermistor resistance
  turbineTemp = analogRead(turbineTempPin);
  float Turbinetempvoltage = (turbineTemp / 1023.0) * 5.0;
  TurbinthermistorResistance = seriesResistor * (5.0 / Turbinetempvoltage - 1.0);
  
  //Calculates the tempurature on the Turbine temp thermistor using the Steinhart-Hart Equation
  float logR = log(TurbinthermistorResistance);
  float SH_equation_turbinetemp = A + B * logR + C * pow(logR, 3); // Steinhart-Hart equation
  TurbinetemperatureK = 1.0 / SH_equation_turbinetemp; 
  turbinetemperatureC = TurbinetemperatureK - 273.15; 

  //Gets the voltage from the outside temp voltage divider then gets the value of the thermistor resistance
  outsideTemp = analogRead(outsideTempPin);
  float outsidetempvoltage = (outsideTemp / 1023.0) * 5.0;
  outsidethermistorResistance = seriesResistor * (5.0 / outsidetempvoltage - 1.0);
  
  //Calculates the tempurature on the outside temp thermistor using the Steinhart-Hart Equation
  float llogR = log(outsidethermistorResistance); 
  float SH_equation_outsidetemp = A + B * llogR + C * pow(llogR, 3); // Steinhart-Hart equation
  outsidetemperatureK = 1.0 / SH_equation_outsidetemp;
  outsidetemperatureC = outsidetemperatureK - 273.15;

  float turbineVoltagevalue = analogRead(A5);
  turbinevoltage = (turbineVoltagevalue / 1023.0) * 5.0;

  turbineCurrent = (turbinevoltage / 330)*1000;

  turbinePower = (turbinevoltage * turbineCurrent);

  

 if (Serial.available() > 0) {  // Check if there is data available from the serial port
    String inputString = Serial.readStringUntil('\n');  // Read the incoming string until newline
    
    // Look for the keyword "servo" in the input string
    int servoIndex = inputString.indexOf("servo");
    if (servoIndex != -1) {  // If "servo" is found in the string find the space after "servo" and get the number
        
        int spaceIndex = inputString.indexOf(' ', servoIndex);
        int commaIndex = inputString.indexOf(',', spaceIndex);  // Find the next comma after the space

        if (spaceIndex != -1) {
            // Extract the number that comes after "servo"
            String angleString;
            if (commaIndex != -1) {
                angleString = inputString.substring(spaceIndex + 1, commaIndex);  // Up to the comma
            } else {
                angleString = inputString.substring(spaceIndex + 1);  // Until the end if no comma
            }

            // Convert the substring to an integer and check if it's valid
            int angle = angleString.toInt();
            if (angleString.length() > 0 && angle >= 0 && angle <= 180) {  // Validate angle
                encoderPos = angle;
                myServo.write(encoderPos);  // Move the servo to the specified angle
                Serial.print("Servo moved to: ");
                Serial.println(encoderPos);  // Print confirmation
          
            }
        }
    }

    inputString.trim();  
    if (inputString.indexOf("Manual") != -1) {
      manual = 1;  // Set mode to Manual
    } else if (inputString.indexOf("Automatic") != -1) {
      manual = 0;  // Set mode to Automatic
    }

    inputString.trim(); // removes trailing spaces or newline characters
    if (inputString.indexOf("STOP_ON") != -1) {
      on_off = 1;  // Set mode to Manual
    } else if (inputString.indexOf("STOP_OFF") != -1) {
      on_off = 0;  // Set mode to Automatic
    }

    // Based on the value of 'manual', set the pin to HIGH or LOW
    if (on_off == 1) {
      digitalWrite(controlPin, HIGH);  // Manual mode: Set pin 6 HIGH
    } else {
      digitalWrite(controlPin, LOW);  // Automatic mode: Set pin 6 LOW
    }
  }
  //sends if the turbine is spinning or not to the Python HMI
  if (turbinevoltage > 1 ) {
    Serial.println("spinning "); 
  }else {
    Serial.print("not ");
  }
  if (manual == 0 && encoderPos != lastEncoderPos) {
    lastEncoderPos = encoderPos;  // Update the last encoder position
    myServo.write(encoderPos);  
    Serial.print("Servo: ");      // Print the label "Servo:"
    Serial.print(encoderPos);   // Send updated position back to Python for display
    Serial.print(",");
 
  }
  //sends the turbine data to the python HMI over serial.
  Serial.print("RPM: ");
  Serial.print(turbinevoltage); 
  Serial.print(",");
  
  Serial.print("Turbine Temperature: ");
  Serial.print(turbinetemperatureC); 
  Serial.print(",");

  Serial.print("Outside Temperature: ");
  Serial.print(outsidetemperatureC);
  Serial.print(",");

  Serial.print("Turbine Voltage: ");
  Serial.print(turbinevoltage);
  Serial.print(",");

  Serial.print("Turbine Current: ");
  Serial.print(turbineCurrent);
  Serial.print(",");

  Serial.print("Power: ");
  Serial.print(turbinePower);
  Serial.println(",");

}

void updateEncoder() {
  // If we're in manual mode, skip processing the encoder input
  if (manual == 1) {
    return; 
  }

  static int lastStateA = LOW;  // Last state of CLK (A)
  
  int currentStateA = digitalRead(encoderPinA);  // Read current state of A
  int currentStateB = digitalRead(encoderPinB);  // Read current state of B

 // Determine direction by checking the state of encoderPinB
  if (currentStateA == HIGH && lastStateA == LOW) {  // Rising edge on A
    if (currentStateB == LOW) {
      encoderPos -= 9;  // Clockwise rotation (subtract)
    } else {
      encoderPos += 9;  // Counter-clockwise rotation (add)
    }
  } else if (currentStateA == LOW && lastStateA == HIGH) {  // Falling edge on A
    if (currentStateB == HIGH) {
      encoderPos -= 9;  // Clockwise rotation (subtract)
    } else {
      encoderPos += 9;  // Counter-clockwise rotation (add)
    }
  }

  // Ensure encoder position stays within 0-180 degrees range
  if (encoderPos < 0) {
    encoderPos = 0;  // Prevent going below 0 degrees
  } else if (encoderPos > 180) {
    encoderPos = 180;  // Prevent going above 180 degrees
  }

  // Update last state
  lastStateA = currentStateA;
}

