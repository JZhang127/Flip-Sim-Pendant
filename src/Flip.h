#ifndef FLIP_H
#define FLIP_H

#include <vector>
#include <algorithm>  // for std::clamp
#include <cmath>      // for std::floor, std::sqrt

// Cell‐type constants:
static constexpr int FLUID_CELL = 0;
static constexpr int AIR_CELL   = 1;
static constexpr int SOLID_CELL = 2;

class Flip {
public:
    Flip(float density,
         int width,
         int height,
         float spacing,
         float par_radius,
         int maxParticles);

    void setParticleCount(int n);
    void setParticlePos(int i, float x, float y);
    int   getParticleCount() const;
    float getParticleX(int i) const;
    float getParticleY(int i) const;

    void setup();
    void simulate(float gx, float gy, float dt,
                  float flipRatio,
                  int numPressureIters,
                  int numParticleIters,
                  float overRelaxation,
                  bool compensateDrift);

    int getGridWidth() const;
    int getGridHeight() const;

private:
    void gravity(float gx, float gy, float dt);
    void particleCollision();
    void particle_grid(int iters);
    void velocity_grid(bool toGrid, float flipRatio);
    void solveIncompressibility(int numIters, float dt,
                                float overRelaxation,
                                bool compensateDrift);
    float clamp(float v, float lo, float hi);

    
    

    // MAC grid
    float               density_;
    int                 x_cells_, y_cells_, cell_num_;
    float               cell_size_;

    // Particles
    int                 maxParticles_;
    int                 p_num_;
    std::vector<float>  p_pos_;
    std::vector<float>  p_vel_;

    // Spacing
    float               par_radius_;
    float               par_spacing_;

    // Bucket‐grid
    int                 p_x, p_y, pcell_num;
    std::vector<int>    per_cell;
    std::vector<int>    first_cell;
    std::vector<int>    cellpar_id;

    // MAC‐grid arrays
    std::vector<float>  x, y, dx, dy, prev_x, prev_y, solid;
    std::vector<int>    cell_type;

    // Density
    std::vector<float>  p_density;
    float               rest_density;

    std::vector<float> p; 
};

#endif // FLIP_H
