// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// This code is public domain
// (but note, once linked against the led-matrix library, this is
// covered by the GPL v2)
//
// This is a grab-bag of various demos and not very readable.
#include "led-matrix.h"

#include "pixel-mapper.h"
#include "graphics.h"

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include <Magick++.h>
#include <magick/image.h>
#include <vector>
using ImageVector = std::vector<Magick::Image>;


using std::min;
using std::max;

#define TERM_ERR  "\033[1;31m"
#define TERM_NORM "\033[0m"

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

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


ImageVector LoadImageAndScaleImage(const char *filename,
                                   int target_width,
                                   int target_height) {
  ImageVector result;
  ImageVector frames;
  try {
    readImages(&frames, filename);
  } catch (std::exception &e) {
    fprintf(stderr, "Error loading image: %s\n", e.what());
    return result;
  }

  if (frames.empty()) {
    fprintf(stderr, "No image found in %s.\n", filename);
    return result;
  }

  result.push_back(frames[0]);  // Only use first frame
  result[0].scale(Magick::Geometry(target_width, target_height));
  return result;
}

std::vector<uint32_t> targetPixels;  // Holds 0xRRGGBB for each pixel


/*
 * The following are demo image generators. They all use the utility
 * class DemoRunner to generate new frames.
 */

class SimpleSquare : public DemoRunner {
public:
  SimpleSquare(Canvas *m) : DemoRunner(m) {}
  void Run() override {
    const int width = canvas()->width() - 1;
    const int height = canvas()->height() - 1;
    // Borders
    DrawLine(canvas(), 0, 0,      width, 0,      Color(255, 0, 0));
    DrawLine(canvas(), 0, height, width, height, Color(255, 255, 0));
    DrawLine(canvas(), 0, 0,      0,     height, Color(0, 0, 255));
    DrawLine(canvas(), width, 0,  width, height, Color(0, 255, 0));

    // Diagonals.
    DrawLine(canvas(), 0, 0,        width, height, Color(255, 255, 255));
    DrawLine(canvas(), 0, height, width, 0,        Color(255,   0, 255));
  }
};


// SIMPLE PORTAL: CATHERINE EXPERIMENTING 
// class PortalEffect : public DemoRunner {
// public:
//   PortalEffect(Canvas *m, int delay_ms = 50)
//     : DemoRunner(m), delay_ms_(delay_ms), t_(0) {
//     center_x_ = canvas()->width() / 2;
//     center_y_ = canvas()->height() / 2;
//   }

//   void Run() override {
//     while (!interrupt_received) {
//       canvas()->Clear();
//       int radius = (t_ % 20) + 1;
//       DrawCircle(canvas(), center_x_, center_y_, radius,
//                  Color((radius * 10) % 255, 100, 255 - (radius * 10) % 255));
//       t_++;
//       usleep(delay_ms_ * 1000);
//     }
//   }

// private:
//   int delay_ms_;
//   int t_;
//   int center_x_;
//   int center_y_;
// };


// A COOLER PORTAL WITH EVOLUTIONARY CONCEPTS FOR COLOR GRADIENT AND SHAPE CHANGE
// //something new testing///////////////

// #include <cstdint> // For uint8_t
// #include <vector>
// #include <cstdlib>
// #include <ctime>
// #include <thread>
// #include <mutex>
// #include <algorithm>
// #include "led-matrix.h" // Include the RGB matrix library

// using namespace rgb_matrix;

// // Define a namespace for your custom code
// namespace MyApp {
//     // Define the Color struct
//     struct Color {
//         uint8_t r, g, b;
//         Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
//     };

//     // Implement DrawCircle
//     void DrawCircle(Canvas *canvas, int x, int y, int radius, const Color& color) {
//         for (int i = -radius; i <= radius; ++i) {
//             for (int j = -radius; j <= radius; ++j) {
//                 if (i * i + j * j <= radius * radius) {
//                     canvas->SetPixel(x + i, y + j, color.r, color.g, color.b);
//                 }
//             }
//         }
//     }

//     // Implement DrawSquare
//     void DrawSquare(Canvas *canvas, int x, int y, int size, const Color& color) {
//         for (int i = x - size; i <= x + size; ++i) {
//             for (int j = y - size; j <= y + size; ++j) {
//                 canvas->SetPixel(i, j, color.r, color.g, color.b);
//             }
//         }
//     }

//     // Implement DrawTriangle
//     void DrawTriangle(Canvas *canvas, int x, int y, int size, const Color& color) {
//         for (int i = 0; i <= size; ++i) {
//             for (int j = -i; j <= i; ++j) {
//                 canvas->SetPixel(x + j, y - i, color.r, color.g, color.b);
//             }
//         }
//     }
// } // End of namespace MyApp

// // Define PortalGene and PortalChromosome
// struct PortalGene {
//     int color_r;
//     int color_g;
//     int color_b;
//     int shape; // 0: circle, 1: square, 2: triangle
// };

// class PortalChromosome {
// public:
//     PortalChromosome() {
//         genes.color_r = rand() % 256;
//         genes.color_g = rand() % 256;
//         genes.color_b = rand() % 256;
//         genes.shape = rand() % 3;
//     }

//     PortalChromosome(const PortalGene& g) : genes(g) {}

//     PortalGene genes;
//     float fitness = 0.0f;

//     void mutate() {
//         if (rand() % 100 < 10) { // 10% mutation rate
//             genes.color_r = rand() % 256;
//         }
//         if (rand() % 100 < 10) {
//             genes.color_g = rand() % 256;
//         }
//         if (rand() % 100 < 10) {
//             genes.color_b = rand() % 256;
//         }
//         if (rand() % 100 < 10) {
//             genes.shape = rand() % 3;
//         }
//     }

//     static PortalChromosome crossover(const PortalChromosome& a, const PortalChromosome& b) {
//         PortalGene new_genes;
//         new_genes.color_r = (rand() % 2) ? a.genes.color_r : b.genes.color_r;
//         new_genes.color_g = (rand() % 2) ? a.genes.color_g : b.genes.color_g;
//         new_genes.color_b = (rand() % 2) ? a.genes.color_b : b.genes.color_b;
//         new_genes.shape = (rand() % 2) ? a.genes.shape : b.genes.shape;
//         return PortalChromosome(new_genes);
//     }
// };

// // Define the PortalPopulation class
// class PortalPopulation {
// public:
//     PortalPopulation(int size) {
//         for (int i = 0; i < size; ++i) {
//             population.push_back(PortalChromosome());
//         }
//     }

//     void evolve() {
//         calculateFitness();
//         std::vector<PortalChromosome> new_population;

//         // Elitism: keep the best portal
//         auto best = std::max_element(population.begin(), population.end(),
//                                      [](const PortalChromosome& a, const PortalChromosome& b) {
//                                          return a.fitness < b.fitness;
//                                      });
//         new_population.push_back(*best);

//         // Create the rest of the population through crossover and mutation
//         while (new_population.size() < population.size()) {
//             int a = rand() % population.size();
//             int b = rand() % population.size();
//             PortalChromosome child = PortalChromosome::crossover(population[a], population[b]);
//             child.mutate();
//             new_population.push_back(child);
//         }

//         population = new_population;
//     }

//     void calculateFitness() {
//         for (auto& portal : population) {
//             // Fitness based on how "bright" the portal is (sum of RGB)
//             portal.fitness = (portal.genes.color_r + portal.genes.color_g + portal.genes.color_b) / 3.0f;
//         }
//     }

//     std::vector<PortalChromosome> population;
// };

// // Define the PortalEffect class
// class PortalEffect : public DemoRunner {
// public:
//     PortalEffect(Canvas *m, int delay_ms = 50, int num_portals = 5)
//         : DemoRunner(m), delay_ms_(delay_ms), num_portals_(num_portals), population_(num_portals) {
//         center_x_ = canvas()->width() / 2;
//         center_y_ = canvas()->height() / 2;
//         srand(time(0));
//     }

//     void Run() override {
//         std::vector<std::thread> threads;
//         for (int i = 0; i < num_portals_; ++i) {
//             threads.emplace_back(&PortalEffect::portalThread, this, i);
//         }

//         while (!interrupt_received) {
//             usleep(delay_ms_ * 1000);
//         }

//         for (auto& t : threads) {
//             t.join();
//         }
//     }

// private:
//     void portalThread(int portal_id) {
//         while (!interrupt_received) {
//             std::lock_guard<std::mutex> lock(mutex_);
//             PortalChromosome& portal = population_.population[portal_id];

//             // Render the portal
//             canvas()->Clear();
//             int radius = (t_ % 20) + 1;
//             if (portal.genes.shape == 0) {
//                 MyApp::DrawCircle(canvas(), center_x_, center_y_, radius,
//                                   MyApp::Color(portal.genes.color_r, portal.genes.color_g, portal.genes.color_b));
//             } else if (portal.genes.shape == 1) {
//                 MyApp::DrawSquare(canvas(), center_x_, center_y_, radius,
//                                   MyApp::Color(portal.genes.color_r, portal.genes.color_g, portal.genes.color_b));
//             } else {
//                 MyApp::DrawTriangle(canvas(), center_x_, center_y_, radius,
//                                     MyApp::Color(portal.genes.color_r, portal.genes.color_g, portal.genes.color_b));
//             }
//             t_++;

//             // Evolve the population
//             population_.evolve();

//             usleep(delay_ms_ * 1000);
//         }
//     }

//     int delay_ms_;
//     int num_portals_;
//     int t_;
//     int center_x_;
//     int center_y_;
//     PortalPopulation population_;
//     std::mutex mutex_;
// };
/////////////////////////// end of testing

// /// Genetic Colors
// /// A genetic algorithm to evolve colors
// /// by bbhsu2 + anonymous
// class GeneticColors : public DemoRunner {
// public:
//   GeneticColors(Canvas *m, int delay_ms = 200)
//     : DemoRunner(m), delay_ms_(delay_ms) {
//     width_ = canvas()->width();
//     height_ = canvas()->height();
//     popSize_ = width_ * height_;

//     // Allocate memory
//     children_ = new citizen[popSize_];
//     parents_ = new citizen[popSize_];
//     srand(time(NULL));
//   }

//   ~GeneticColors() {
//     delete [] children_;
//     delete [] parents_;
//   }

//   static int rnd (int i) { return rand() % i; }

//   void Run() override {
//     // Set a random target_
//     target_ = 0x123456;

//     // Create the first generation of random children_
//     for (int i = 0; i < popSize_; ++i) {
//       children_[i].dna = rand() & 0xFFFFFF;
//     }

//     // Show initial random generation
//     for (int i = 0; i < popSize_; ++i) {
//       int c = children_[i].dna;
//       int x = i % width_;
//       int y = i / width_;
//       canvas()->SetPixel(x, y, R(c), G(c), B(c));
//     }
//     usleep(2000);  // Pause 1 second so you can see it


//     while (!interrupt_received && !is85PercentFit() ) {
//       swap();
//       sort();
//       mate();
//       std::random_shuffle (children_, children_ + popSize_, rnd);

//       // Draw citizens to canvas
//       for(int i=0; i < popSize_; i++) {
//         int c = children_[i].dna;
//         int x = i % width_;
//         int y = (int)(i / width_);
//         canvas()->SetPixel(x, y, R(c), G(c), B(c));
//       }
//       usleep(2000);  // Pause 1 second so you can see it


//       // When we reach the 85% fitness threshold...
//       if(is85PercentFit()) {
//         // ...set a new random target_
//         target_ = rand() & 0xFFFFFF;

//         // Randomly mutate everyone for sake of new colors
//         for (int i = 0; i < popSize_; ++i) {
//           mutate(children_[i]);
//         }
//       }
//       usleep(delay_ms_ * 1000);
//     }
//     // Show final result before exit
//     for (int i = 0; i < popSize_; ++i) {
//       int c = children_[i].dna;
//       int x = i % width_;
//       int y = i / width_;
//       canvas()->SetPixel(x, y, R(c), G(c), B(c));
//     }
//     usleep(5000000);  // Pause 5 seconds before exiting


//   }

// private:
//   /// citizen will hold dna information, a 24-bit color value.
//   struct citizen {
//     citizen() { }

//     citizen(int chrom)
//       : dna(chrom) {
//     }

//     int dna;
//   };

//   /// for sorting by fitness
//   class comparer {
//   public:
//     comparer(int t)
//       : target_(t) { }

//     inline bool operator() (const citizen& c1, const citizen& c2) {
//       return (calcFitness(c1.dna, target_) < calcFitness(c2.dna, target_));
//     }

//   private:
//     const int target_;
//   };

//   static int R(const int cit) { return at(cit, 16); }
//   static int G(const int cit) { return at(cit, 8); }
//   static int B(const int cit) { return at(cit, 0); }
//   static int at(const int v, const  int offset) { return (v >> offset) & 0xFF; }

//   /// fitness here is how "similar" the color is to the target
//   static int calcFitness(const int value, const int target) {
//     // Count the number of differing bits
//     int diffBits = 0;
//     for (unsigned int diff = value ^ target; diff; diff &= diff - 1) {
//       ++diffBits;
//     }
//     return diffBits;
//   }

//   /// sort by fitness so the most fit citizens are at the top of parents_
//   /// this is to establish an elite population of greatest fitness
//   /// the most fit members and some others are allowed to reproduce
//   /// to the next generation
//   void sort() {
//     std::sort(parents_, parents_ + popSize_, comparer(target_));
//   }

//   /// let the elites continue to the next generation children
//   /// randomly select 2 parents of (near)elite fitness and determine
//   /// how they will mate. after mating, randomly mutate citizens
//   void mate() {
//     // Adjust these for fun and profit
//     const float eliteRate = 0.30f;
//     const float mutationRate = 0.20f;

//     const int numElite = popSize_ * eliteRate;
//     for (int i = 0; i < numElite; ++i) {
//       children_[i] = parents_[i];
//     }

//     for (int i = numElite; i < popSize_; ++i) {
//       //select the parents randomly
//       const float sexuallyActive = 1.0 - eliteRate;
//       const int p1 = rand() % (int)(popSize_ * sexuallyActive);
//       const int p2 = rand() % (int)(popSize_ * sexuallyActive);
//       const unsigned matingMask = (~0u) << (rand() % bitsPerPixel);

//       // Make a baby
//       unsigned baby = (parents_[p1].dna & matingMask)
//         | (parents_[p2].dna & ~matingMask);
//       children_[i].dna = baby;

//       // Mutate randomly based on mutation rate
//       if ((rand() / (float)RAND_MAX) < mutationRate) {
//         mutate(children_[i]);
//       }
//     }
//   }

//   /// parents make children,
//   /// children become parents,
//   /// and they make children...
//   void swap() {
//     citizen* temp = parents_;
//     parents_ = children_;
//     children_ = temp;
//   }

//   void mutate(citizen& c) {
//     // Flip a random bit
//     c.dna ^= 1 << (rand() % bitsPerPixel);
//   }

//   /// can adjust this threshold to make transition to new target seamless
//   bool is85PercentFit() {
//     int numFit = 0;
//     for (int i = 0; i < popSize_; ++i) {
//       if (calcFitness(children_[i].dna, target_) < 1) {
//         ++numFit;
//       }
//     }
//     return ((numFit / (float)popSize_) > 0.85f);
//   }

//   static const int bitsPerPixel = 24;
//   int popSize_;
//   int width_, height_;
//   int delay_ms_;
//   int target_;
//   citizen* children_;
//   citizen* parents_;
// };
//////////////

// #include <algorithm>
// #include <cstdlib>
// #include <ctime>
// #include <unistd.h>
// #include <vector>

// class GeneticColors : public DemoRunner {
// public:
//   GeneticColors(Canvas *m, int delay_ms = 200)
//     : DemoRunner(m), delay_ms_(delay_ms) {
//     width_ = canvas()->width();
//     height_ = canvas()->height();
//     popSize_ = width_ * height_;

//     // Allocate memory
//     children_ = new citizen[popSize_];
//     parents_ = new citizen[popSize_];
//     srand(time(NULL));

//     // Initialize only the corner with random colors
//     initializeCorner();
//   }

//   ~GeneticColors() {
//     delete [] children_;
//     delete [] parents_;
//   }

//   void Run() override {
//     // Set a random target_
//     target_ = rand() & 0xFFFFFF;

//     while (!interrupt_received) {
//       // Spread colors to neighboring pixels
//       spreadColors();

//       // Evolve the active pixels
//       evolveActivePixels();

//       // Draw the current state to the canvas
//       drawCanvas();

//       // Check if we need to set a new target
//       if (is85PercentFit()) {
//         target_ = rand() & 0xFFFFFF; // Set a new random target
//       }

//       usleep(delay_ms_ * 1000); // Delay between generations
//     }
//   }

// private:
//   struct citizen {
//     int dna; // 24-bit color value
//   };

//   static int R(int color) { return (color >> 16) & 0xFF; }
//   static int G(int color) { return (color >> 8) & 0xFF; }
//   static int B(int color) { return color & 0xFF; }

//   /// Initialize only the corner with random colors
//   void initializeCorner() {
//     // Clear the entire canvas (set all pixels to black/off)
//     for (int i = 0; i < popSize_; ++i) {
//       children_[i].dna = 0x000000; // Black
//     }

//     // Initialize the bottom-left corner (e.g., 4x4 pixels) with random colors
//     int cornerSize = 4; // Adjust this for the size of the initial corner
//     for (int y = 0; y < cornerSize; ++y) {
//       for (int x = 0; x < cornerSize; ++x) {
//         int index = y * width_ + x;
//         children_[index].dna = rand() & 0xFFFFFF; // Random color
//         activePixels_.push_back(index); // Mark these pixels as active
//       }
//     }
//   }

//   /// Spread colors to neighboring pixels
//   void spreadColors() {
//     std::vector<int> newActivePixels;

//     for (int index : activePixels_) {
//       int x = index % width_;
//       int y = index / width_;

//       // Check all 4 neighbors (up, down, left, right)
//       int neighbors[4][2] = {{x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}};

//       for (int i = 0; i < 4; ++i) {
//         int nx = neighbors[i][0];
//         int ny = neighbors[i][1];

//         if (nx >= 0 && nx < width_ && ny >= 0 && ny < height_) {
//           int neighborIndex = ny * width_ + nx;

//           // If the neighbor is inactive, activate it and copy the color
//           if (children_[neighborIndex].dna == 0x000000) {
//             children_[neighborIndex].dna = children_[index].dna;
//             newActivePixels.push_back(neighborIndex);
//           }
//         }
//       }
//     }

//     // Update the list of active pixels
//     activePixels_.insert(activePixels_.end(), newActivePixels.begin(), newActivePixels.end());
//   }

//   /// Evolve the active pixels using a genetic algorithm
//   void evolveActivePixels() {
//     for (int index : activePixels_) {
//       // Mutate the pixel's color to move closer to the target
//       if (rand() / (float)RAND_MAX < mutationRate_) {
//         guidedMutate(children_[index]);
//       }
//     }
//   }

//   /// Guided mutation: Move the color closer to the target
//   void guidedMutate(citizen& c) {
//     int currentR = R(c.dna);
//     int currentG = G(c.dna);
//     int currentB = B(c.dna);

//     int targetR = R(target_);
//     int targetG = G(target_);
//     int targetB = B(target_);

//     // Adjust one of the RGB components to move closer to the target
//     int component = rand() % 3; // Choose R, G, or B
//     if (component == 0) {
//       currentR = (currentR < targetR) ? currentR + 1 : currentR - 1;
//     } else if (component == 1) {
//       currentG = (currentG < targetG) ? currentG + 1 : currentG - 1;
//     } else {
//       currentB = (currentB < targetB) ? currentB + 1 : currentB - 1;
//     }

//     // Clamp the values to 0-255
//     currentR = std::max(0, std::min(255, currentR));
//     currentG = std::max(0, std::min(255, currentG));
//     currentB = std::max(0, std::min(255, currentB));

//     // Update the color
//     c.dna = (currentR << 16) | (currentG << 8) | currentB;
//   }

//   /// Check if 85% of the active pixels are close to the target
//   bool is85PercentFit() {
//     int numFit = 0;
//     for (int index : activePixels_) {
//       if (calcFitness(children_[index].dna, target_) <= fitnessThreshold_) {
//         ++numFit;
//       }
//     }
//     return ((numFit / (float)activePixels_.size()) > 0.85f);
//   }

//   /// Calculate fitness (number of differing bits)
//   static int calcFitness(const int value, const int target) {
//     int diffBits = 0;
//     for (unsigned int diff = value ^ target; diff; diff &= diff - 1) {
//       ++diffBits;
//     }
//     return diffBits;
//   }

//   /// Draw the current state to the canvas
//   void drawCanvas() {
//     for (int i = 0; i < popSize_; ++i) {
//       int c = children_[i].dna;
//       int x = i % width_;
//       int y = i / width_;
//       canvas()->SetPixel(x, y, R(c), G(c), B(c));
//     }
//   }

//   // Parameters
//   static const int bitsPerPixel = 24;
//   const float mutationRate_ = 0.20f; // Mutation rate
//   const int fitnessThreshold_ = 5; // Consider a pixel a match if it has <= 5 differing bits
//   int popSize_;
//   int width_, height_;
//   int delay_ms_;
//   int target_;
//   citizen* children_;
//   citizen* parents_;
//   std::vector<int> activePixels_; // Tracks which pixels are active (non-black)
// };

/// end evolutiuon 


/// Genetic Colors
/// A genetic algorithm to evolve colors
/// by bbhsu2 + anonymous
class GeneticColors : public DemoRunner {
public:
  GeneticColors(Canvas *m, int delay_ms = 200)
    : DemoRunner(m), delay_ms_(delay_ms) {
    width_ = canvas()->width();
    height_ = canvas()->height();
    popSize_ = width_ * height_;

    // Allocate memory
    children_ = new citizen[popSize_];
    parents_ = new citizen[popSize_];
    srand(time(NULL));
  }

  ~GeneticColors() {
    delete [] children_;
    delete [] parents_;
  }

  static int rnd (int i) { return rand() % i; }

  void Run() override {
    // Set a random target_
    //target_ = rand() & 0xFFFFFF;

    // Show the target image directly on the matrix (for confirmation)
    for (int i = 0; i < popSize_; ++i) {
      int x = i % width_;
      int y = i / width_;
      uint32_t rgb = targetPixels[i];
      canvas()->SetPixel(x, y,
                        (rgb >> 16) & 0xFF,
                        (rgb >> 8) & 0xFF,
                        rgb & 0xFF);
    }
    usleep(2000000); // show for 2 seconds


    // Create the first generation of random children_
    for (int i = 0; i < popSize_; ++i) {
      children_[i].dna = rand() & 0xFFFFFF;
    }

    while (!interrupt_received) {
      swap();
      sort();
      mate();
      std::random_shuffle (children_, children_ + popSize_, rnd);

      // Draw citizens to canvas
      for(int i=0; i < popSize_; i++) {
        int c = children_[i].dna;
        int x = i % width_;
        int y = (int)(i / width_);
        canvas()->SetPixel(x, y, R(c), G(c), B(c));
      }

      // When we reach the 85% fitness threshold...
      if(is85PercentFit()) {
        // ...set a new random target_
       // target_ = rand() & 0xFFFFFF;

        // Randomly mutate everyone for sake of new colors
        for (int i = 0; i < popSize_; ++i) {
          mutate(children_[i]);
        }
      }
      //usleep(delay_ms_ * 1000);
      usleep(50 * 1000);
    }
  }

private:
  /// citizen will hold dna information, a 24-bit color value.
  struct citizen {
    citizen() { }

    citizen(int chrom)
      : dna(chrom) {
    }

    int dna;
  };

  /// for sorting by fitness
  class comparer {
  public:
    comparer(citizen* parents) : parents_(parents) {}

    inline bool operator() (const citizen& c1, const citizen& c2) {
      int i1 = &c1 - parents_;
      int i2 = &c2 - parents_;
      return (calcFitness(c1.dna, targetPixels[i1]) < calcFitness(c2.dna, targetPixels[i2]));
    }

  private:
    citizen* parents_;  // Pointer to outer class's parents_ array
  };



  static int R(const int cit) { return at(cit, 16); }
  static int G(const int cit) { return at(cit, 8); }
  static int B(const int cit) { return at(cit, 0); }
  static int at(const int v, const  int offset) { return (v >> offset) & 0xFF; }

  /// fitness here is how "similar" the color is to the target
  static int calcFitness(const int value, const int target) {
    int diffBits = 0;
    for (unsigned int diff = value ^ target; diff; diff &= diff - 1) {
      ++diffBits;
    }
    return diffBits;
  }


  /// sort by fitness so the most fit citizens are at the top of parents_
  /// this is to establish an elite population of greatest fitness
  /// the most fit members and some others are allowed to reproduce
  /// to the next generation
void sort() {
  // Step 1: Create a vector of (index, citizen) pairs
  std::vector<std::pair<int, citizen>> indexed;
  indexed.reserve(popSize_);
  for (int i = 0; i < popSize_; ++i) {
    indexed.push_back({i, parents_[i]});
  }

  // Step 2: Sort by fitness to corresponding target pixel
  std::sort(indexed.begin(), indexed.end(),
            [](const std::pair<int, citizen>& a, const std::pair<int, citizen>& b) {
              return calcFitness(a.second.dna, targetPixels[a.first]) <
                     calcFitness(b.second.dna, targetPixels[b.first]);
            });

  // Step 3: Copy sorted citizens back into parents_
  for (int i = 0; i < popSize_; ++i) {
    parents_[i] = indexed[i].second;
  }
}



  /// let the elites continue to the next generation children
  /// randomly select 2 parents of (near)elite fitness and determine
  /// how they will mate. after mating, randomly mutate citizens
  void mate() {
    // Adjust these for fun and profit
    const float eliteRate = 0.30f;
    const float mutationRate = 0.20f;

    const int numElite = popSize_ * eliteRate;
    for (int i = 0; i < numElite; ++i) {
      children_[i] = parents_[i];
    }

    for (int i = numElite; i < popSize_; ++i) {
      //select the parents randomly
      const float sexuallyActive = 1.0 - eliteRate;
      const int p1 = rand() % (int)(popSize_ * sexuallyActive);
      const int p2 = rand() % (int)(popSize_ * sexuallyActive);
      const unsigned matingMask = (~0u) << (rand() % bitsPerPixel);

      // Make a baby
      unsigned baby = (parents_[p1].dna & matingMask)
        | (parents_[p2].dna & ~matingMask);
      children_[i].dna = baby;

      // Mutate randomly based on mutation rate
      if ((rand() / (float)RAND_MAX) < mutationRate) {
        mutate(children_[i]);
      }
    }
  }

  /// parents make children,
  /// children become parents,
  /// and they make children...
  void swap() {
    citizen* temp = parents_;
    parents_ = children_;
    children_ = temp;
  }

  void mutate(citizen& c) {
    // Flip a random bit
    c.dna ^= 1 << (rand() % bitsPerPixel);
  }

  /// can adjust this threshold to make transition to new target seamless
  bool is85PercentFit() {
    int numFit = 0;
    for (int i = 0; i < popSize_; ++i) {
      if (calcFitness(children_[i].dna, targetPixels[i]) < 1) {
        ++numFit;
      }
    }
    return ((numFit / (float)popSize_) > 0.85f);
  }

  static const int bitsPerPixel = 24;
  int popSize_;
  int width_, height_;
  int delay_ms_;
  int target_;
  citizen* children_;
  citizen* parents_;
};


static int usage(const char *progname) {
  fprintf(stderr, "usage: %s <options> -D <demo-nr> [optional parameter]\n",
          progname);
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "\t-D <demo-nr>              : Always needs to be set\n"
          );


  rgb_matrix::PrintMatrixFlags(stderr);

  fprintf(stderr, "Demos, choosen with -D\n");
  fprintf(stderr, "\t0  - some rotating square\n"
          "\t1  - forward scrolling an image (-m <scroll-ms>)\n"
          "\t2  - backward scrolling an image (-m <scroll-ms>)\n"
          "\t3  - test image: a square\n"
          "\t4  - Pulsing color\n"
          "\t5  - Grayscale Block\n"
          "\t6  - Abelian sandpile model (-m <time-step-ms>)\n"
          "\t7  - Conway's game of life (-m <time-step-ms>)\n"
          "\t8  - Langton's ant (-m <time-step-ms>)\n"
          "\t9  - Volume bars (-m <time-step-ms>)\n"
          "\t10 - Evolution of color (-m <time-step-ms>)\n"
          "\t11 - Brightness pulse generator\n" 
          "\t12 - Portal animation by Catherine\n");

  fprintf(stderr, "Example:\n\t%s -D 1 runtext.ppm\n"
          "Scrolls the runtext until Ctrl-C is pressed\n", progname);
  return 1;
}

int main(int argc, char *argv[]) {
  Magick::InitializeMagick(*argv);  // ✅ Put this as the first line
  int demo = -1;
  int scroll_ms = 30;

  const char *demo_parameter = NULL;
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 32;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;

  // First things first: extract the command line flags that contain
  // relevant matrix options.
  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  int opt;
  while ((opt = getopt(argc, argv, "dD:r:P:c:p:b:m:LR:")) != -1) {
    switch (opt) {
    case 'D':
      demo = atoi(optarg);
      break;

    case 'm':
      scroll_ms = atoi(optarg);
      break;

    default: /* '?' */
      return usage(argv[0]);
    }
  }

  if (optind < argc) {
    demo_parameter = argv[optind];
  }

  if (demo < 0) {
    fprintf(stderr, TERM_ERR "Expected required option -D <demo>\n" TERM_NORM);
    return usage(argv[0]);
  }

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return 1;

  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
         matrix->width(), matrix->height(), matrix_options.hardware_mapping);

  Canvas *canvas = matrix;

  ImageVector images = LoadImageAndScaleImage(demo_parameter,
                                            matrix->width(),
                                              matrix->height());
  if (images.empty()) {
    fprintf(stderr, "Failed to load target image.\n");
    return 1;
  }

  const Magick::Image &targetImage = images[0];
  targetPixels.clear();  // Clear any existing data
  for (size_t y = 0; y < targetImage.rows(); ++y) {
    for (size_t x = 0; x < targetImage.columns(); ++x) {
      const Magick::Color &c = targetImage.pixelColor(x, y);
      uint32_t rgb = (ScaleQuantumToChar(c.redQuantum()) << 16) |
                    (ScaleQuantumToChar(c.greenQuantum()) << 8) |
                    ScaleQuantumToChar(c.blueQuantum());
      targetPixels.push_back(rgb);
    }
  }

  if (targetPixels.size() != matrix->width() * matrix->height()) {
  fprintf(stderr, TERM_ERR "ERROR: targetPixels size (%lu) does not match matrix size (%d)\n" TERM_NORM,
          targetPixels.size(), matrix->width() * matrix->height());
  return 1;
}
  // The DemoRunner objects are filling
  // the matrix continuously.
  DemoRunner *demo_runner = NULL;
  switch (demo) {
  case 3:
    demo_runner = new SimpleSquare(canvas);
    break;

  case 10:
    demo_runner = new GeneticColors(canvas, scroll_ms);
    break;

  }

  if (demo_runner == NULL)
    return usage(argv[0]);

  if (demo == 10 && demo_parameter == NULL) {
  fprintf(stderr, TERM_ERR "Demo 10 requires a target image filename.\n" TERM_NORM);
  return usage(argv[0]);
}


  // Set up an interrupt handler to be able to stop animations while they go
  // on. Each demo tests for while (!interrupt_received) {},
  // so they exit as soon as they get a signal.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  printf("Press <CTRL-C> to exit and reset LEDs\n");

  // Now, run our particular demo; it will exit when it sees interrupt_received.
  demo_runner->Run();

  delete demo_runner;
  delete canvas;

  printf("Received CTRL-C. Exiting.\n");
  return 0;
}
