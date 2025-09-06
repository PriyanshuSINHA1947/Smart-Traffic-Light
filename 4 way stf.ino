// Pin mappings for 4 lanes: North, East, South, West
const int trigPins[4] = { 32,33,26,27 };
const int echoPins[4] = { 25,14,12,13 };

const int redPins[4] = { 4,16,7,2 };
const int yellowPins[4] = { 0,21,22,3 };
const int greenPins[4] = { 1,34,35,5 };

const long distanceThreshold = 30;  // cm

bool emergencyMode = false;
unsigned long emergencyStartTime = 0;

String emergencyLane = "";  // Store lane ID from RFID
unsigned long lastSwitchTime = 0;
int currentGreen = 0;                // Active green direction (0-3)
unsigned long greenDuration = 5000;  // Default duration in ms

// Assuming RFID is handled as in the previous (pin-based) example
#include <SPI.h>
#include <MFRC522.h>

// ESP32 pins connected to RC522 RFID module (ADJUST THESE TO YOUR WIRING!)
#define SS_PIN 15     // Slave Select (SDA on some modules)
#define RST_PIN 19   // Reset pin (if your module has one, -1 if not connected)
#define MOSI_PIN 23  // Master Out Slave In
#define MISO_PIN 36  // Master In Slave Out
#define SCK_PIN 18   // Serial Clock

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(115200);  // Increased baud rate for general debugging
  delay(1);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN); // Initialize SPI bus
  mfrc522.PCD_Init(); // Initialize MFRC522 reader

  Serial.println("RFID Reader Initialized");
  mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details

  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);

    pinMode(redPins[i], OUTPUT);
    // pinMode(yellowPins[i], OUTPUT);
    pinMode(greenPins[i], OUTPUT);
  }
  // Serial.println("should work...");

  setLights(currentGreen);  // Start with lane 0 green
}

void loop() {
  checkRFID();
  // yield(); // feeds the watchdog, especially important on ESP32

  // // Debug to confirm loop runs
  // Serial.println("Loop running...");
  // delay(100);
  // if (millis() > 10000 && emergencyLane == "") {
  //   emergencyLane = "EAST123";  // Simulate reading an East lane emergency tag after 10 seconds
  //   Serial.println("Simulating emergency RFID: EAST123");
  // }

  // if (emergencyLane != "") {
  //   int lane = laneFromID(emergencyLane);
  //   if (lane != -1) {
  //     setLights(lane);
  //     delay(7000); // Give emergency vehicle priority
  //     emergencyLane = ""; // Reset after passing
  //   }
  // }
  if (emergencyLane != "") {
    int lane = laneFromID(emergencyLane);
    if (!emergencyMode && lane != -1) {
      setLights(lane);
      emergencyStartTime = millis();
      emergencyMode = true;
    }

    if (emergencyMode && millis() - emergencyStartTime >= 7000) {
      emergencyLane = "";
      emergencyMode = false;
      lastSwitchTime = millis();  // Reset timing for normal operation
    }
  } else {
    unsigned long now = millis();
    if (now - lastSwitchTime > greenDuration) {
      int nextLane = selectNextLane();
      setLights(nextLane);
      lastSwitchTime = now;
    }
  }
}

// Select next lane based on vehicle presence
int selectNextLane() {
  for (int i = 1; i <= 4; i++) {
    int index = (currentGreen + i) % 4;
    if (vehicleDetected(index)) {
      return index;
    }
  }
  // If none detected, rotate normally
  return (currentGreen + 1) % 4;
}

// Measure distance from ultrasonic sensor
bool vehicleDetected(int lane) {
  digitalWrite(trigPins[lane], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[lane], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[lane], LOW);

  long duration = pulseIn(echoPins[lane], HIGH);
  long distance = duration * 0.034 / 2;
  return (distance > distanceThreshold);
  // return false;
}
// bool vehicleDetected(int lane) {
//   if (lane != 0) return false;

//   digitalWrite(trigPins[lane], LOW);
//   delayMicroseconds(2);
//   digitalWrite(trigPins[lane], HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPins[lane], LOW);

//   long duration = pulseIn(echoPins[lane], HIGH, 30000); // 30ms timeout
//   long distance = duration * 0.034 / 2;
//   Serial.print("Lane 0 distance: ");
//   Serial.println(distance);
//   return (distance > 0 && distance < distanceThreshold);
// }

// Update traffic lights and print status
void setLights(int greenLane) {
  for (int i = 0; i < 4; i++) {
    String laneName;
    switch (i) {
      case 0: laneName = "North"; break;
      case 1: laneName = "East"; break;
      case 2: laneName = "South"; break;
      case 3: laneName = "West"; break;
      default: laneName = "Unknown"; break;
    }

    if (i == greenLane) {
      digitalWrite(greenPins[i], HIGH);
      digitalWrite(redPins[i], LOW);
      // digitalWrite(yellowPins[i], LOW);
      delay(1000);
      Serial.print(laneName);
      Serial.println(": Green");
    } else {
      digitalWrite(redPins[i], HIGH);
      digitalWrite(greenPins[i], LOW);
      // digitalWrite(yellowPins[i], LOW);
      Serial.print(laneName);
      Serial.println(": Red");
    }
  }
  currentGreen = greenLane;
  Serial.println("---");  // Separator for clarity
}

// Handle RFID input from MFRC522
void checkRFID() {
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("Card detected, UID: ");
    String tagID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      tagID += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println(tagID);
    emergencyLane = mapTagIDToLane(tagID);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

// Map RFID tag UID to lane index
String mapTagIDToLane(String uid) {
  if (uid == "YOUR_NORTH_TAG_UID") return "NORTH123";  // Replace with your actual UID
  if (uid == "YOUR_EAST_TAG_UID") return "EAST123";    // Replace with your actual UID
  if (uid == "YOUR_SOUTH_TAG_UID") return "SOUTH123";  // Replace with your actual UID
  if (uid == "YOUR_WEST_TAG_UID") return "WEST123";    // Replace with your actual UID
  return "";                                           // Return empty string if no match
}

// Map RFID tag ID (your original format) to lane index (still used internally)
int laneFromID(String id) {
  if (id == "NORTH123") return 0;
  if (id == "EAST123") return 1;
  if (id == "SOUTH123") return 2;
  if (id == "WEST123") return 3;
  return -1;
}