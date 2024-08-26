/*
   ____  __  __  ____  _  _  _____       ___  _____  ____  _  _ 
  (  _ \(  )(  )(_  _)( \( )(  _  )___  / __)(  _  )(_  _)( \( )
   )(_) ))(__)(  _)(_  )  (  )(_)((___)( (__  )(_)(  _)(_  )  ( 
  (____/(______)(____)(_)\_)(_____)     \___)(_____)(____)(_)\_)
  Official code for Arduino boards (and relatives)   version 3.4
  
  Duino-Coin Team & Community 2019-2022 © MIT Licensed
  https://duinocoin.com
  https://github.com/revoxhere/duino-coin
  If you don't know where to start, visit official website and navigate to
  the Getting Started page. Have fun mining!
*/

#pragma GCC optimize ("-Ofast") // Enable fast optimizations

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
typedef uint16_t uintDiff; // Use 16-bit for AVR
#else
typedef uint32_t uintDiff; // Use 32-bit for others
#endif

#include "uniqueID.h"
#include "sha1.h"

String lastblockhash = "";
String newblockhash = "";
String DUCOID = "";
uintDiff difficulty = 0;
uintDiff ducos1result = 0;

const uint16_t job_maxsize = 104;  
uint8_t job[job_maxsize];
Sha1Wrapper Sha1_base;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  DUCOID = get_DUCOID();
  Serial.begin(115200);
  Serial.flush(); // Flush any previous data
}

// DUCO-S1A hasher
uintDiff ducos1a(const String& lastblockhash, const String& newblockhash, uintDiff difficulty) {
  newblockhash.toUpperCase();
  const char *c = newblockhash.c_str();
  uint8_t final_len = newblockhash.length() >> 1;

  for (uint8_t i = 0, j = 0; j < final_len; i += 2, j++) {
    job[j] = ((((c[i] & 0x1F) + 9) % 25) << 4) + ((c[i + 1] & 0x1F) + 9) % 25);
  }

  #if defined(ARDUINO_ARCH_AVR)
    if (difficulty > 655) return 0; // Check difficulty for AVR
  #endif

  Sha1_base.init();
  Sha1_base.print(lastblockhash);

  for (uintDiff ducos1res = 0; ducos1res <= difficulty * 100; ducos1res++) {
    Sha1_base.print(ducos1res);
    uint8_t *hash_bytes = Sha1_base.result();

    if (memcmp(hash_bytes, job, SHA1_HASH_LEN) == 0) {
      return ducos1res; // Return the result if hash matches
    }

    Sha1_base.init(); // Reset SHA1 for the next iteration
  }
  return 0;
}

String get_DUCOID() {
  String ID = "DUCOID";
  char buff[4];
  for (size_t i = 0; i < 8; i++) {
    sprintf(buff, "%02X", (uint8_t)UniqueID8[i]);
    ID += buff;
  }
  return ID;
}

void loop() {
  if (Serial.available() > 0) {
    memset(job, 0, job_maxsize);
    lastblockhash = Serial.readStringUntil(',');
    newblockhash = Serial.readStringUntil(',');
    difficulty = strtoul(Serial.readStringUntil(',').c_str(), NULL, 10);
    while (Serial.available()) Serial.read();

    digitalWrite(LED_BUILTIN, LOW); // Turn on LED

    uint32_t startTime = micros();
    ducos1result = ducos1a(lastblockhash, newblockhash, difficulty);
    uint32_t elapsedTime = micros() - startTime;

    digitalWrite(LED_BUILTIN, HIGH); // Turn off LED

    Serial.print(String(ducos1result) + "," + String(elapsedTime) + "," + DUCOID + "\n");
  }
}
