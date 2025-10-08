// #include <Arduino.h>
// #include <Wire.h>
// #include <MPU6050.h>

// // Create MPU6050 instance
// MPU6050 mpu;

// void setup() {
//   Serial.begin(115200);
//   while (!Serial) { delay(10); }

//   Wire.begin(21, 22);       // SDA = GPIO21, SCL = GPIO22
//   mpu.initialize();
//   if (!mpu.testConnection()) {
//     Serial.println("MPU6050 connection failed!");
//     while (1) { delay(1000); }
//   }
//   Serial.println("MPU6050 connected.");
// }

// void loop() {
//   int16_t ax, ay, az;
//   mpu.getAcceleration(&ax, &ay, &az);

//   // Convert to g’s (assuming ±2g range and 16 384 LSB/g)
//   float x = ax / 16384.0f;
//   float y = ay / 16384.0f;
//   float z = az / 16384.0f;

//   // Compute roll & pitch (in degrees)
//   float roll  = atan2(y, z) * 180.0f / PI;
//   float pitch = atan2(-x, sqrt(y*y + z*z)) * 180.0f / PI;

//   Serial.print("Roll:  "); Serial.print(roll, 2);
//   Serial.print("°, Pitch: "); Serial.print(pitch, 2);
//   Serial.println("°");

//   delay(200);
// }



// ——— Includes & Defines ———
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MD_MAX72XX.h>
#include <MPU6050.h>
#include "Flip.h"                // your FLIP sim implementation

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   1
#define CS_PIN        5           // CS pin for your matrix

// ——— Simulation Parameters ———
static constexpr int   CELLS         = 20;
static constexpr int   MAX_PARTICLES = 100;
static constexpr float DOMAIN_W      = 1.0f;
static constexpr float DOMAIN_H      = 1.0f;
static constexpr float SPACING       = DOMAIN_W / (CELLS - 1);
static constexpr float PAR_RADIUS    = SPACING * 0.3f;

// ——— Hardware objects ———
MD_MAX72XX matrix(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MPU6050    mpu;
Flip fluid(
  /*density=*/     1000.0f,
  /*width=*/       DOMAIN_W,
  /*height=*/      DOMAIN_H,
  /*spacing=*/     SPACING,
  /*par_radius=*/  PAR_RADIUS,
  /*maxParticles=*/MAX_PARTICLES
);

// Seed particles in the top-left block
void seedParticles() {
  fluid.setParticleCount(MAX_PARTICLES);
  int cols = 5, rows = 20;
  for (int j = 0; j < rows; ++j) {
    for (int i = 0; i < cols; ++i) {
      int idx = j*cols + i;
      fluid.setParticlePos(idx,
        SPACING * i,
        SPACING * j
      );
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);         // SDA → 21, SCL → 22

  // init MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 failed to start!");
    while (1) { delay(1000); }
  }
  Serial.println("MPU6050 OK");

  // init matrix
  matrix.begin();
  matrix.control(MD_MAX72XX::INTENSITY, 5);
  matrix.clear();

  // init sim
  fluid.setup();
  seedParticles();
}

void loop() {
  // —— 1) simulate one frame ——  
  static unsigned long prevMicros = micros();
  unsigned long now = micros();
  float dt = (now - prevMicros) * 1e-6f;
  prevMicros = now;
  if (dt > 1.0f/30.0f) dt = 1.0f/30.0f;

  // —— 2) read tilt from MPU6050 ——  
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  // Convert raw to m/s² (assuming default ±2 g and 16 384 LSB/g):
  float gy = (float)ax / 16384.0f * 9.81f;
  float gx = -(float)ay / 16384.0f * 9.81f;
  // You may need to flip signs depending on your board orientation:
  //    gx = -gx; gy = -gy;

  // —— 3) run FLIP with real gravity ——  
  fluid.simulate(
    gx, gy, dt,
    /*flipRatio=*/     0.8f,
    /*pressureIters=*/ 2,
    /*particleIters=*/ 2,
    /*overRelaxation=*/1.9f,
    /*compensateDrift=*/true
  );

  matrix.setPoint(0, 1, true);

  // —— 4) draw onto the 8×8 matrix ——  
  matrix.clear();
  int count = fluid.getParticleCount();
  for (int i = 0; i < count; ++i) {
    float fx = fluid.getParticleX(i);
    float fy = fluid.getParticleY(i);
    int x = constrain(int(fx * 8), 0, 7);
    int y = constrain(int((1.0f - fy) * 8), 0, 7);
    matrix.setPoint(x, y, true);
  }
  matrix.update();

  delay(33);
}

// ////////////// take in user inputs
// // ——— Includes & Defines ———
// #include <Arduino.h>
// #include <SPI.h>
// #include <MD_MAX72XX.h>
// #include "Flip.h"                // your FLIP sim implementation

// #define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// #define MAX_DEVICES   1
// #define CS_PIN        5           // CS pin for your matrix

// // ——— Simulation Parameters (same as before) ———
// static constexpr int   CELLS         = 20;
// static constexpr int   MAX_PARTICLES = 100;
// static constexpr float DOMAIN_W      = 1.0f;
// static constexpr float DOMAIN_H      = 1.0f;
// static constexpr float SPACING       = DOMAIN_W / (CELLS - 1);
// static constexpr float PAR_RADIUS    = SPACING * 0.3f;

// // ——— 8×8 Matrix & FLIP sim ———
// MD_MAX72XX matrix(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Flip fluid(
//   /*density=*/     1000.0f,
//   /*width=*/       DOMAIN_W,
//   /*height=*/      DOMAIN_H,
//   /*spacing=*/     SPACING,
//   /*par_radius=*/  PAR_RADIUS,
//   /*maxParticles=*/MAX_PARTICLES
// );

// // Seed particles in the top-left block as before
// void seedParticles() {
//   fluid.setParticleCount(MAX_PARTICLES);
//   int cols = 5, rows = 20;
//   for (int j = 0; j < rows; ++j) {
//     for (int i = 0; i < cols; ++i) {
//       int idx = j*cols + i;
//       fluid.setParticlePos(idx,
//         SPACING * i,
//         SPACING * j
//       );
//     }
//   }
// }
// // at top, before setup()
// float gx = 0.0f, gy = -9.81f;  // gravity defaults

// void setup() {
//   Serial.begin(115200);
//   while (!Serial) { delay(10); }
//   Serial.println("Type new gravity as: <gx> <gy>  (e.g. 0 -9.81)");

//   // ... your existing init for Wire, MPU, matrix, fluid, etc.
//   // init matrix
//   matrix.begin();
//   matrix.control(MD_MAX72XX::INTENSITY, 5);  // brightness 0–15
//   matrix.clear();
//   // init sim
//   fluid.setup();
//   seedParticles();
// }

// void loop() {
//   // —— 0) Check for serial input to update gravity ——  
//   if (Serial.available()) {
//     String line = Serial.readStringUntil('\n');
//     line.trim();
//     if (line.length()) {
//       float newGx, newGy;
//       // parse two floats from the line
//       if (sscanf(line.c_str(), "%f %f", &newGx, &newGy) == 2) {
//         gx = newGx;
//         gy = newGy;
//         Serial.print("Gravity set to gx="); Serial.print(gx);
//         Serial.print(", gy="); Serial.println(gy);
//       } else {
//         Serial.println("Invalid input. Use: <gx> <gy>");
//       }
//     }
//   }

//   // —— 1) simulate one frame ——  
//   static unsigned long prevMicros = micros();
//   unsigned long now = micros();
//   float dt = (now - prevMicros) * 1e-6f;
//   prevMicros = now;
//   if (dt > 1.0f/30.0f) dt = 1.0f/30.0f;  // cap at 30 FPS

//   fluid.simulate(
//     gx, gy, dt,
//     /*flipRatio=*/     0.8f,
//     /*pressureIters=*/ 2,
//     /*particleIters=*/ 2,
//     /*overRelaxation=*/1.9f,
//     /*compensateDrift=*/true
//   );

//   // —— 2) draw onto the 8×8 matrix ——  
//   matrix.clear();
//   int count = fluid.getParticleCount();
//   for (int i = 0; i < count; ++i) {
//     float fx = fluid.getParticleX(i);
//     float fy = fluid.getParticleY(i);
//     int x = constrain(int(fx * 8), 0, 7);
//     int y = constrain(int((1.0f - fy) * 8), 0, 7);
//     matrix.setPoint(x, y, true);
//   }
//   matrix.update();

//   delay(33);
// }


// void setup() {
//   Serial.begin(115200);
//   // init matrix
//   matrix.begin();
//   matrix.control(MD_MAX72XX::INTENSITY, 5);  // brightness 0–15
//   matrix.clear();
//   // init sim
//   fluid.setup();
//   seedParticles();
// }

// void loop() {
//   // —— 1) simulate one frame ——  
//   static unsigned long prevMicros = micros();
//   unsigned long now = micros();
//   float dt = (now - prevMicros) * 1e-6f;
//   prevMicros = now;
//   if (dt > 1.0f/30.0f) dt = 1.0f/30.0f;  // cap at 30 FPS
  
//   // hard-code gravity downwards; replace with MPU6050 later
//   float gx = 0, gy = -9.81f;
//   fluid.simulate(
//     gx, gy, dt,
//     /*flipRatio=*/     0.8f,
//     /*pressureIters=*/ 2,
//     /*particleIters=*/ 2,
//     /*overRelaxation=*/1.9f,
//     /*compensateDrift=*/true
//   );

//   // —— 2) draw onto the 8×8 matrix ——  
//   // clear previous frame
//   matrix.clear();

//   matrix.setPoint(0, 0, true);
  
//   int count = fluid.getParticleCount();
//   for (int i = 0; i < count; ++i) {
//     // fluid.getParticleX/Y in [0,1]
//     float fx = fluid.getParticleX(i);
//     float fy = fluid.getParticleY(i);
//     // map to 0–7
//     int x = constrain(int(fx * 8), 0, 7);
//     int y = constrain(int((1.0 - fy) * 8), 0, 7);  
//     matrix.setPoint(x, y, true);
//   }
//   matrix.update();

//   // small delay so you can see it
//   delay(33);
// }


// // // #include <SPI.h>
// // // #include <MD_MAX72XX.h>

// // // #define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// // // #define MAX_DEVICES   1
// // // #define CS_PIN        5

// // // MD_MAX72XX matrix(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// // // void setup() {
// // //   matrix.begin();
// // //   matrix.control(MD_MAX72XX::INTENSITY, 8);
// // // }

// // // void loop() {
// // //   // all columns on
// // //   for (uint8_t dev = 0; dev < matrix.getDeviceCount(); dev++) {
// // //     for (uint8_t col = 0; col < 4; col++) {
// // //       matrix.setColumn(dev, col, 0xFF);
// // //     }
// // //   }
// // //   matrix.update();
// // //   delay(500);

// // //   // clear display
// // //   matrix.clear();
// // //   matrix.update();
// // //   delay(500);
// // // }
