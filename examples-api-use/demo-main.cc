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
#include <thread>
#include <chrono>


//type alias and namespace for rgb_matrix
using ImageVector = std::vector<Magick::Image>;
using std::min;
using std::max;
using namespace rgb_matrix;

//preprocessor macros for terminal text formatting
#define TERM_ERR  "\033[1;31m"
#define TERM_NORM "\033[0m"

//global flag for interrupt command c -> volatile to indicate do not optimize 
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

//defines a base class for all demos
class DemoRunner {
  protected:
    //protected so only subclasses can create demorunner instances
    DemoRunner(Canvas *canvas) : canvas_(canvas) {}  //stores a pointer to LED canvas
    inline Canvas *canvas() { return canvas_; } //protected acccessor to retrieve the canvas

  public:
    virtual ~DemoRunner() {} //virtual destructor for proper cleanup of 
    virtual void Run() = 0; //Demo Runner is an abstract class --> every derived class must implement this

  private:
    Canvas *const canvas_; //const because the canvas pointer can not be reassigned once object created
  };

ImageVector LoadImageAndScaleImage(const char *filename,
                                   int target_width,
                                   int target_height) {
  ImageVector result; //holds the final image (1 frame)
  ImageVector frames; //temp storage for all frames (gifs --> use this next)
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

std::vector<uint32_t> targetPixels;  // global vector to holds target 0xRRGGBB for each pixel from image

class GeneticColors : public DemoRunner {
public:
  GeneticColors(Canvas* canvas, int delay_ms = 200)
    : DemoRunner(canvas), delay_ms_(delay_ms) {
    width_ = canvas->width();
    height_ = canvas->height();
    popSize_ = width_ * height_;
    children_.resize(popSize_);
    parents_.resize(popSize_);
    std::srand(std::time(nullptr));
  }

  void Run() override {
    for (int i = 0; i < popSize_; ++i)
      children_[i].dna = randColor();

    parents_ = children_;

    showTargetImage();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    while (!interrupt_received) {
      swapGenerations();
      sortByFitness();
      mate();

      for (int i = 0; i < popSize_; ++i) {
        const auto& c = children_[i];
        int x = i % width_;
        int y = i / width_;
        canvas()->SetPixel(x, y, R(c.dna), G(c.dna), B(c.dna));
      }

      if (is85PercentFit()) {
        for (int i = 0; i < popSize_; ++i)
          guidedMutate(children_[i], targetPixels[i], 5);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
    }
  }

private:
  struct citizen {
    int dna = 0;
  };

  std::vector<citizen> parents_, children_;
  int popSize_;
  int width_, height_, delay_ms_;

  static int randColor() { return rand() & 0xFFFFFF; }

  static int calcFitness(int value, int target) {
    int dr = R(value) - R(target);
    int dg = G(value) - G(target);
    int db = B(value) - B(target);
    return dr * dr + dg * dg + db * db;
  }

  void sortByFitness() {
    std::vector<std::pair<int, citizen>> indexed;
    indexed.reserve(popSize_);
    for (int i = 0; i < popSize_; ++i)
      indexed.emplace_back(i, parents_[i]);

    std::sort(indexed.begin(), indexed.end(),
      [](const auto& a, const auto& b) {
        return calcFitness(a.second.dna, targetPixels[a.first]) <
               calcFitness(b.second.dna, targetPixels[b.first]);
      });

    for (int i = 0; i < popSize_; ++i)
      parents_[i] = indexed[i].second;
  }

  void mate() {
    constexpr float eliteRate = 0.1f;
    constexpr float mutationRate = 0.5f;
    int eliteCount = static_cast<int>(popSize_ * eliteRate);

    for (int i = 0; i < eliteCount; ++i)
      children_[i] = parents_[i];

    for (int i = eliteCount; i < popSize_; ++i) {
      children_[i] = parents_[i];
      if ((rand() / static_cast<float>(RAND_MAX)) < mutationRate)
        guidedMutate(children_[i], targetPixels[i], 5);
    }
  }

  void guidedMutate(citizen& c, uint32_t target, int step = 1) {
    int r = R(c.dna), g = G(c.dna), b = B(c.dna);
    int tr = R(target), tg = G(target), tb = B(target);

    r = (r < tr) ? std::min(255, r + step) : std::max(0, r - step);
    g = (g < tg) ? std::min(255, g + step) : std::max(0, g - step);
    b = (b < tb) ? std::min(255, b + step) : std::max(0, b - step);

    c.dna = (r << 16) | (g << 8) | b;
  }

  void showTargetImage() {
    for (int i = 0; i < popSize_; ++i) {
      int x = i % width_;
      int y = i / width_;
      uint32_t rgb = targetPixels[i];
      canvas()->SetPixel(x, y, R(rgb), G(rgb), B(rgb));
    }
  }

  void swapGenerations() {
    std::swap(parents_, children_);
  }

  bool is85PercentFit() {
    int fitCount = 0;
    for (int i = 0; i < popSize_; ++i) {
      if (calcFitness(children_[i].dna, targetPixels[i]) < 1)
        ++fitCount;
    }
    return (fitCount / static_cast<float>(popSize_)) > 0.85f;
  }

  static int R(int color) { return (color >> 16) & 0xFF; }
  static int G(int color) { return (color >> 8) & 0xFF; }
  static int B(int color) { return color & 0xFF; }
};


///// 
//class GuidedCOlorEvolution inherits from DemoRunner--> DemoRunner requires a Run() method to access matrix
class GuidedColorEvolution : public DemoRunner {
public:
//constructor
  GuidedColorEvolution(Canvas *canvas, int delay_ms = 50)
    : DemoRunner(canvas), delay_ms_(delay_ms) {
    width_ = canvas->width();
    height_ = canvas->height();
    popSize_ = width_ * height_; //total number of screen pixels
    pixels_.resize(popSize_); //inits vector pixels_ to hold a color value (uint32_t) for each pixel 
    srand(time(NULL)); //seeds random number generator with time

    // inits each pixel with a random color 
    for (int i = 0; i < popSize_; ++i) {
      pixels_[i] = rand() & 0xFFFFFF;
    }
  }

  void Run() override {
    //while there is no interrupt 
    while (!interrupt_received) {
      bool allMatch = true; //flag to track if all pixels have reached target colors

      // loops through every pixel 
      for (int i = 0; i < popSize_; ++i) {
        uint32_t target = targetPixels[i]; //fetches target pixel 
        guidedMutate(pixels_[i], target); // calls guidedMutate to adjust the curent color pixel color 

        //separates pixel to x y
        int x = i % width_;
        int y = i / width_;
        canvas()->SetPixel(x, y, 
                           (pixels_[i] >> 16) & 0xFF,
                           (pixels_[i] >> 8) & 0xFF,
                           pixels_[i] & 0xFF);

        if (pixels_[i] != target) allMatch = false; //if any pixel not yet at target set to false
      }

      //if all pixels are at their targets; hold the completed image then  exit the loop we are done or sleep 
      if (allMatch) {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
  }
  }

//private helper function 
//takes current pixel color and target color and adjusts it by 1 step toward the target in each RGB channel 
//deterministic mutation --> nudging to the target 
private:
  void guidedMutate(uint32_t &color, uint32_t target, int step = 1) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;

    int tr = (target >> 16) & 0xFF;
    int tg = (target >> 8) & 0xFF;
    int tb = target & 0xFF;

    if (r < tr) r = std::min(255, r + step);
    else if (r > tr) r = std::max(0, r - step);

    if (g < tg) g = std::min(255, g + step);
    else if (g > tg) g = std::max(0, g - step);

    if (b < tb) b = std::min(255, b + step);
    else if (b > tb) b = std::max(0, b - step);

    color = (r << 16) | (g << 8) | b;
  }

  //some private members
  std::vector<uint32_t> pixels_; //stores current evolving color of each pixel
  int width_, height_, popSize_; 
  int delay_ms_; //how long towait before each mutation step
};


//instructions for if a user runs program incorrectly 
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
          "\t10 - Guided mutation towards input image(-m <time-step-ms>)\n"
          "\t11 - Pure evolution towards input image \n" 
          "\t12 - Portal animation by Catherine\n");

  fprintf(stderr, "Example:\n\t%s -D 1 runtext.ppm\n"
          "Scrolls the runtext until Ctrl-C is pressed\n", progname);
  return 1;
}

int main(int argc, char *argv[]) {
  Magick::InitializeMagick(*argv);  // init graphics magick library 
  int scroll_ms = 30;
  int demo = -1;


  const char *demo_parameter = NULL;
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 32;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;

  // extract the command line flags that contain relevant matrix options.
  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }
  // loops through command line flags 
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

  //matrix set up --> creates the matrix based on command line config 
  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return 1;

  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
         matrix->width(), matrix->height(), matrix_options.hardware_mapping);

  //loads and prepares image to fit matrix size
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
  // launch the demo 
  DemoRunner *demo_runner = NULL;
  switch (demo) {
  case 10:
    demo_runner = new GuidedColorEvolution(canvas, scroll_ms);
    break;
  
  case 11:
    demo_runner = new GeneticColors(canvas, scroll_ms);
    break;


  }

  if (demo_runner == NULL)
    return usage(argv[0]);

  if (demo == 10 && demo_parameter == NULL) {
  fprintf(stderr, TERM_ERR "Demo 10 requires a target image filename.\n" TERM_NORM);
  return usage(argv[0]);
}

  // interrupt set up and execution 
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  printf("Press <CTRL-C> to exit and reset LEDs\n");

  // Now, run our particular demo; it will exit when it sees interrupt_received.
  demo_runner->Run();


  // clean up: frees memory and exits 
  delete demo_runner;
  delete canvas;
  printf("Received CTRL-C. Exiting.\n");
  return 0;
}
