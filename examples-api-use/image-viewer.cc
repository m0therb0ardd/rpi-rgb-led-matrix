// SPDX-License-IdentifierText: 2024 Catherine + ChatGPT
// SPDX-License-Identifier: MIT

#include "led-matrix.h"
#include "graphics.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
rgb_matrix::RGBMatrix::Options;


struct RGB {
  uint8_t r, g, b;
};

// Load a PPM (P6) image file into memory
bool LoadPPM(const std::string& filename, int& width, int& height, RGB*& data) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Could not open file: " << filename << std::endl;
    return false;
  }

  std::string header;
  file >> header;
  if (header != "P6") {
    std::cerr << "Not a P6 PPM file." << std::endl;
    return false;
  }

  file >> width >> height;
  int maxval;
  file >> maxval;
  file.ignore(); // Skip one byte (newline)

  data = new RGB[width * height];
  file.read(reinterpret_cast<char*>(data), width * height * 3);
  return true;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: sudo ./image-viewer <image.ppm>" << std::endl;
    return 1;
  }

  // Load image
  int img_width, img_height;
  RGB* img_data = nullptr;
  if (!LoadPPM(argv[1], img_width, img_height, img_data)) {
    return 1;
  }

  // Setup matrix options
  RGBMatrixOptions options;
  options.rows = 64;
  options.cols = 64;
  options.chain_length = 6;
  options.parallel = 1;
  options.hardware_mapping = "adafruit-hat-pwm";
  options.gpio_slowdown = 4;

  RGBMatrix* matrix = RGBMatrix::CreateFromOptions(options, nullptr);
  if (!matrix) {
    std::cerr << "Failed to create matrix." << std::endl;
    return 1;
  }

  Canvas* canvas = matrix;

  // Draw image onto the canvas
  for (int y = 0; y < std::min(img_height, canvas->height()); ++y) {
    for (int x = 0; x < std::min(img_width, canvas->width()); ++x) {
      RGB& pixel = img_data[y * img_width + x];
      canvas->SetPixel(x, y, pixel.r, pixel.g, pixel.b);
    }
  }

  std::cout << "Image displayed. Press CTRL+C to exit." << std::endl;
  while (true) {
    sleep(100);
  }

  delete[] img_data;
  delete matrix;
  return 0;
}
