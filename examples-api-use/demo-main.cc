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
#include <iostream>



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
    std::cerr << "Error loading image: " << e.what() << std::endl;

    return result;
  }

  if (frames.empty()) {
    std::cerr << "No image found in " << filename << "." << std::endl;
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
  std::cerr << "usage: " << progname << " <options> -D <demo-nr> [optional parameter]\n";
  std::cerr << "Options:\n";
  std::cerr << "\t-D <demo-nr>              : Always needs to be set\n";

  rgb_matrix::PrintMatrixFlags(stderr);  // This still uses FILE*, leave as-is unless library supports streams

  std::cerr << "Demos, chosen with -D\n"
            << "\t0  - some rotating square\n"
            << "\t10 - Guided mutation towards input image (-m <time-step-ms>)\n"
            << "\t11 - Pure evolution towards input image\n"
            << "\t12 - Portal animation by Catherine\n";

  std::cerr << "Example:\n\t" << progname << " -D 1 runtext.ppm\n"
            << "Scrolls the runtext until Ctrl-C is pressed\n";

  return 1;
}

int main(int argc, char *argv[]) {
  Magick::InitializeMagick(*argv);  // Initialize GraphicsMagick

  int scroll_ms = 30;
  int demo = -1;
  const char *demo_parameter = NULL;

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  matrix_options.rows = 32;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;

  // Parse matrix options
  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  // Parse command-line options
  int opt;
  while ((opt = getopt(argc, argv, "dD:r:P:c:p:b:m:LR:")) != -1) {
    switch (opt) {
      case 'D': demo = atoi(optarg); break;
      case 'm': scroll_ms = atoi(optarg); break;
      default: return usage(argv[0]);
    }
  }

  if (optind < argc) {
    demo_parameter = argv[optind];
  }

  if (demo < 0) {
    std::cerr << TERM_ERR << "Expected required option -D <demo>\n" << TERM_NORM;

    return usage(argv[0]);
  }

  // Create LED matrix
  auto matrix = std::unique_ptr<RGBMatrix>(
      RGBMatrix::CreateFromOptions(matrix_options, runtime_opt));

  if (!matrix) {
    std::cerr << TERM_ERR << "Failed to initialize matrix.\n" << TERM_NORM;
    return 1;
  }

  Canvas *canvas = matrix.get();

  // Load and scale the image (first frame for now, but supports GIF later)
  ImageVector images = LoadImageAndScaleImage(
      demo_parameter, matrix->width(), matrix->height());

  if (images.empty()) {
    std::cerr << TERM_ERR << "Failed to load target image.\n" << TERM_NORM;
    return 1;
  }

  // Convert first image to targetPixels
  const Magick::Image &targetImage = images[0];
  targetPixels.clear();
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
    std::cerr << TERM_ERR << "ERROR: targetPixels size (" 
              << targetPixels.size() << ") does not match matrix size ("
              << matrix->width() * matrix->height() << ")\n" << TERM_NORM;
    return 1;
  }

  //Create appropriate demo runner
  std::unique_ptr<DemoRunner> demo_runner;

  switch (demo) {
    case 10:
      demo_runner = std::make_unique<GuidedColorEvolution>(canvas, scroll_ms);
      break;
    case 11:
      demo_runner = std::make_unique<GeneticColors>(canvas, scroll_ms);
      break;
    default:
      return usage(argv[0]);
  }

  if (!demo_runner) {
    return usage(argv[0]);
  }

  // Set up signal handler to exit on CTRL+C
  signal(SIGINT, InterruptHandler);
  signal(SIGTERM, InterruptHandler);
  std::cout << "Press <CTRL-C> to exit and reset LEDs\n";

  // Run the selected animation
  demo_runner->Run();

  std::cout << "Received CTRL-C. Exiting.\n";
  return 0;
}
