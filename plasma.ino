// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library
#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            5  // D1

#define WIDTH  16
#define HEIGHT 16

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      (WIDTH*HEIGHT)

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void plasma();

typedef struct {
  uint8_t r, g, b;
} color_t;

// calc a two dimentional position to onedimentrional position map array 
uint8_t mapPix[HEIGHT][WIDTH];
void initMap() {
  uint8_t i = 0;
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      uint8_t y2 = ((x % 2) == 0 ? y : HEIGHT-1-y);
      mapPix[x][y2] = i++;
    }
  }
}

// maximum brithness of leds
uint8_t brith = 80;
color_t scaleBrith(color_t in) {
  return (color_t) {
          .r=((uint16_t)in.r*brith)/255, 
          .g=((uint16_t)in.g*brith)/255, 
          .b=((uint16_t)in.b*brith)/255};
}

// set pixel with h value from hsv color space (rainbow colors)
void setPixelH(int x, int y, int h) {
  color_t col = scaleBrith(HtoRGB(h));
  pixels.setPixelColor(mapPix[x][y], pixels.Color(col.r, col.g, col.b));
}

#define SQUARE(x) ((x)*(x))

typedef struct {
  uint16_t XYSumOffset;
  uint16_t PolarOffset;
  uint16_t cxOffset;
  uint16_t cyOffset;
} plasma_state_t;

plasma_state_t plasma_state;
void plasma(plasma_state_t *state);
void showNextPlasmaframe(uint16_t ms); 

void setup() {
  initMap();
  pixels.begin(); // This initializes the NeoPixel library.
}

void loop() {
  showNextPlasmaframe(40);
}

void showNextPlasmaframe(uint16_t ms) {
  long start = millis();
  plasma(&plasma_state);
  long cur = millis();
  while ((cur - start) < ms) {
    delay(1);
    cur = millis();
  }
}

int isqrt(long n) {      /* Nur mit Bitshifts, Additionen und Subtraktionen */
  register int i;
  register long root = 0, left = 0;
  for (i = (sizeof(long) << 3) - 2; i >= 0; i -= 2) {
    left = left<<2 | n>>i & 3 ;
    root <<= 1;
    if (left > root) {
      left -= ++root;
      ++root;
    }
  }
  return (int)(root>>1) ;
}

const int16_t  sin_table[66] = {
      0,   804,  1608,  2410,  3212,  4011,  4808,  5602,
   6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
  12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
  18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
  23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
  27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
  30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
  32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
  32767, 32757
};

static int32_t Sine(int32_t phase) {
  int16_t s0;
  uint16_t tmp_phase, tmp_phase_hi;
  tmp_phase = phase & 0x7fff;
  if (tmp_phase & 0x4000)
    tmp_phase = 0x8000 - 1 - tmp_phase;
  tmp_phase_hi = tmp_phase >> 8; // 0...64
  s0 = sin_table[tmp_phase_hi];
  s0 += ((int16_t)((((int32_t)(sin_table[tmp_phase_hi+1] - s0))*
                              (tmp_phase&0xff) + 128) >> 8));
  if (phase & 0x8000) {
    s0 = -s0;
  }
  return s0;
}

color_t HtoRGB(int h31bit) { 
  h31bit %= 49152;
  if (h31bit < 0) {
    h31bit += 49152;
  }
  color_t rgb;
  unsigned char sextant;
  int           q;
  sextant  = h31bit / 8192;   // 60°
  h31bit   = h31bit % 8192;
  q        = 8191 - h31bit; 
  switch (sextant) {
      case 0:
        rgb.r = 255;
        rgb.g = h31bit / 32; 
        rgb.b = 0;
        break;
      case 1:
        rgb.r = q / 32;
        rgb.g = 255;
        rgb.b = 0;
        break;
      case 2:
        rgb.r = 0;
        rgb.g = 255;
        rgb.b = h31bit / 32;
        break;
      case 3:
        rgb.r = 0;
        rgb.g = q / 32;
        rgb.b = 255;
        break;
      case 4:
        rgb.r = h31bit / 32;
        rgb.g = 0;
        rgb.b = 255;
        break;
      default:
        rgb.r = 255;
        rgb.g = 0;
        rgb.b = q / 32;
        break;
  };
  return rgb;
}

// polar plasma
int plasma_Polar(int x, int y, unsigned int offset, unsigned int scale, int cx, int cy) {
  return Sine(((isqrt(SQUARE(x - (WIDTH*256/2) - cx) + SQUARE(y - (HEIGHT*256/2) - cy))*scale)/4095) + offset);
}

// sum of x-sine and y-sine
int plasma_XYSum(unsigned int x, unsigned int y, unsigned int offset, unsigned int scale) {
  return Sine((unsigned int)(x )*scale/65535 + offset) + Sine((unsigned int)(y )*scale/32767 + offset);
}

void plasma(plasma_state_t *state) {
  state->XYSumOffset += 80;    
  state->cxOffset    += 50;
  state->cyOffset    += 59;
  state->PolarOffset +=100;
  int cx = (Sine(state->cxOffset) * (WIDTH/2))/128;
  int cy = (Sine(state->cyOffset) * (HEIGHT/2))/128;
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint16_t mx = x*256;
      uint16_t my = y*256;
      //sum of hansi-tuned XYSum plasma and peter-tuned polar plasma
      uint16_t color = 0;
      color += plasma_XYSum(mx, my, state->XYSumOffset, 32767);
      color += plasma_Polar(mx, my, state->PolarOffset, 10000, cx, cy);
      color /= 2;
      color += color/2;
      setPixelH(x, y, color);
    }
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
}
