
// #include <Arduino.h>
// #include <SPI.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_ILI9341.h>
// #include <vector>       // for std::vector
// #include <algorithm>    // for std::min
// #include <cmath>        // for sqrtf()

// #include "Flip.h"         // Include your own class header

// #include <Arduino.h>             // always good to have first
// #include "esp32-hal-cpu.h"       // this comes with the ESP32 Arduino core


// Flip::Flip(float density,
//            int width,
//            int height,
//            float spacing,
//            float par_radius,
//            int maxParticles)
//   : density_(density),
//     x_cells_(int(std::floor(width  / spacing)) + 1),
//     y_cells_(int(std::floor(height / spacing)) + 1),
//     cell_num_(x_cells_ * y_cells_),
//     cell_size_(std::max(width/(float)x_cells_, height/(float)y_cells_)),

//     maxParticles_(maxParticles),
//     p_num_(0),
//     p_pos_(2*maxParticles_, 0.0f),
//     p_vel_(2*maxParticles_, 0.0f),

//     par_radius_(par_radius),
//     // par_spacing_ must wait until after par_radius_ is set:
//     par_spacing_(2.2f * par_radius_),

//     // we'll size the bucket‐vectors below in the constructor body:
//     p_x(0), p_y(0), pcell_num(0),

//     // MAC‐grid vectors—we know their size from cell_num_:
//     x(cell_num_, 0.0f),
//     y(cell_num_, 0.0f),
//     dx(cell_num_, 0.0f),
//     dy(cell_num_, 0.0f),
//     prev_x(cell_num_, 0.0f),
//     prev_y(cell_num_, 0.0f),
//     solid(cell_num_, 1.0f),
//     cell_type(cell_num_, AIR_CELL),
//     p_density(cell_num_, 0.0f),
//     rest_density(0.0f),
//     p(cell_num_, 0.0f)          // << add this
// {
//     // ── now that par_spacing_ is valid, build our particle‐bucket (hash) grid ──
//     p_x       = int(std::floor(width  / par_spacing_)) + 1;
//     p_y       = int(std::floor(height / par_spacing_)) + 1;
//     pcell_num = p_x * p_y;

//     per_cell   .assign(pcell_num,      0);
//     first_cell .assign(pcell_num + 1,  0);
//     cellpar_id .assign(maxParticles_,   0);

//     // mark solid boundaries (walls) in the MAC grid:
//     for (int i = 0; i < x_cells_; ++i) {
//         for (int j = 0; j < y_cells_; ++j) {
//             int idx = i * y_cells_ + j;
//             if (i == 0 || i == x_cells_-1 || j == 0 || j == y_cells_-1) {
//                 solid[idx]    = 0.0f;
//                 cell_type[idx] = SOLID_CELL;
//             }
//         }
//     }
// }

// int Flip::getGridWidth()  const { return x_cells_; }
// int Flip::getGridHeight() const { return y_cells_; }

// void Flip::setParticleCount(int n) 
// { 
//     p_num_ = std::min(n, maxParticles_); 
// }

// void Flip::setParticlePos(int i, float px, float py) {
//     if (i < 0 || i >= p_num_) return;
//     p_pos_[2*i  ] = px;
//     p_pos_[2*i+1] = py;
// }

// int   Flip::getParticleCount() const { return p_num_; }
// float Flip::getParticleX(int i) const { return p_pos_[2*i]; }
// float Flip::getParticleY(int i) const { return p_pos_[2*i+1]; }

// int Flip::clamp(int v, int lo, int hi) {
//   if (v < lo) return lo;
//   if (v > hi) return hi;
//   return v;
// }


// // gravity + simple Euler‐push
// void Flip::gravity(float gx, float gy, float dt) {
//     for (int i = 0; i < p_num_; ++i) {
//         p_vel_[2*i  ] += gx * dt;
//         p_vel_[2*i+1] += gy * dt;
//         p_pos_[2*i  ] += p_vel_[2*i  ] * dt;
//         p_pos_[2*i+1] += p_vel_[2*i+1] * dt;
//     }
// }

// // bounce off the domain walls
// void Flip::particleCollision() {
//     float minX = par_radius_, maxX = 1.0f - par_radius_;
//     float minY = par_radius_, maxY = 1.0f - par_radius_;

//     for (int i = 0; i < p_num_; ++i) {
//         float &px = p_pos_[2*i], &py = p_pos_[2*i+1];
//         float &vx = p_vel_[2*i], &vy = p_vel_[2*i+1];
//         if (px < minX) { px = minX; vx = 0; }
//         if (px > maxX) { px = maxX; vx = 0; }
//         if (py < minY) { py = minY; vy = 0; }
//         if (py > maxY) { py = maxY; vy = 0; }
//     }
// }

// // fill per_cell, prefix‐sum into first_cell, then flatten into cellpar_id
// void Flip::particle_grid(int iters) {
//     std::fill(per_cell.begin(), per_cell.end(), 0);

//     for (int i = 0; i < p_num_; ++i) {
//         float fx = p_pos_[2*i]   / par_spacing_;
//         float fy = p_pos_[2*i+1] / par_spacing_;
//         int   ix = clamp(int(std::floor(fx)), 0, p_x-1);
//         int   iy = clamp(int(std::floor(fy)), 0, p_y-1);
//         per_cell[ix * p_y + iy]++;
//     }

//     int sum = 0;
//     for (int c = 0; c < pcell_num; ++c) {
//         first_cell[c] = sum;
//         sum += per_cell[c];
//     }
//     first_cell[pcell_num] = sum;

//     for (int i = 0; i < p_num_; ++i) {
//         float fx = p_pos_[2*i]   / par_spacing_;
//         float fy = p_pos_[2*i+1] / par_spacing_;
//         int   ix = clamp(int(std::floor(fx)), 0, p_x-1);
//         int   iy = clamp(int(std::floor(fy)), 0, p_y-1);
//         int   idx = ix * p_y + iy;
//         first_cell[idx]--;
//         cellpar_id[first_cell[idx]] = i;
//     }

//     // simple particle‐separation pass (repel close neighbors)
//     float minDist = 2.0f * par_radius_;
//     for (int iter = 0; iter < iters; ++iter) {
//         for (int i = 0; i < p_num_; ++i) {
//             float px = p_pos_[2*i], py = p_pos_[2*i+1];
//             int ix = clamp(int(std::floor(px / par_spacing_)), 0, p_x-1);
//             int iy = clamp(int(std::floor(py / par_spacing_)), 0, p_y-1);
//             for (int dx = -1; dx <= 1; ++dx) {
//                 for (int dy = -1; dy <= 1; ++dy) {
//                     int cx = clamp(ix+dx, 0, p_x-1);
//                     int cy = clamp(iy+dy, 0, p_y-1);
//                     int start = first_cell[cx*p_y + cy];
//                     int end   = first_cell[cx*p_y + cy + 1];
//                     for (int k = start; k < end; ++k) {
//                         int j = cellpar_id[k];
//                         if (j == i) continue;
//                         float qx = p_pos_[2*j], qy = p_pos_[2*j+1];
//                         float dx = qx - px, dy = qy - py;
//                         float dist = std::sqrt(dx*dx + dy*dy);
//                         if (dist == 0 || dist > minDist) continue;
//                         float s = 0.5f*(minDist - dist)/dist;
//                         p_pos_[2*i]   -= dx*s;
//                         p_pos_[2*i+1] -= dy*s;
//                         p_pos_[2*j]   += dx*s;
//                         p_pos_[2*j+1] += dy*s;
//                     }
//                 }
//             }
//         }
//     }
// }

// void Flip::velocity_grid(bool toGrid, float flipRatio) {
//     int nX = x_cells_, nY = y_cells_, N = cell_num_;
//     float h = cell_size_, h2 = 0.5f*h;

//     if (toGrid) {
//         prev_x = x;  prev_y = y;
//         std::fill(x.begin(), x.end(), 0.0f);
//         std::fill(y.begin(), y.end(), 0.0f);
//         std::fill(dx.begin(), dx.end(), 0.0f);
//         std::fill(dy.begin(), dy.end(), 0.0f);

//         // mark solid vs air
//         for (int c = 0; c < N; ++c)
//             cell_type[c] = (solid[c] == 0.0f ? SOLID_CELL : AIR_CELL);
//         // any cell with a particle → FLUID_CELL
//         for (int p = 0; p < p_num_; ++p) {
//             float px = p_pos_[2*p], py = p_pos_[2*p+1];
//             int i = clamp(int(px/h), 0, x_cells_-1);
//             int j = clamp(int(py/h), 0, y_cells_-1);
//             int c = i*nY + j;
//             if (cell_type[c] == AIR_CELL) cell_type[c] = FLUID_CELL;
//         }
//     }

//     for (int comp = 0; comp < 2; ++comp) {
//         float offX = comp==0 ? 0.0f : h2;
//         float offY = comp==0 ? h2  : 0.0f;
//         auto &gridV = (comp==0 ? x : y);
//         auto &prevV = (comp==0 ? prev_x : prev_y);
//         auto &weight = (comp==0 ? dx : dy);

//         for (int p = 0; p < p_num_; ++p) {
//             float px = clamp(p_pos_[2*p]   , h, (x_cells_-1)*h);
//             float py = clamp(p_pos_[2*p+1] , h, (y_cells_-1)*h);
//             float fx = (px - offX)/h, fy = (py - offY)/h;
//             int i0 = std::min(int(std::floor(fx)), x_cells_-2),
//                 j0 = std::min(int(std::floor(fy)), y_cells_-2);
//             float tx = fx - i0, ty = fy - j0;
//             int i1 = i0+1, j1 = j0+1;
//             float w00 = (1-tx)*(1-ty), w10 = tx*(1-ty),
//                   w11 = tx*ty,       w01 = (1-tx)*ty;
//             int c00 = i0*nY + j0, c10 = i1*nY + j0,
//                 c11 = i1*nY + j1, c01 = i0*nY + j1;

//             if (toGrid) {
//                 float pv = p_vel_[2*p+comp];
//                 gridV[c00] += pv*w00; weight[c00] += w00;
//                 gridV[c10] += pv*w10; weight[c10] += w10;
//                 gridV[c11] += pv*w11; weight[c11] += w11;
//                 gridV[c01] += pv*w01; weight[c01] += w01;
//             } else {
//                 int stride = comp==0 ? nY : 1;
//                 float v00 = (cell_type[c00]!=AIR_CELL||cell_type[c00-stride]!=AIR_CELL)?w00:0;
//                 float v10 = (cell_type[c10]!=AIR_CELL||cell_type[c10-stride]!=AIR_CELL)?w10:0;
//                 float v11 = (cell_type[c11]!=AIR_CELL||cell_type[c11-stride]!=AIR_CELL)?w11:0;
//                 float v01 = (cell_type[c01]!=AIR_CELL||cell_type[c01-stride]!=AIR_CELL)?w01:0;
//                 float sum = v00+v10+v11+v01;
//                 if (sum<=0) continue;
//                 float oldV = p_vel_[2*p+comp];
//                 float picV = (v00*gridV[c00] + v10*gridV[c10]
//                             + v11*gridV[c11] + v01*gridV[c01]) / sum;
//                 float corr = ( v00*(gridV[c00]-prevV[c00])
//                             + v10*(gridV[c10]-prevV[c10])
//                             + v11*(gridV[c11]-prevV[c11])
//                             + v01*(gridV[c01]-prevV[c01]) ) / sum;
//                 p_vel_[2*p+comp] = (1-flipRatio)*picV + flipRatio*(oldV + corr);
//             }
//         }

//         if (toGrid) {
//             // normalize
//             for (int c = 0; c < N; ++c)
//                 if (weight[c]>0) gridV[c] /= weight[c];
//             // enforce no-slip
//             for (int i = 0; i < x_cells_; ++i)
//             for (int j = 0; j < y_cells_; ++j) {
//                 int c = i*nY + j;
//                 bool solidCell = cell_type[c]==SOLID_CELL;
//                 if (solidCell || (i>0 && cell_type[c-nY]==SOLID_CELL)) x[c]=prev_x[c];
//                 if (solidCell || (j>0 && cell_type[c-1]==SOLID_CELL)) y[c]=prev_y[c];
//             }
//         }
//     }
// }

// void Flip::solveIncompressibility(int numPressureIters,
//                                   float dt,
//                                   float overRelaxation,
//                                   bool compensateDrift) {
//     int nY = y_cells_;
//     // zero pressure
//     std::fill(p.begin(), p.end(), 0.0f);

//     for (int iter = 0; iter < 100; ++iter) {
//         for (int i = 1; i < x_cells_-1; ++i) {
//             for (int j = 1; j < y_cells_-1; ++j) {
//                 int c = i*nY + j;
//                 if (cell_type[c] != FLUID_CELL) continue;

//                 // neighbors
//                 int left  = (i-1)*nY + j,
//                     right = (i+1)*nY + j,
//                     down  = i*nY + j-1,
//                     up    = i*nY + j+1;

//                 float s0 = solid[left],  s1 = solid[right],
//                       s2 = solid[down],  s3 = solid[up];
//                 float denom = s0 + s1 + s2 + s3;
//                 if (denom == 0) continue;

//                 // divergence
//                 float div = x[right] - x[c] + y[up] - y[c];
//                 if (compensateDrift && rest_density>0.0f) {
//                     float comp = p_density[c] - rest_density;
//                     div -= comp;
//                 }

//                 float deltaP = -div/denom * overRelaxation;
//                 p[c] += deltaP;

//                 x[c]     -= s0 * deltaP;
//                 x[right] += s1 * deltaP;
//                 y[c]     -= s2 * deltaP;
//                 y[up]    += s3 * deltaP;
//             }
//         }
//     }
// }



// void Flip::simulate(float gx, float gy, float dt,
//                     float flipRatio,
//                     int numPressureIters,
//                     int numParticleIters,
//                     float overRelaxation,
//                     bool compensateDrift)
// {
//     gravity(gx, gy, dt);
//     particle_grid(numParticleIters);
//     particleCollision();
//     velocity_grid(true,  0.0f);
//     solveIncompressibility(numPressureIters, dt, overRelaxation, compensateDrift);
//     velocity_grid(false, flipRatio);
// }

// ——— Includes & Defines ———
#include <Arduino.h>
#include <esp32-hal-cpu.h>       // setCpuFrequencyMhz()
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <math.h>
#include "Flip.h"                // your FLIP sim implementation

#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4            // tie to RST or set to -1
#define DEG_TO_RAD 0.017453292519943295f

// ——— Simulation Parameters ———
static constexpr int   CELLS         = 20;
static constexpr int   MAX_PARTICLES = 100;
static constexpr float DOMAIN_W      = 1.0f;
static constexpr float DOMAIN_H      = 1.0f;
static constexpr float SPACING       = DOMAIN_W / (CELLS - 1);
static constexpr float PAR_RADIUS    = SPACING * 0.3f;

// ——— Display & Simulator ———
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
Flip fluid(
  /*density=*/     1000.0f,
  /*width=*/       DOMAIN_W,
  /*height=*/      DOMAIN_H,
  /*spacing=*/     SPACING,
  /*par_radius=*/  PAR_RADIUS,
  /*maxParticles=*/MAX_PARTICLES
);

// ——— Drawing Helpers ———
int  screenW, screenH, cellW, cellH;
int  cellX[CELLS], cellY[CELLS];
bool prevOcc[CELLS*CELLS], occupied[CELLS*CELLS];

// for timing
unsigned long prevMicros;

// ——— Seed Particles Once ———
void seedParticles() {
  fluid.setParticleCount(MAX_PARTICLES);
  int cols = 5, rows = 20;
  for (int j = 0; j < rows; ++j) {
    for (int i = 0; i < cols; ++i) {
      int idx = j*cols + i;
      fluid.setParticlePos(
        idx,
        SPACING * i,
        SPACING * j
      );
    }
  }
}

// ——— Draw Static Grid (once) ———
void drawStaticGrid() {
  tft.fillScreen(ILI9341_BLACK);
  // vertical lines
  for (int i = 1; i < CELLS; ++i) {
    tft.drawFastVLine(cellX[i], 0, screenH, ILI9341_DARKGREY);
  }
  // horizontal lines
  for (int j = 1; j < CELLS; ++j) {
    // cellY[j] is the *top* of that row
    tft.drawFastHLine(0, cellY[j]+cellH, screenW, ILI9341_DARKGREY);
  }
}

void setup() {
  // ① Max out CPU
  setCpuFrequencyMhz(240);

  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // init display
  tft.begin();
  tft.setRotation(1);  // landscape

  // init sim
  fluid.setup();
  seedParticles();

  // compute grid dimensions
  screenW = tft.width();
  screenH = tft.height();
  cellW   = screenW  / CELLS;
  cellH   = screenH / CELLS;

  // precompute each cell corner
  for (int i = 0; i < CELLS; ++i) {
    cellX[i] = i * cellW;
    // cellY[i] = y‐pixel of bottom of row i minus cellH
    cellY[i] = screenH - (i + 1) * cellH;
  }

  // clear occupancy buffers
  memset(prevOcc, 0, sizeof(prevOcc));
  memset(occupied, 0, sizeof(occupied));

  // ③ draw the grid ONCE
  drawStaticGrid();

  // init timing
  prevMicros = micros();
}

void loop() {
  // —— 1) Timing & single FLIP step ——  
  unsigned long t0 = micros();
  unsigned long now = micros();
  float frameTime = (now - prevMicros) * 1e-6f;
  prevMicros = now;
  // clamp to ~30 FPS max interval
  if (frameTime > 1.0f/30.0f) frameTime = 1.0f/30.0f;

  // read tilt (pot → angle); swap to MPU6050 later
  int pot = analogRead(32);
  float angle = pot * (360.0f/4095.0f) * DEG_TO_RAD;
  float gx = sin(angle) * -9.81f;
  float gy = cos(angle) * -9.81f;

  // **ONE** simulate() call per frame
  fluid.simulate(
    gx, gy, frameTime,
    /*flipRatio=*/     0.8f,
    /*pressureIters=*/ 2,
    /*particleIters=*/ 2,
    /*overRelaxation=*/1.9f,
    /*compensateDrift=*/true
  );
  unsigned long t1 = micros();

  // —— 2) Build occupancy grid ——  
  memset(occupied, 0, sizeof(occupied));
  int cnt = fluid.getParticleCount();
  for (int i = 0; i < cnt; ++i) {
    int ix = fluid.getParticleX(i) * CELLS;
    int iy = fluid.getParticleY(i) * CELLS;
    if (ix >= CELLS) ix = CELLS - 1;
    if (iy >= CELLS) iy = CELLS - 1;
    occupied[ix*CELLS + iy] = true;
  }

  // —— 3) Clear cells that turned OFF ——  
  for (int idx = 0; idx < CELLS*CELLS; ++idx) {
    if (prevOcc[idx] && !occupied[idx]) {
      int ix = idx / CELLS;
      int iy = idx % CELLS;
      tft.fillRect(cellX[ix], cellY[iy], cellW, cellH, ILI9341_BLACK);
    }
  }

  // —— 4) Draw cells that turned ON ——  
  for (int idx = 0; idx < CELLS*CELLS; ++idx) {
    if (!prevOcc[idx] && occupied[idx]) {
      int ix = idx / CELLS;
      int iy = idx % CELLS;
      tft.fillRect(cellX[ix], cellY[iy], cellW, cellH, ILI9341_WHITE);
    }
  }

  // save for next frame
  memcpy(prevOcc, occupied, sizeof(prevOcc));

  // —— 5) Overlay true particle positions ——  
  for (int i = 0; i < cnt; ++i) {
    int sx = fluid.getParticleX(i) * screenW;
    int sy = screenH - fluid.getParticleY(i) * screenH;
    tft.fillCircle(sx, sy, 2, ILI9341_RED);
  }
  unsigned long t2 = micros();

  // —— 6) Redraw 1px grid lines ON TOP ——  
  for (int i = 1; i < CELLS; ++i) {
    tft.drawFastVLine(cellX[i], 0, screenH, ILI9341_DARKGREY);
  }
  for (int j = 1; j < CELLS; ++j) {
    tft.drawFastHLine(0, cellY[j] + cellH, screenW, ILI9341_DARKGREY);
  }

  // —— 7) Print profile once/sec ——  
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 1000) {
    lastLog = millis();
    Serial.printf(
      "Sim: %4uµs  Draw: %4uµs  Total: %4uµs\n",
      (unsigned)(t1 - t0),
      (unsigned)(t2 - t1),
      (unsigned)(t2 - t0)
    );
  }
}



/////////////////// WORKS
// // TFT display pins (change as needed for your wiring)
// #define TFT_CS   15
// #define TFT_DC   2
// #define TFT_RST  4  // or set to -1 and wire to Arduino RST

// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// // ——— Tiny sim parameters ———
// static constexpr int   CELLS         = 20;                   // 3×3 grid
// static constexpr float DOMAIN_W      = 1.0f;
// static constexpr float DOMAIN_H      = 1.0f;
// static constexpr float SPACING       = 1.0f / (CELLS - 1);  // 0.5
// static constexpr float PAR_RADIUS    = SPACING * 0.3f;      // 0.15
// static constexpr int   MAX_PARTICLES = 100;                   // 3 particles

// // global simulator instance
// Flip fluid(
//   /*density=*/     1000.0f,
//   /*width=*/       DOMAIN_W,
//   /*height=*/      DOMAIN_H,
//   /*spacing=*/     SPACING,
//   /*par_radius=*/  PAR_RADIUS,
//   /*maxParticles=*/MAX_PARTICLES
// );



// void seedParticles() {
//   fluid.setParticleCount(MAX_PARTICLES);
//   int cols = 5, rows = 20;
//   for (int j = 0; j < rows; ++j) {
//     for (int i = 0; i < cols; ++i) {
//         int idx = j * cols + i;       // 0..9
//         float px = SPACING * i;       // column i
//         float py = SPACING * j;       // row j
//         fluid.setParticlePos(idx, px, py);
//     }
//   }
  

//   // fluid.setParticleCount(3);
//   // // bottom-left
//   // fluid.setParticlePos(0, 0.0f,       0.0f);
//   // // bottom-middle
//   // fluid.setParticlePos(1, SPACING,    0.0f);
//   // // center
//   // fluid.setParticlePos(2, SPACING,    SPACING);
// }

// void setup() {
//   Serial.begin(115200);
//   setCpuFrequencyMhz(240);

//   while (!Serial) { delay(10); }

//   tft.begin();
//   tft.setRotation(1);            // landscape
//   tft.fillScreen(ILI9341_BLACK);

//   //Serial.println(F("=== Arduino Tiny FLIP 3×3 w/ ILI9341 ==="));
//   fluid.setup();
//   seedParticles();
// }



// const int potPin = 32;



// void loop() {

//   // time
//   static unsigned long prevMicros = micros();
//   static float         acc        = 0.0f;
//   const float          fixedDt    = 1.0f/120.0f;   // your stable dt

//   // 1) measure real‐time passed, cap it so we never get a huge hiccup
//   unsigned long now       = micros();
//   float         frameTime = (now - prevMicros)*1e-6f;
//   prevMicros              = now;
//   frameTime               = min(frameTime, 0.1f);



//   int potValue = analogRead(potPin);
//   // Serial.print("Potentiometer Value: ");
//   // Serial.println(potValue);

//   float angle = map(potValue, 0, 4095, 0, 360) * DEG_TO_RAD; // Convert to radians

//   float gy = cos(angle)*(-9.81f);
//   float gx = sin(angle)*(-9.81f);

//   // Serial.print("gx: ");
//   // Serial.print(gx, 3);
//   // Serial.print(" | gy: ");
//   // Serial.println(gy, 3);


//   // 2) accumulate and sub‐step
//   acc += frameTime;
//   while (acc >= fixedDt) {
//     fluid.simulate(
//       /*gx*/           gx,
//       /*gy*/            gy,
//       /*dt*/            fixedDt,
//       /*flipRatio*/     0.8f,
//       /*pressureIters*/ 2,
//       /*particleIters*/ 2,
//       /*overRelaxation*/1.9f,
//       /*compensateDrift*/true
//     );
//     acc -= fixedDt;
//   }

//   // // 1) advance one step
//   // fluid.simulate(
//   //   /*gx*/ -9.8f, /*gy*/ 0.0f,
//   //   /*dt*/ 1.0f/20.0f,
//   //   /*flipRatio*/ 0.8f,
//   //   /*pressureIters*/ 2,
//   //   /*particleIters*/ 2,
//   //   /*overRelaxation*/ 1.9f,
//   //   /*compensateDrift*/ true
//   // );

//   // 2) fetch grid size
//   int Nx = fluid.getGridWidth();
//   int Ny = fluid.getGridHeight();

//   // 3) draw to serial (optional)
//   // Serial.print(F("Nx: ")); Serial.print(Nx);
//   // Serial.print(F("   Ny: ")); Serial.println(Ny);

//   // 4) draw on TFT
//   uint16_t screenW = tft.width();   // for rotation=1, width=320
//   uint16_t screenH = tft.height();  // height=240
//   float   cellW   = float(screenW)  / float(Nx);
//   float   cellH   = float(screenH) / float(Ny);

//   // clear only the grid area
//   tft.fillRect(0, 0, screenW, screenH, ILI9341_BLACK);

//   // optional: draw grid lines
//   tft.drawFastVLine(0, 0, screenH, ILI9341_DARKGREY);
//   for (int i = 1; i < Nx; ++i) {
//     int x = round(i * cellW);
//     tft.drawFastVLine(x, 0, screenH, ILI9341_DARKGREY);
//   }
//   for (int j = 1; j < Ny; ++j) {
//     int y = round(j * cellH);
//     tft.drawFastHLine(0, y, screenW, ILI9341_DARKGREY);
//   }
//   tft.drawFastHLine(0, screenH-1, screenW, ILI9341_DARKGREY);
//   tft.drawFastVLine(screenW-1,0, screenH, ILI9341_DARKGREY);

//   // occupancy grid
//   bool occupied[CELLS*CELLS] = { false };
//   int cnt = fluid.getParticleCount();
//   for (int i = 0; i < cnt; ++i) {
//     float px = fluid.getParticleX(i);
//     float py = fluid.getParticleY(i);
//     int ix = min(int(px * Nx), Nx - 1);
//     int iy = min(int(py * Ny), Ny - 1);
//     occupied[ix * Ny + iy] = true;
//   }

//   // draw occupied cells
//   for (int ix = 0; ix < Nx; ++ix) {
//     for (int iy = 0; iy < Ny; ++iy) {
//       if (!occupied[ix*Ny + iy]) continue;
//       int x = round(ix * cellW);
//       int y = screenH - round((iy+1) * cellH);
//       int w = ceil(cellW);
//       int h = ceil(cellH);
//       tft.fillRect(x, y, w, h, ILI9341_WHITE);
//     }
//   }

//   // overlay true particle positions
//   for (int i = 0; i < cnt; ++i) {
//     float simX = fluid.getParticleX(i);
//     float simY = fluid.getParticleY(i);
//     int screenX = round(simX * screenW);
//     int screenY = screenH - round(simY * screenH);
//     tft.fillCircle(screenX, screenY, 4, ILI9341_RED);
//   }

//   //delay(200);
// }


// // ——— Tiny sim parameters ———
// static constexpr int   CELLS        = 3;                   // 3×3 grid
// static constexpr float DOMAIN_W     = 1.0f;
// static constexpr float DOMAIN_H     = 1.0f;
// static constexpr float SPACING      = 1.0f / (CELLS - 1);  // 0.5
// static constexpr float PAR_RADIUS   = SPACING * 0.3f;      // 0.15
// static constexpr int   MAX_PARTICLES= 3;                   // 3 particles

// // global simulator instance
// Flip fluid(
//   /*density=*/    1000.0f,
//   /*width=*/      DOMAIN_W,
//   /*height=*/     DOMAIN_H,
//   /*spacing=*/    SPACING,
//   /*par_radius=*/ PAR_RADIUS,
//   /*maxParticles=*/MAX_PARTICLES
// );

// void seedParticles() {
//   fluid.setParticleCount(3);
//   // bottom-left
//   fluid.setParticlePos(0, 0.0f,         0.0f);
//   // bottom-middle
//   fluid.setParticlePos(1, SPACING,      0.0f);
//   // center
//   fluid.setParticlePos(2, SPACING,      SPACING);
// }

// void setup() {
//   Serial.begin(115200);
//   while (!Serial) { delay(5); }
//   Serial.println(F("=== Arduino Tiny FLIP 3×3, 3 particles ==="));
//   //fluid.setup();
//   seedParticles();
// }

// void loop() {
//   // 1) advance one step
//   fluid.simulate(
//     /*gx*/ -9.8f, /*gy*/ 0.0f,
//     /*dt*/ 1.0f/120.0f,
//     /*flipRatio*/ 0.8f,
//     /*pressureIters*/ 2,
//     /*particleIters*/ 2,
//     /*overRelaxation*/ 1.9f,
//     /*compensateDrift*/ true
//   );

//   // 2) fetch grid size
//   int Nx = fluid.getGridWidth();   // should be 3
//   int Ny = fluid.getGridHeight();  // should be 3
//   Serial.print(F("Nx: ")); Serial.print(Nx);
//   Serial.print(F("   Ny: ")); Serial.println(Ny);

//   // 3) print each particle’s continuous pos + cell index
//   int cnt = fluid.getParticleCount();  // =3
//   for (int i = 0; i < cnt; ++i) {
//     float px = fluid.getParticleX(i);
//     float py = fluid.getParticleY(i);
//     Serial.printf("fluid.getParticleX(i):  %f  fluid.getParticleY(i):  %f \n", fluid.getParticleX(i), fluid.getParticleY(i));
    
//     int ix = min(int(px * Nx), Nx - 1);
//     int iy = min(int(py * Ny), Ny - 1);

//     Serial.print(F("PX: ")); Serial.print(px, 4);
//     Serial.print(F("   PY: ")); Serial.print(py, 4);
//     Serial.print(F("   -> ix: ")); Serial.print(ix);
//     Serial.print(F("   iy: ")); Serial.println(iy);
//   }
//   Serial.println();


  

//   delay(200);
// }


// // ——— Tweaked simulator parameters ———
// const float density       = 1000.0f;
// const float domainW       = 1.0f, domainH = 1.0f;

// // Use 1/8 spacing instead of 1/16
// const float gridSpacing   = 1.0f / 8.0f;
// const float parRadius     = gridSpacing * 0.3f;

// // Only 16 particles: 2 rows × 8 columns
// const int   maxParticles  = 16;

// // Instantiate with small sizes
// Flip fluid(
//   density,
//   domainW, domainH,
//   gridSpacing,
//   parRadius,
//   maxParticles
// );

// void seedParticles() {
//   float r   = parRadius;
//   float dx  = 2.0f * r;
//   float dy  = 2.0f * r;
//   int   numX = min(maxParticles/2, int((1.0f - 2*(gridSpacing + r)) / dx));
//   int   rows = 2;
//   int   total = rows * numX;

//   fluid.setParticleCount(total);
//   for (int row = 0; row < rows; ++row) {
//     for (int i = 0; i < numX; ++i) {
//       int idx = row * numX + i;
//       float px = gridSpacing + r + dx * i;
//       float py = gridSpacing + r + dy * row;
//       fluid.setParticlePos(idx, px, py);
//     }
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   while (!Serial) { delay(1); }

//   // zero‐init all your PSRAM mallocs or vectors in Flip ctor (if you switched to malloc)
//   //fluid.setup();
//   seedParticles();

//   Serial.println("=== Tiny FLIP on stock ESP32-WROOM (no PSRAM) ===");
// }

// void loop() {
//   fluid.simulate(
//     -9.8f, 0.0f,       // gravity
//      1.0f/120.0f,      // dt
//      0.8f,             // flipRatio
//      2,                // pressure iters
//      2,                // particle‐separation iters
//      1.9f,             // over‐relaxation
//      true              // compensate drift
//   );

//   int Nx = fluid.getGridWidth();   // should now be 9
//   int Ny = fluid.getGridHeight();  // should now be 9
//   Serial.printf("Nx: %d   Ny: %d\n", Nx, Ny);

//   int cnt = fluid.getParticleCount();
//   for (int i = 0; i < cnt; ++i) {
//     float px = fluid.getParticleX(i), py = fluid.getParticleY(i);
//     int ix = min(int(px * Nx), Nx-1);
//     int iy = min(int(py * Ny), Ny-1);
//     Serial.printf("x: %d   y: %d\n", ix, iy);
//   }
//   delay(150);
// }








////idkkkkk
// #include <Arduino.h>
// #include <SPI.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_ILI9341.h>
// #include <vector>       // for std::vector
// #include <algorithm>    // for std::min
// #include <cmath>        // for sqrtf()

// #include "Flip.h"         // Include your own class header


// // TFT wiring
// #define TFT_DC    2
// #define TFT_CS   15
// #define TFT_RST   4
// #define DOT_SIZE  4
// #define DOT_COLOR ILI9341_WHITE

// //Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);



// // // //-------------------------------------------------------------------------------
// // // // Globals and setup
// // // //-------------------------------------------------------------------------------


// //Flip::Flip(float density, int width, int height, float spacing, int maxParticles) 
// //Flip fluid(DENSITY, WIDTH, HEIGHT, SPACING, PAR_RADIUS, MAX_PAR);

// Flip fluid(
//         1000.0f,   // density
//         1, 1,      // domain size
//         1.0f/16.0f,// grid spacing
//         (1.0f/16.0f)*0.3f,// particle radius
//         50
//     );

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(ILI9341_BLACK);

//   // Seed 20 particles across the top
//   int N = 20;
//   fluid.setParticleCount(N);
//   for (int i = 0; i < N; ++i) {
//     float x = 0.1f + 0.8f * (i / float(N-1));  // 0.1 → 0.9
//     float y = 0.9f;
//     fluid.setParticlePos(i, x, y);
//   }
// }

// //-------------------------------------------------------------------------------
// // Helpers and loop
// //-------------------------------------------------------------------------------
// //–– at the top, define your grid resolution
// #define GRID_W 24
// #define GRID_H 24

// //–– replace drawDot:
// void drawDot(float sx, float sy) {
//   int cellW = tft.width()  / GRID_W;
//   int cellH = tft.height() / GRID_H;
//   int cx = int(sx * GRID_W);
//   int cy = int((1.0f - sy) * GRID_H);
//   // clamp just in case
//   cx = constrain(cx, 0, GRID_W - 1);
//   cy = constrain(cy, 0, GRID_H - 1);
//   tft.fillRect(cx * cellW,
//                cy * cellH,
//                cellW,
//                cellH,
//                DOT_COLOR);
// }

// void loop() {
//   // 1) simulate gravity-only
//   fluid.simulate(
//       -9.8f, 0.0f,         // gravity
//       1.0f/120.0f,         // dt
//       0.8f,                // flipRatio
//       2,                   // pressure iters
//       2,                   // particle‐separation iters
//       1.9f,                // over-relaxation
//       true                 // compensate drift
//   );

//   // 2) clear
//   tft.fillScreen(ILI9341_BLACK);

//   // 3) draw
//   int cnt = fluid.getParticleCount();
//   for (int i = 0; i < cnt; ++i) {
//     drawDot(fluid.getParticleX(i), fluid.getParticleY(i));
//   }

//   // 4) frame delay
//   delay(5);
// }


// const float DENSITY = 1000.0;
// const int WIDTH = 20;
// const int HEIGHT = 20;
// const float SPACING = 0.01;
// const float PAR_RADIUS = SPACING * 0.3;
// const int MAX_PAR = 50;
// const float SIM_W = 1.0f;
// const float SIM_H = 1.0f;

// // how many grid‐cells across/down to draw
// #define GRID_W 17
// #define GRID_H 17

// // –– Our FLIP solver instance
// static const int NMAX = 50;

// Flip fluid(
//   1000.0f,       // density
//   1, 1,          // domain size (normalized 0…1)
//   1.0f/16.0f,    // cell spacing
//   (1.0f/16.0f)*0.3f,  // particle radius
//   NMAX
// );

// // simple helper to draw one “pixel”–cell on the TFT
// void drawDot(float sx, float sy) {
//   int cellW = tft.width()  / GRID_W;
//   int cellH = tft.height() / GRID_H;
//   int cx = int(sx * GRID_W);
//   int cy = int((1.0f - sy) * GRID_H);
//   // keep in bounds
//   cx = constrain(cx, 0, GRID_W - 1);
//   cy = constrain(cy, 0, GRID_H - 1);
//   tft.fillRect(cx*cellW, cy*cellH, cellW, cellH, DOT_COLOR);
// }

// void setup() {
//   Serial.begin(115200);
//   tft.begin();
//   tft.setRotation(1);
//   tft.fillScreen(ILI9341_BLACK);

//   // // 1) mark solid walls in the grid
//   // fluid.setup();
  

//   // 2) seed one row of particles
//   //    same pattern as on PC: from x=spacing+r … x=1−(spacing+r)
//   // float spacing = 1.0f/16.0f;
//   // float r       = spacing*0.3f;
//   // float dx      = 2.0f*r;
//   // int   numX    = min(NMAX, int((1.0f - 2*(spacing+r)) / dx));
//   // fluid.setParticleCount(numX);
//   // for (int i = 0; i < numX; ++i) {
//   //   float px = spacing + r + dx*i;
//   //   float py = spacing + r;
//   //   fluid.setParticlePos(i, px, py);
//   // }
//     float spacing = 1.0f/16.0f;   // same as your grid spacing
//     float r       = spacing * 0.3f;   // particle radius
//     float dx      = 2.0f * r;         // horizontal step
//     float dy      = 2.0f * r;         // vertical step between rows

//     int numX    = std::min(NMAX,int((1.0f - 2*(spacing + r)) / dx));
//     int numRows = 2;
//     int total   = numRows * numX;

//     fluid.setParticleCount(total);

//     for (int row = 0; row < numRows; ++row) {
//         for (int i = 0; i < numX; ++i) {
//             int idx  = row * numX + i;
//             float px = spacing + r + dx * i;
//             float py = spacing + r + dy * row;
//             fluid.setParticlePos(idx, px, py);
//         }
//     }
// }

// void loop() {
//   // 1) step the FLIP sim
//   fluid.simulate(
//     -9.8f, 0.0f,         // gravity (x,y)
//     1.0f/120.0f,         // dt
//     0.8f,                // flip ratio
//     2,                   // pressure‐solve iterations
//     2,                   // particle‐separation iterations
//     1.9f,                // over‐relaxation
//     true                 // compensate drift
//   );

//   // 2) clear screen
//   tft.fillScreen(ILI9341_BLACK);

//   // 1) grid dimensions
  
//   int Nx = fluid.getGridWidth();
//   int Ny = fluid.getGridHeight();
  


//   int cnt = fluid.getParticleCount();
//   for (int i = 0; i < cnt; ++i) {
//       float x = fluid.getParticleX(i);
//       float y = fluid.getParticleY(i);
//       int ix = std::min(int(x * Nx), Nx - 1);
//       int iy = std::min(int(y * Ny), Ny - 1);

//       Serial.print("x: ");
//       Serial.print(ix);
//       Serial.print("   y: ");
//       Serial.println(iy);
//       //std::cout << "x: " << ix << "   y: " << iy <<"\n";

//       //occupied[ix * Ny + iy] = true;
//   }
  
//   // // 3) draw each particle as a tiny cell
//   // int cnt = fluid.getParticleCount();
//   // for (int i = 0; i < cnt; ++i) {
//   //   drawDot(fluid.getParticleX(i), fluid.getParticleY(i));
//   // }

//   // 4) pace yourself
//   delay(5);
// }

// // // Set number of particles
// // void Flip::setParticleCount(int n) {
// //     p_num = std::min(n, maxParticles_);
// // }

// // // Set position of particle i
// // void Flip::setParticlePos(int i, float x, float y) {
// //     if (i < 0 || i >= p_num) return;
// //     p_pos[2 * i] = x;
// //     p_pos[2 * i + 1] = y;
// // }

// // // Get number of active particles
// // int Flip::getParticleCount() const {
// //     return p_num;
// // }

// // // Get X position of particle i
// // float Flip::getParticleX(int i) const {
// //     return p_pos[2 * i];
// // }

// // // Get Y position of particle i
// // float Flip::getParticleY(int i) const {
// //     return p_pos[2 * i + 1];
// // }

// // float Flip::clamp(float x, float min, float max) {
// //     if (x < min)
// //         return min;
// //     else if (x > max)
// //         return max;
// //     else 
// //         return x;
// // }

// // // Private method: Apply gravity to velocities
// // void Flip::gravity(float g_x, float g_y, float dt) {
// //     for (int i = 0; i < p_num; i++) {
// //         p_vel[2 * i]     += g_x * dt;
// //         p_vel[2 * i + 1] += g_y * dt;
// //         p_pos[2*i] += p_vel[2*i] * dt;
// //         p_pos[2*i+1] += p_vel[2*i+1] * dt;
// //     }
// // }

// // void Flip::particleCollision() {
// //     int left = cell_size + par_radius_;
// //     int right = (x_cells - 1) * cell_size - par_radius_;
// //     int bottom = cell_size + par_radius_;
// //     int top = (y_cells - 1) * cell_size - par_radius_;

// //     for (int i = 0; i < p_num; i++) {
// //         int x = p_pos[2*i];
// //         int y = p_pos[2*i+1];

// //         if (x < left) {
// //             p_pos[2*i] = left;
// //             p_vel[2*i] = 0.0;
// //         }
// //         if (x > right) {
// //             p_pos[2*i] = right;
// //             p_vel[2*i] = 0.0;
// //         }
// //         if (x < bottom) {
// //             p_pos[2*i+1] = bottom;
// //             p_vel[2*i+1] = 0.0;
// //         }
// //         if (x > top) {
// //             p_pos[2*i+1] = top;
// //             p_vel[2*i+1] = 0.0;
// //         }
// //     }
// // }

// // void Flip::particle_grid(int iters) {
// //     std::fill(per_cell.begin(), per_cell.end(), 0);

// //     // fill per_cell list by counting the patcile in each cell
// //     for (int i = 0; i < p_num; i++) {
// //       int x = p_pos[2*i] / par_spacing;
// //       int y = p_pos[2*i+1] / par_spacing;

// //       int idx = clamp(std::floor(x), 0, p_x-1);
// //       int idy = clamp(std::floor(y), 0, p_y-1);
// //       int id = idx*p_y + idy;

// //       per_cell[id]++;
      
// //     }

// //     // fill first_cell, by summing them by to find the index in cellpar_id
// //     int sum = 0;
// //     for (int i = 0; i < pcell_num; i++) {
// //       sum += per_cell[i];
// //       first_cell[i] = sum;
// //     }
// //     first_cell[pcell_num] = sum;


// //     /*
// //     like in cell 0 say it contain particles 123, so in the array it'll be par_id[1, 2, 3] all equal to 0
// //     and the firstcellParticle is just used to say the first cell contain 3 particles and the next cell will start at index 3 */
// //     // fill the cellpar_id array
// //     for (int i = 0; i < p_num; i++) {
// //       // get which cell the partcile is located on
// //       int x = p_pos[2*i] / par_spacing;
// //       int y = p_pos[2*i+1] / par_spacing;
      
// //       int idx = clamp(std::floor(x), 0, p_x-1);
// //       int idy = clamp(std::floor(y), 0, p_y-1);
// //       // find the first_cell id for this particle
// //       int id = idx*p_y + idy;
      
// //       first_cell[id]--;
// //       cellpar_id[first_cell[id]] = i; // sotring the partcile into that cell
// //     }


// //     /// seperating the particles
// //     const float min_dist = 2.0f * par_radius_;
// //     for (int it = 0; it < iters; ++it) {
// //         for (int i = 0; i < p_num; ++i) {
// //             float x = p_pos[2*i];
// //             float y = p_pos[2*i + 1];

// //             int idx = clamp(int(std::floor(x / par_spacing)), 0, p_x - 1);
// //             int idy = clamp(int(std::floor(y / par_spacing)), 0, p_y - 1);

// //             int left   = std::max(0, idx - 1);
// //             int right  = std::min(idx + 1, p_x - 1);
// //             int bottom = std::max(0, idy - 1);
// //             int top    = std::min(idy + 1, p_y - 1);

// //             for (int cx = left; cx <= right; ++cx) {
// //                 for (int cy = bottom; cy <= top; ++cy) {
// //                     int cell_id = cx * p_y + cy;
// //                     int start   = first_cell[cell_id];
// //                     int end     = first_cell[cell_id + 1];

// //                     for (int k = start; k < end; ++k) {
// //                         int id = cellpar_id[k];
// //                         if (id == i) continue;

// //                         float px = p_pos[2*id];
// //                         float py = p_pos[2*id + 1];
// //                         float dx = px - x;
// //                         float dy = py - y;
// //                         float dist = std::sqrt(dx*dx + dy*dy);

// //                         if (dist == 0.0f || dist > min_dist) 
// //                             continue;

// //                         float s = 0.5f * (min_dist - dist) / dist;
// //                         dx *= s; 
// //                         dy *= s;

// //                         // push them apart
// //                         p_pos[2*i]       -= dx;
// //                         p_pos[2*i + 1]   -= dy;
// //                         p_pos[2*id]      += dx;
// //                         p_pos[2*id + 1]  += dy;
// //                     }
// //                 }
// //             }
// //         }
// //     }
// // }


// // // Public method: Simulate one step
// // void Flip::simulate(float g_x, float g_y, float dt, int par_iters) {
// //     gravity(g_x, g_y, dt);
// //     particle_grid(par_iters);
// //     particleCollision();

// // }


// // //-------------------------------------------------------------------------------
// // // Minimal Flip class with a separate gravity() method
// // //-------------------------------------------------------------------------------
// // class Flip {
// // public:
// //   Flip(int maxParticles)
// //     : maxParticles_(maxParticles),
// //       p_num_(0),
// //       p_pos_(2*maxParticles_, 0.0f),
// //       p_vel_(2*maxParticles_, 0.0f)
// //   {}

// //   // Run one step: apply gravity then move particles
// //   void simulate(float g_x, float g_y, float dt) {
// //     gravity(g_x, g_y, dt);
// //   }

// //   // Set and get
// //   void setParticleCount(int n) { 
// //     p_num_ = min(n, maxParticles_); 
// //   }
// //   void setParticlePos(int i, float x, float y) {
// //     if (i<0||i>=p_num_) return;
// //     p_pos_[2*i]   = x;
// //     p_pos_[2*i+1] = y;
// //   }
// //   int getParticleCount() const { 
// //     return p_num_; 
// //   }
// //   float getParticleX(int i) const { 
// //     return p_pos_[2*i]; 
// //   }
// //   float getParticleY(int i) const { 
// //     return p_pos_[2*i+1]; 
// //   }

// // private:
// //   int maxParticles_, p_num_;
// //   std::vector<float> p_pos_, p_vel_;

// //   // 1) apply gravity to velocities
// //   void gravity(float g_x, float g_y, float dt) {
// //     for (int i = 0; i < p_num_; ++i) {
// //       p_vel_[2*i] += g_x * dt;
// //       p_vel_[2*i+1] += g_y * dt;
// //       p_pos_[2*i] += p_vel_[2*i] * dt;
// //       p_pos_[2*i+1] += p_vel_[2*i+1] * dt;
      
// //     }
// //   }


// //   // // particle collision with wall
// //   // void particleCollision() {
// //   //   int left = 
// //   // }

  
// // };





// // #include <SPI.h>
// // #include <Adafruit_GFX.h>
// // #include <Adafruit_ILI9341.h>


// // #define BTN_PIN 5
// // #define TFT_DC 2
// // #define TFT_CS 15
// // #define TFT_RST  4   // or tie to EN/RESET if you like
// // #define DOT_COLOR ILI9341_WHITE
// // #define DOT_SIZE 8


// // Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// // void setup() {
// //   Serial.begin(115200);

// //   // Initialize the display
// //   tft.begin();
// //   tft.setRotation(1);            // landscape
// //   tft.fillScreen(ILI9341_BLACK);
// //   tft.setTextColor(ILI9341_WHITE);
// //   tft.setTextSize(2);
// // }

// // void print_point(int x, int y) {
// //   int sx = x * DOT_SIZE;
// //   int sy = y * DOT_SIZE;
// //   tft.fillRect(sx, sy, DOT_SIZE, DOT_SIZE, DOT_COLOR);
// // }

// // void loop() {
// //   // Just print a counter so you can see updates
// //   tft.fillRect(0, 0, 240, DOT_SIZE, ILI9341_BLACK);

// //   for (int i = 0; i < 20; i++) {
// //     for (int j = 1; j < 20; j++) {
// //       print_point(i, j);
// //     }
// //   }

// //   // print_point(0, 0);
// //   // print_point(2, 3);
// //   // tft.print("..........................................................................................................................");
// //   // tft.println(counter++);
// //   // delay(500);
// // }

// // // #include <Arduino.h>

// // // // put function declarations here:
// // // int myFunction(int, int);

// // // void setup() {
// // //   // put your setup code here, to run once:
// // //   int result = myFunction(2, 3);
// // // }

// // // void loop() {
// // //   // put your main code here, to run repeatedly:
// // // }

// // // // put function definitions here:
// // // int myFunction(int x, int y) {
// // //   return x + y;
// // // }