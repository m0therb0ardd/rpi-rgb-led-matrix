#ifndef PORTAL_EFFECT_H
#define PORTAL_EFFECT_H

#include "led-matrix.h"
#include <cstdint>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <mutex>
#include <algorithm>

using namespace rgb_matrix;

// Forward declaration of the DemoRunner class
class DemoRunner {
protected:
    DemoRunner(Canvas *canvas) : canvas_(canvas) {}
    inline Canvas *canvas() { return canvas_; }

public:
    virtual ~DemoRunner() {}
    virtual void Run() = 0;

private:
    Canvas *const canvas_;
};

// Define a namespace for custom code
namespace MyApp {
    // Define the Color struct
    struct Color {
        uint8_t r, g, b;
        Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    };

    // Function declarations
    void DrawCircle(Canvas *canvas, int x, int y, int radius, const Color& color);
    void DrawSquare(Canvas *canvas, int x, int y, int size, const Color& color);
    void DrawTriangle(Canvas *canvas, int x, int y, int size, const Color& color);
} // End of namespace MyApp

// Define PortalGene and PortalChromosome
struct PortalGene {
    int color_r;
    int color_g;
    int color_b;
    int shape; // 0: circle, 1: square, 2: triangle
};

class PortalChromosome {
public:
    PortalChromosome();
    PortalChromosome(const PortalGene& g);
    PortalGene genes;
    float fitness;
    void mutate();
    static PortalChromosome crossover(const PortalChromosome& a, const PortalChromosome& b);
};

// Define the PortalPopulation class
class PortalPopulation {
public:
    PortalPopulation(int size);
    void evolve();
    void calculateFitness();
    std::vector<PortalChromosome> population;
};

// Define the PortalEffect class
class PortalEffect : public DemoRunner {
public:
    PortalEffect(Canvas *m, int delay_ms = 50, int num_portals = 5);
    void Run() override;

private:
    void portalThread(int portal_id);
    int delay_ms_;
    int num_portals_;
    int t_;
    int center_x_;
    int center_y_;
    PortalPopulation population_;
    std::mutex mutex_;
};

#endif // PORTAL_EFFECT_H