#include "Flip.h"
#include <iostream>
#include <cmath>
#include <Arduino.h>

Flip::Flip(float density,
           int width,
           int height,
           float spacing,
           float par_radius,
           int maxParticles)
  : density_(density),
    x_cells_(int(std::floor(width  / spacing)) + 1),
    y_cells_(int(std::floor(height / spacing)) + 1),
    cell_num_(x_cells_ * y_cells_),
    cell_size_(std::max(width/(float)x_cells_, height/(float)y_cells_)),

    maxParticles_(maxParticles),
    p_num_(0),
    p_pos_(2*maxParticles_, 0.0f),
    p_vel_(2*maxParticles_, 0.0f),

    par_radius_(par_radius),
    // par_spacing_ must wait until after par_radius_ is set:
    par_spacing_(2.2f * par_radius_),

    // we'll size the bucket‐vectors below in the constructor body:
    p_x(0), p_y(0), pcell_num(0),

    // MAC‐grid vectors—we know their size from cell_num_:
    x(cell_num_, 0.0f),
    y(cell_num_, 0.0f),
    dx(cell_num_, 0.0f),
    dy(cell_num_, 0.0f),
    prev_x(cell_num_, 0.0f),
    prev_y(cell_num_, 0.0f),
    solid(cell_num_, 1.0f),
    cell_type(cell_num_, AIR_CELL),
    p_density(cell_num_, 0.0f),
    rest_density(0.0f),
    p(cell_num_, 0.0f)          // << add this
{
    // ── now that par_spacing_ is valid, build our particle‐bucket (hash) grid ──
    p_x       = int(std::floor(width  / par_spacing_)) + 1;
    p_y       = int(std::floor(height / par_spacing_)) + 1;
    pcell_num = p_x * p_y;

    per_cell   .assign(pcell_num,      0);
    first_cell .assign(pcell_num + 1,  0);
    cellpar_id .assign(maxParticles_,   0);

    // mark solid boundaries (walls) in the MAC grid:
    for (int i = 0; i < x_cells_; ++i) {
        for (int j = 0; j < y_cells_; ++j) {
            int idx = i * y_cells_ + j;
            if (i == 0 || i == x_cells_-1 || j == 0 || j == y_cells_-1) {
                solid[idx]    = 0.0f;
                cell_type[idx] = SOLID_CELL;
            }
        }
    }
}


float Flip::clamp(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void Flip::setParticleCount(int n) 
{ 
    p_num_ = std::min(n, maxParticles_); 
}

void Flip::setParticlePos(int i, float px, float py) {
    if (i < 0 || i >= p_num_) return;
    p_pos_[2*i  ] = px;
    p_pos_[2*i+1] = py;
}

int   Flip::getParticleCount() const { return p_num_; }
float Flip::getParticleX(int i) const { return p_pos_[2*i]; }
float Flip::getParticleY(int i) const { return p_pos_[2*i+1]; }

int Flip::getGridWidth() const {return x_cells_;}
int Flip::getGridHeight() const {return y_cells_;}


// gravity + simple Euler‐push
void Flip::gravity(float gx, float gy, float dt) {
    for (int i = 0; i < p_num_; ++i) {
        p_vel_[2*i  ] += gx * dt;
        p_vel_[2*i+1] += gy * dt;
        p_pos_[2*i  ] += p_vel_[2*i  ] * dt;
        p_pos_[2*i+1] += p_vel_[2*i+1] * dt;
    }
}

// bounce off the domain walls
void Flip::particleCollision() {
    float minX = par_radius_, maxX = 1.0f - par_radius_;
    float minY = par_radius_, maxY = 1.0f - par_radius_;

    for (int i = 0; i < p_num_; ++i) {
        float &px = p_pos_[2*i], &py = p_pos_[2*i+1];
        float &vx = p_vel_[2*i], &vy = p_vel_[2*i+1];
        if (px < minX) { px = minX; vx = 0; }
        if (px > maxX) { px = maxX; vx = 0; }
        if (py < minY) { py = minY; vy = 0; }
        if (py > maxY) { py = maxY; vy = 0; }
    }
}

// fill per_cell, prefix‐sum into first_cell, then flatten into cellpar_id
void Flip::particle_grid(int iters) {
    std::fill(per_cell.begin(), per_cell.end(), 0);

    for (int i = 0; i < p_num_; ++i) {
        float fx = p_pos_[2*i]   / par_spacing_;
        float fy = p_pos_[2*i+1] / par_spacing_;
        int   ix = clamp(int(std::floor(fx)), 0, p_x-1);
        int   iy = clamp(int(std::floor(fy)), 0, p_y-1);
        per_cell[ix * p_y + iy]++;
    }

    int sum = 0;
    for (int c = 0; c < pcell_num; ++c) {
        sum += per_cell[c];
        first_cell[c] = sum;
        
    }
    first_cell[pcell_num] = sum;

    for (int i = 0; i < p_num_; ++i) {
        float fx = p_pos_[2*i]   / par_spacing_;
        float fy = p_pos_[2*i+1] / par_spacing_;
        int   ix = clamp(int(std::floor(fx)), 0, p_x-1);
        int   iy = clamp(int(std::floor(fy)), 0, p_y-1);
        int   idx = ix * p_y + iy;
        first_cell[idx]--;
        cellpar_id[first_cell[idx]] = i;
    }
    

    // simple particle‐separation pass (repel close neighbors)
    float minDist = 2.0f * par_radius_;
    for (int iter = 0; iter < iters; ++iter) {
        for (int i = 0; i < p_num_; ++i) {
            float px = p_pos_[2*i], py = p_pos_[2*i+1];
            int ix = clamp(int(std::floor(px / par_spacing_)), 0, p_x-1);
            int iy = clamp(int(std::floor(py / par_spacing_)), 0, p_y-1);
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int cx = clamp(ix+dx, 0, p_x-1);
                    int cy = clamp(iy+dy, 0, p_y-1);
                    int start = first_cell[cx*p_y + cy];
                    int end   = first_cell[cx*p_y + cy + 1];
                    for (int k = start; k < end; ++k) {
                        int j = cellpar_id[k];
                        if (j == i) continue;
                        float qx = p_pos_[2*j], qy = p_pos_[2*j+1];
                        float dx = qx - px, dy = qy - py;
                        float dist = std::sqrt(dx*dx + dy*dy);
                        if (dist == 0 || dist > minDist) continue;
                        float s = 0.5f*(minDist - dist)/dist;
                        p_pos_[2*i]   -= dx*s;
                        p_pos_[2*i+1] -= dy*s;
                        p_pos_[2*j]   += dx*s;
                        p_pos_[2*j+1] += dy*s;
                    }
                }
            }
        }
    }
}

void Flip::velocity_grid(bool toGrid, float flipRatio) {
    int nX = x_cells_, nY = y_cells_, N = cell_num_;
    float h = cell_size_, h2 = 0.5f*h;


    if (toGrid) {
        prev_x = x;  prev_y = y;
        std::fill(x.begin(), x.end(), 0.0f);
        std::fill(y.begin(), y.end(), 0.0f);
        std::fill(dx.begin(), dx.end(), 0.0f);
        std::fill(dy.begin(), dy.end(), 0.0f);

        // mark solid vs air
        for (int c = 0; c < N; ++c)
            cell_type[c] = (solid[c] == 0.0f ? SOLID_CELL : AIR_CELL);
        // any cell with a particle → FLUID_CELL
        for (int p = 0; p < p_num_; ++p) {
            float px = p_pos_[2*p], py = p_pos_[2*p+1];
            int i = clamp(int(px/h), 0, x_cells_-1);
            int j = clamp(int(py/h), 0, y_cells_-1);
            int c = i*nY + j;
            if (cell_type[c] == AIR_CELL) cell_type[c] = FLUID_CELL;
        }
    }

    for (int comp = 0; comp < 2; ++comp) {
        float offX = comp==0 ? 0.0f : h2;
        float offY = comp==0 ? h2  : 0.0f;
        auto &gridV = (comp==0 ? x : y);
        auto &prevV = (comp==0 ? prev_x : prev_y);
        auto &weight = (comp==0 ? dx : dy);

        for (int p = 0; p < p_num_; ++p) {
            ///std::cout << "!!!!!" << p_pos_[2*p]  << "     !!!!!" << p_pos_[2*p+1] << "\n";
            float px = clamp(p_pos_[2*p]   , h, (x_cells_-1)*h);
            float py = clamp(p_pos_[2*p+1] , h, (y_cells_-1)*h);
            /// std::cout << "!!!! h: " << h << "    !!!!! x: " << (x_cells_-1)*h << "     !!!!! y: " << (x_cells_-1)*h <<"\n";
            /// std::cout << "px: " << px << "    py: " << py << "\n";
            float fx = (px - offX)/h, fy = (py - offY)/h;

            /// std::cout << fx << " " << fy << "\n";
            /// std::cout << px << " " << offX << "\n";
            /// std::cout << py << " " << offY << "\n";

            int i0 = std::min(int(std::floor(fx)), x_cells_-2),
                j0 = std::min(int(std::floor(fy)), y_cells_-2);

            float tx = fx - i0, ty = fy - j0;
            int i1 = i0+1, j1 = j0+1;
            float w00 = (1-tx)*(1-ty), w10 = tx*(1-ty),
                  w11 = tx*ty,       w01 = (1-tx)*ty;
            int c00 = i0*nY + j0, c10 = i1*nY + j0,
                c11 = i1*nY + j1, c01 = i0*nY + j1;

            

            int stride = (comp == 0 ? nY : 1);

           /// std::cout << i0 << " " << nY << " " << j0 << " " << stride << "\n";

///             if (c00 < stride || c00 - stride >= cell_num_) {
///                 Serial.printf("OOB neighbor: c00=%d stride=%d\n", c00, stride);
/// }


            if (toGrid) {
                float pv = p_vel_[2*p+comp];
                gridV[c00] += pv*w00; weight[c00] += w00;
                gridV[c10] += pv*w10; weight[c10] += w10;
                gridV[c11] += pv*w11; weight[c11] += w11;
                gridV[c01] += pv*w01; weight[c01] += w01;
            } else {
                int stride = comp==0 ? nY : 1;
                float v00 = (cell_type[c00]!=AIR_CELL||cell_type[c00-stride]!=AIR_CELL)?w00:0;
                float v10 = (cell_type[c10]!=AIR_CELL||cell_type[c10-stride]!=AIR_CELL)?w10:0;
                float v11 = (cell_type[c11]!=AIR_CELL||cell_type[c11-stride]!=AIR_CELL)?w11:0;
                float v01 = (cell_type[c01]!=AIR_CELL||cell_type[c01-stride]!=AIR_CELL)?w01:0;
                float sum = v00+v10+v11+v01;

                if (sum<=0) continue;
                float oldV = p_vel_[2*p+comp];
                float picV = (v00*gridV[c00] + v10*gridV[c10]
                            + v11*gridV[c11] + v01*gridV[c01]) / sum;
                float corr = ( v00*(gridV[c00]-prevV[c00])
                            + v10*(gridV[c10]-prevV[c10])
                            + v11*(gridV[c11]-prevV[c11])
                            + v01*(gridV[c01]-prevV[c01]) ) / sum;
                p_vel_[2*p+comp] = (1-flipRatio)*picV + flipRatio*(oldV + corr);
                // std::cout << flipRatio << "\n";
                // std::cout << picV << "\n";
                // std::cout << oldV << "\n";
                // std::cout << corr << "\n";
                // std::cout << (1-flipRatio)*picV + flipRatio*(oldV + corr) << "\n";


            }
            ///std::cout << "\n";
        }

        if (toGrid) {
            // normalize
            for (int c = 0; c < N; ++c)
                if (weight[c]>0) gridV[c] /= weight[c];
            // enforce no-slip
            for (int i = 0; i < x_cells_; ++i)
            for (int j = 0; j < y_cells_; ++j) {
                int c = i*nY + j;
                bool solidCell = cell_type[c]==SOLID_CELL;
                if (solidCell || (i>0 && cell_type[c-nY]==SOLID_CELL)) x[c]=prev_x[c];
                if (solidCell || (j>0 && cell_type[c-1]==SOLID_CELL)) y[c]=prev_y[c];
            }
        }
    }
}


void Flip::solveIncompressibility(int numPressureIters,
                                  float dt,
                                  float overRelaxation,
                                  bool compensateDrift) {
    int nY = y_cells_;
    int nX = x_cells_;
    int N = cell_num_;
    
    // 1. Clear pressure and save old velocities
    std::fill(p.begin(), p.end(), 0.0f);
    prev_x = x;
    prev_y = y;

    // 2. Pressure scale factor (matches JavaScript cp)
    float cp = density_ * cell_size_ / dt;

    // 3. Gauss–Seidel iterations
    for (int iter = 0; iter < numPressureIters; ++iter) {
        for (int i = 1; i < nX - 1; ++i) {
            for (int j = 1; j < nY - 1; ++j) {
                int c = i * nY + j;
                if (cell_type[c] != FLUID_CELL) continue;

                // Neighbor indices
                int left  = (i - 1) * nY + j;
                int right = (i + 1) * nY + j;
                int down  = i * nY + (j - 1);
                int up    = i * nY + (j + 1);

                // Solid mask (0 = solid, 1 = fluid)
                float s0 = solid[left];
                float s1 = solid[right];
                float s2 = solid[down];
                float s3 = solid[up];
                float denom = s0 + s1 + s2 + s3;

                if (denom == 0.0f) continue; // fully solid cell, skip

                // Velocity divergence
                float div = (x[right] - x[c]) + (y[up] - y[c]);

                // Optional: compensate for density drift
                if (compensateDrift && rest_density > 0.0f) {
                    float comp = p_density[c] - rest_density;
                    if (comp > 0.0f) div -= comp;
                }

                // Pressure correction
                float deltaP = -div / denom;
                deltaP *= overRelaxation;

                // Accumulate into pressure field (scaled like JS)
                p[c] += cp * deltaP;

                // Apply correction to velocities
                x[c]     -= s0 * deltaP;
                x[right] += s1 * deltaP;
                y[c]     -= s2 * deltaP;
                y[up]    += s3 * deltaP;
            }
        }
    }
}

void Flip::setup() {
    // already done in ctor, but you can reset walls here
}

void Flip::simulate(float gx, float gy, float dt,
                    float flipRatio,
                    int numPressureIters,
                    int numParticleIters,
                    float overRelaxation,
                    bool compensateDrift)
{
    gravity(gx, gy, dt);
    particle_grid(numParticleIters);
    particleCollision();
    velocity_grid(true,  0.0f);
    solveIncompressibility(numPressureIters, dt, overRelaxation, compensateDrift);
    velocity_grid(false, flipRatio);

    
}
