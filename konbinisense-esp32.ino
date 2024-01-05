#include <ESP32Servo.h>
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Hardware pins
#define TRIG_PIN1 15 // ultrasonic sensor 1 trigger pin
#define ECHO_PIN1 2 // ultrasonic sensor 1 echo pin
#define TRIG_PIN2 16 // ultrasonic sensor 2 trigger pin
#define ECHO_PIN2 17 // ultrasonic sensor 2 echo pin
#define SERVO_PIN 18 // servo motor pin
#define LED_PIN 5 // LED indicator pin

// Global variables
int trashLevel = 0; // percentage of trash level
bool isPersonDetected = false;
const int PERSON_DISTANCE_THRESHOLD = 25; // in centimeters
const int TRASH_FULL_THRESHOLD = 10;
String randomId;
bool hasRandomIdGenerated = false;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
float previousTrashLevel = -1;

//WIFI Credentials
#define SSID "oneword"
#define PASSWORD "batpigistheway"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAuxzg1wYoi3O9aDA7Ev1tY6Ie37jcUqIE"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://konbini-trashduino-default-rtdb.firebaseio.com/" 

//Define Firebase Data object
FirebaseData fbdo;

//firebase config
FirebaseConfig config;

//Firebase auth object
FirebaseAuth auth;


Servo trashLid; // servo object for trash bin lid




void setup() {
  // Setup serial communication
  Serial.begin(115200);

  // Connect to WiFi network
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting.......");
  }
  Serial.println("Connected to WiFi!");

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  // Setup ultrasonic sensors pins
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // Setup servo motor
  trashLid.attach(SERVO_PIN);
  trashLid.write(90); // initial position (closed)

  // Setup LED indicator pin
  pinMode(LED_PIN, OUTPUT);

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}



void loop() {
  long personDuration, personDistance, trashDuration, trashDistance;

 // measure person distance
  digitalWrite(TRIG_PIN1, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN1, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN1, LOW);
  float duration1 = pulseIn(ECHO_PIN1, HIGH);
  float distance1 = duration1 * 0.034 / 2;
  Serial.println("Person Distance :");
  Serial.println(distance1);
 

  // measure trash level
  digitalWrite(TRIG_PIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN2, LOW);
  float duration2 = pulseIn(ECHO_PIN2, HIGH);
  float distance2 = duration2 * 0.034 / 2;
  Serial.println("Trash Level:");
  Serial.println(distance2);
  


  // Check if a person is detected
  if (distance1 < PERSON_DISTANCE_THRESHOLD) {
    isPersonDetected = true;
  } else {
    isPersonDetected = false;
  }


    // Control the servo based on person detection
  if (!isPersonDetected) {
    for (int i = trashLid.read(); i >= 0; i -= 1) {
      trashLid.write(i); 
      delay(5);         
    }
    delay(500); // keep the lid closed for a while
  } else {
    for (int i = trashLid.read(); i <= 90; i += 1) {
      trashLid.write(i); 
      delay(5);         
    }
    delay(500); // keep the lid open for a while
  }
    float trashLevel = distance2; // Ensure trashLevel is a float
    Serial.print("Trash Level Sent: ");
    Serial.println(trashLevel);

    float currentTrashLevel = distance2;
    Serial.print("Trash Level Sent: ");
    Serial.println(currentTrashLevel);

    // Check if a randomId has been generated before
    if (!hasRandomIdGenerated) {
      // Generate a random alphanumeric ID with 9 characters
      String chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; // Characters to choose from
      randomId = "";
      
      for (int i = 0; i < 9; ++i) {
        randomId += chars[random(0, chars.length())]; // Append a random character from 'chars' to the ID
      }

      hasRandomIdGenerated = true; // Set the flag to indicate that a randomId has been generated
    }

    // Check if the trash level has changed
    if (currentTrashLevel != previousTrashLevel) {
      // Update the Firebase Realtime Database only if the trash level has changed
      if (Firebase.ready() && signupOK) {
        // Use the generated randomId within the condition block
        String firebasePath = "trashbin/" + randomId + "/capacityLevel";
        Firebase.RTDB.setFloat(&fbdo, firebasePath.c_str(), currentTrashLevel);
        Serial.print("Trash Level Sent: ");
        Serial.println(currentTrashLevel); // Display the updated trash level
        Serial.print("Random ID Sent: ");
        Serial.println(randomId);
        previousTrashLevel = currentTrashLevel; // Update the previous trash level
      }
    }
  }