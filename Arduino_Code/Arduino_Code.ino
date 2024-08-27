/*

   ____  __  __  ____  _  _  _____       ___  _____  ____  _  _ 
  (  _ \(  )(  )(_  _)( \( )(  _  )___  / __)(  _  )(_  _)( \( )
   )(_) ))(__)(  _)(_  )  (  )(_)((___)( (__  )(_)(  _)(_  )  ( 
  (____/(______)(____)(_)\_)(_____)     \___)(_____)(____)(_)\_)
  Official code for Arduino boards (and relatives)   version 4.2
  
  Duino-Coin Team & Community 2019-2024 © MIT Licensed
  https://duinocoin.com
  https://github.com/revoxhere/duino-coin
  If you don't know where to start, visit official website and navigate to
  the Getting Started page. Have fun mining!
*/

/* For microcontrollers with low memory change that to -Os in all files,
for default settings use -O0. -O may be a good tradeoff between both */
#pragma GCC optimize ("-Ofast")
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
typedef uint32_t uintDiff;
#else
typedef uint32_t uintDiff;
#endif
#include "uniqueID.h"
#include "duco_hash.h"

String get_DUCOID() {
  String ID = "DUCOID";
  for (size_t i = 0; i < 8; i++) {
    ID += String(UniqueID8[i], HEX); // Directly convert to hex string
  }
  return ID;
}

String DUCOID = "";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  DUCOID = get_DUCOID();
  Serial.begin(115200);
  Serial.setTimeout(10000);
  while (!Serial); // Wait for serial connection
  Serial.flush();
}

void lowercase_hex_to_bytes(const char*hexDigest, uint8_t* rawDigest) {
  for (uint8_t i = 0; i < SHA1_HASH_LEN; i++) {
    uint8_t high = (hexDigest[i *2] >= 'A') ? (hexDigest[i* 2] - 'A' + 10) : (hexDigest[i * 2] - '0');
    uint8_t low = (hexDigest[i *2 + 1] >= 'A') ? (hexDigest[i* 2 + 1] - 'A' + 10) : (hexDigest[i * 2 + 1] - '0');
    rawDigest[i] = (high << 4) | low; // Combine high and low
  }
}

uintDiff ducos1a(const char*prevBlockHash, const char* targetBlockHash, uintDiff difficulty) {
  #if defined(ARDUINO_ARCH_AVR)
    if (difficulty > 655) return 0; // Return 0 if difficulty is too high for AVR
  #endif

  uint8_t target[SHA1_HASH_LEN];
  lowercase_hex_to_bytes(targetBlockHash, target);

  uintDiff const maxNonce = difficulty * 100 + 1;
  return ducos1a_mine(prevBlockHash, target, maxNonce);
}

uintDiff ducos1a_mine(const char*prevBlockHash, const uint8_t* target, uintDiff maxNonce) {
  static duco_hash_state_t hash;
  duco_hash_init(&hash, prevBlockHash);

  char nonceStr[10 + 1];
  for (uintDiff nonce = 0; nonce < maxNonce; nonce++) {
    ultoa(nonce,    nonceStr, 10);  // Convert nonce to string
    const uint8_t* hash_bytes = duco_hash_try_nonce(&hash, nonceStr);
    
    if (memcmp(hash_bytes, target, SHA1_HASH_LEN) == 0) {
      return nonce;  // Return the nonce if the hash matches the target
    }
  }

  return 0;  // Return 0 if no valid nonce is found
}

void loop() {
  if (Serial.available() <= 0) return;

  char lastBlockHash[40 + 1];
  char newBlockHash[40 + 1];

  // Read last block hash
  if (Serial.readBytesUntil(',', lastBlockHash, 41) != 40) return;
  lastBlockHash[40] = 0;

  // Read new block hash
  if (Serial.readBytesUntil(',', newBlockHash, 41) != 40) return;
  newBlockHash[40] = 0;

  // Read difficulty
  uintDiff difficulty = strtoul(Serial.readStringUntil(',').c_str(), NULL, 10);
  while (Serial.available()) Serial.read();  // Clear the receive buffer

  digitalWrite(LED_BUILTIN, LOW);  // Turn on the built-in LED

  uint32_t startTime = micros();
  uintDiff ducos1result = ducos1a(lastBlockHash, newBlockHash, difficulty);
  uint32_t elapsedTime = micros() - startTime;

  digitalWrite(LED_BUILTIN, HIGH);  // Turn off the built-in LED

  // Prepare and send result
  while (Serial.available()) Serial.read();  // Clear the receive buffer before sending the result
  Serial.print(String(ducos1result, 2) + "," + String(elapsedTime, 2) + "," + String(DUCOID) + "\n");
}
