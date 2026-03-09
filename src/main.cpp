



// ——— Includes & Defines ———
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MD_MAX72XX.h>
#include <MPU6050.h>
#include "Flip.h"                // FLIP sim implementation

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   1
#define CS_PIN        5           // CS pin for matrix

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


