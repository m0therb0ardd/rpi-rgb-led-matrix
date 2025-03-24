volatile bool interrupt_received = false;

#include "portal_effect.h"
#include <unistd.h> // For usleep
#include <cmath> // For sin and cos

// Declare interrupt_received as extern
extern volatile bool interrupt_received;


using namespace rgb_matrix;

// Implement DrawCircle
void MyApp::DrawCircle(Canvas *canvas, int x, int y, int radius, const Color& color) {
    for (int i = -radius; i <= radius; ++i) {
        for (int j = -radius; j <= radius; ++j) {
            if (i * i + j * j <= radius * radius) {
                canvas->SetPixel(x + i, y + j, color.r, color.g, color.b);
            }
        }
    }
}

// Implement DrawSquare
void MyApp::DrawSquare(Canvas *canvas, int x, int y, int size, const Color& color) {
    for (int i = x - size; i <= x + size; ++i) {
        for (int j = y - size; j <= y + size; ++j) {
            canvas->SetPixel(i, j, color.r, color.g, color.b);
        }
    }
}

// Implement DrawTriangle
void MyApp::DrawTriangle(Canvas *canvas, int x, int y, int size, const Color& color) {
    for (int i = 0; i <= size; ++i) {
        for (int j = -i; j <= i; ++j) {
            canvas->SetPixel(x + j, y - i, color.r, color.g, color.b);
        }
    }
}

// Implement PortalChromosome
PortalChromosome::PortalChromosome() {
    genes.color_r = rand() % 256;
    genes.color_g = rand() % 256;
    genes.color_b = rand() % 256;
    genes.shape = rand() % 3;
}

PortalChromosome::PortalChromosome(const PortalGene& g) : genes(g) {}

void PortalChromosome::mutate() {
    if (rand() % 100 < 10) { // 10% mutation rate
        genes.color_r = rand() % 256;
    }
    if (rand() % 100 < 10) {
        genes.color_g = rand() % 256;
    }
    if (rand() % 100 < 10) {
        genes.color_b = rand() % 256;
    }
    if (rand() % 100 < 10) {
        genes.shape = rand() % 3;
    }
}

PortalChromosome PortalChromosome::crossover(const PortalChromosome& a, const PortalChromosome& b) {
    PortalGene new_genes;
    new_genes.color_r = (rand() % 2) ? a.genes.color_r : b.genes.color_r;
    new_genes.color_g = (rand() % 2) ? a.genes.color_g : b.genes.color_g;
    new_genes.color_b = (rand() % 2) ? a.genes.color_b : b.genes.color_b;
    new_genes.shape = (rand() % 2) ? a.genes.shape : b.genes.shape;
    return PortalChromosome(new_genes);
}

// Implement PortalPopulation
PortalPopulation::PortalPopulation(int size) {
    for (int i = 0; i < size; ++i) {
        population.push_back(PortalChromosome());
    }
}

void PortalPopulation::evolve() {
    calculateFitness();
    std::vector<PortalChromosome> new_population;

    // Elitism: keep the best portal
    auto best = std::max_element(population.begin(), population.end(),
                                 [](const PortalChromosome& a, const PortalChromosome& b) {
                                     return a.fitness < b.fitness;
                                 });
    new_population.push_back(*best);

    // Create the rest of the population through crossover and mutation
    while (new_population.size() < population.size()) {
        int a = rand() % population.size();
        int b = rand() % population.size();
        PortalChromosome child = PortalChromosome::crossover(population[a], population[b]);
        child.mutate();
        new_population.push_back(child);
    }

    population = new_population;
}

void PortalPopulation::calculateFitness() {
    for (auto& portal : population) {
        // Fitness based on how "bright" the portal is (sum of RGB)
        portal.fitness = (portal.genes.color_r + portal.genes.color_g + portal.genes.color_b) / 3.0f;
    }
}

// Implement PortalEffect
PortalEffect::PortalEffect(Canvas *m, int delay_ms, int num_portals)
    : DemoRunner(m), delay_ms_(delay_ms), num_portals_(num_portals), population_(num_portals) {
    center_x_ = canvas()->width() / 2;
    center_y_ = canvas()->height() / 2;
    srand(time(0));
}

void PortalEffect::Run() {
    std::vector<std::thread> threads;
    for (int i = 0; i < num_portals_; ++i) {
        threads.emplace_back(&PortalEffect::portalThread, this, i);
    }

    while (!interrupt_received) {
        usleep(delay_ms_ * 1000);
    }

    for (auto& t : threads) {
        t.join();
    }
}

void PortalEffect::portalThread(int portal_id) {
    while (!interrupt_received) {
        std::lock_guard<std::mutex> lock(mutex_);
        PortalChromosome& portal = population_.population[portal_id];

        // Render the portal
        canvas()->Clear();
        int radius = (t_ % 20) + 1;
        if (portal.genes.shape == 0) {
            MyApp::DrawCircle(canvas(), center_x_, center_y_, radius,
                              MyApp::Color(portal.genes.color_r, portal.genes.color_g, portal.genes.color_b));
        } else if (portal.genes.shape == 1) {
            MyApp::DrawSquare(canvas(), center_x_, center_y_, radius,
                              MyApp::Color(portal.genes.color_r, portal.genes.color_g, portal.genes.color_b));
        } else {
            MyApp::DrawTriangle(canvas(), center_x_, center_y_, radius,
                                MyApp::Color(portal.genes.color_r, portal.genes.color_g, portal.genes.color_b));
        }
        t_++;

        // Evolve the population
        population_.evolve();

        usleep(delay_ms_ * 1000);
    }
}

// Add a main function
int main(int argc, char *argv[]) {
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;
    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == nullptr) {
        fprintf(stderr, "Failed to create RGB matrix.\n");
        return 1;
    }

    Canvas *canvas = matrix;

    // Create and run the PortalEffect demo
    PortalEffect portalEffect(canvas, 50, 5);
    portalEffect.Run();

    // Cleanup
    delete canvas;
    return 0;
}