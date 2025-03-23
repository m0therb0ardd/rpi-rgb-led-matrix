#include "led-matrix.h"
#include "graphics.h"
#include <unistd.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace rgb_matrix;
using namespace std;

struct FallingChar {
  char ch;
  int x;
  int y;
  bool settled;
  FallingChar(char c, int xpos) : ch(c), x(xpos), y(0), settled(false) {}
};

int main(int argc, char *argv[]) {
  RGBMatrix::Options options;
  RuntimeOptions runtime;
  options.rows = 32;
  options.cols = 64;

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(options, runtime);
  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();  // ✅ FIXED

  rgb_matrix::Font font;
  if (!font.LoadFont("../fonts/10x20.bdf")) {
      fprintf(stderr, "Couldn't load font\n");
      return 1;
  }

  Color white(255, 255, 255);

  string secret = "I love you.";
  vector<FallingChar> falling_chars;

  srand(time(0));
  int index = 0;

  while (true) {
    offscreen_canvas->Clear();

    // Add new character occasionally
    if (index < secret.length() && rand() % 5 == 0) {
      int xpos = rand() % (offscreen_canvas->width() - 6);  // Character width padding
      falling_chars.emplace_back(secret[index], xpos);
      index++;
    }

    for (auto &fc : falling_chars) {
      fc.y += 1;
      if (fc.y > offscreen_canvas->height()) {
        fc.y = 0;
      }
      DrawText(offscreen_canvas, font, fc.x, fc.y, white, nullptr, string(1, fc.ch).c_str());
    }


    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
    usleep(50 * 1000);  // 50ms delay ~ 20fps
  }

  matrix->Clear();
  delete matrix;
  return 0;
}
