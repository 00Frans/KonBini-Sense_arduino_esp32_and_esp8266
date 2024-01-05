#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <Servo.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Hardware pins
#define TRIG_PIN1 D2 // ultrasonic sensor 1 trigger pin (change the pin numbers if necessary)
#define ECHO_PIN1 D3 // ultrasonic sensor 1 echo pin
#define TRIG_PIN2 D4 // ultrasonic sensor 2 trigger pin
#define ECHO_PIN2 D0 // ultrasonic sensor 2 echo pin

#define SERVO_PIN D1 // servo motor pin
#define LED_PIN D2 // LED indicator pin

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
// #define SSID "oneword"
// #define PASSWORD "batpigistheway"
// #define SSID "UC-Legit Wifi"
// #define PASSWORD "A123456789"
#define SSID "POCO F4"
#define PASSWORD "xymer123"
// Insert Firebase project API Key
#define API_KEY "AIzaSyAuxzg1wYoi3O9aDA7Ev1tY6Ie37jcUqIE"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://konbini-trashduino-default-rtdb.firebaseio.com/" 

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "cejudofrans411@gmail.com"
#define USER_PASSWORD "password12345"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

Servo trashLid; // servo object for trash bin lid




void setup() {
  // Setup serial communication
  Serial.begin(115200);
// Connect to WiFi network
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
    }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // // Connect to WiFi network
  // WiFi.begin(SSID, PASSWORD);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.println("Connecting.......");
  // }
  // Serial.println("Connected to WiFi!");

/* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
    /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

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

  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
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
    // Serial.print("Trash Level Sent: ");
    // Serial.println(trashLevel);

    // Serial.print("Trash Level Sent: ");
    // Serial.println(currentTrashLevel);

    // Check if a randomId has been generated before
    // if (!hasRandomIdGenerated) {
    //   // Generate a random alphanumeric ID with 9 characters
    //   String chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; // Characters to choose from
    //   randomId = "";
      
    //   for (int i = 0; i < 9; ++i) {
    //     randomId += chars[random(0, chars.length())]; // Append a random character from 'chars' to the ID
    //   }

    //   hasRandomIdGenerated = true; // Set the flag to indicate that a randomId has been generated
    // }
    // Check if the trash level has changed
    // if (currentTrashLevel != previousTrashLevel) {
    //   // Update the Firebase Realtime Database only if the trash level has changed
    //   if (Firebase.ready() && signupOK) {
    //     // Use the generated randomId within the condition block
    //     String firebasePath = "trashbin/TB123456/capacityLevel";
    //     // Serial.printf("Set float... %s\n", Firebase.setFloat(fbdo, F("/test/float"), currentTrashLevel) ? "ok" : fbdo.errorReason().c_str());
    //     Firebase.setFloat(fbdo, firebasePath.c_str(), currentTrashLevel);
    //     Serial.print("Trash Level Sent: ");
    //     Serial.println(currentTrashLevel); // Display the updated trash level
    //     previousTrashLevel = currentTrashLevel; // Update the previous trash level
    //   }
    // }
    float trashLevel = distance2; // Ensure trashLevel is a float
    Serial.print("Trash Level Sent: ");
    Serial.println(trashLevel);

    float currentTrashLevel = distance2;
    Serial.print("Trash Level Sent: ");
    Serial.println(currentTrashLevel);
    if((currentTrashLevel != previousTrashLevel) > TRASH_FULL_THRESHOLD){
      // Update the Firebase Realtime Database only if the trash level has changed
      if (Firebase.ready() && signupOK) {
        String firebasePath = "trashbin/TB20231361/capacityLevel";
        Firebase.setFloat(fbdo, firebasePath.c_str(), currentTrashLevel);
        Serial.print("Trash Level is Full: ");
        Serial.println(currentTrashLevel); // Display the updated trash level
        previousTrashLevel = currentTrashLevel; // Update the previous trash level
      }
    }
    // Check if the trash level has changed is not full
    else{
      // Update the Firebase Realtime Database only if the trash level has changed
      if (Firebase.ready() && signupOK) {
        // Use the generated randomId within the condition block
        String firebasePath = "trashbin/TB20231361/capacityLevel";
        Firebase.setFloat(fbdo, firebasePath.c_str(), currentTrashLevel);
        Serial.print("Trash Level Sent: ");
        Serial.println(currentTrashLevel); // Display the updated trash level
        previousTrashLevel = currentTrashLevel; // Update the previous trash level
      }
    }
    delay(2000);
  }