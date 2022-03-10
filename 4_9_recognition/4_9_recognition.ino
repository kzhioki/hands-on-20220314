#include <Camera.h>
#include <SDHCI.h>
#include <DNNRT.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_RST 8
#define TFT_DC  9
#define TFT_CS  10

Adafruit_ILI9341 tft
   = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

SDClass SD;
DNNRT dnnrt;	// DNNRT object

DNNVariable input(28*28);	// Object for DNNRT Input data.

// Image overlay
void overlap_image(CamImage &base, int left_x, int top_y, CamImage &img)
{
  int base_w = base.getWidth();
  int img_w  = img.getWidth();
  int img_h  = img.getHeight();
  uint8_t *base_data = base.getImgBuff();
  uint8_t *img_data = img.getImgBuff();
  float *input_buffer;

  input_buffer = input.data();
  base_data += (top_y * base_w + left_x) * 2;
    
  for (int h=0; h<img_h; h++, base_data += (base_w - img_w) * 2)
  {
    for (int w=0; w<img_w; w++)
    {
      // YUV data order is like Cb0/Y0/Cr0/Y1/Cb1/Y2/Cr1/Y3 ...,
      // Gray scale data has no color, so input 128 as zero color.
      *base_data++ = 128;       // Cb or Cr
      *base_data++ = *img_data; // Y
      *input_buffer++ = (float)*img_data++;   // Do make DNNRT input data in this loop.
    }
  }
}

/* Call back function of Camera */
void CamCB(CamImage img)
{
  if (img.isAvailable())
  {
    CamImage small;
    
    // Scale down the image to 28x28 image.
    img.clipAndResizeImageByHW(small, 47,7, 270, 230, 28,28);
    small.convertPixFormat(CAM_IMAGE_PIX_FMT_GRAY);     // Convert gray scale color.

    // Overlay gray image on the original image.
    overlap_image(img, 10, 10, small);

    dnnrt.inputVariable(input, 0);      // Input the scaled data into DNNRT.
    dnnrt.forward();                    // Do forward() with loaded neural network.
    DNNVariable output = dnnrt.outputVariable(0);       // Get the result.

    int index = output.maxIndex();      // Get the highest probability index.
    switch (index)
    {
      case 0:
        Serial.println("**** Four");
        break;
      case 1:
        Serial.println("********* Nine");
        break;
      case 2:
        Serial.println("Non");
        break;
    }
    
    img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
    tft.drawRGBBitmap(0, 0, (uint16_t *)img.getImgBuff(), 320, 240);
    tft.drawRect(47,7, 224, 224, tft.color565(255,0,0));
  }
}

/* setup() of Arduino */
void setup() {
  Serial.begin(115200);
  SD.begin();
  SD.beginUsbMsc();

  // Open the result.nnb
  File nnbfile("result.nnb");
  if (!nnbfile) {
    Serial.println("nnb not found..."); while(1);
  }

  // Initialize DNNRT
  if (dnnrt.begin(nnbfile) < 0) {
    Serial.println("DNNRT initialze error."); while(1);
  }

  Serial.println("DNNRT initialzed");
  
  tft.begin();
  tft.setRotation(3);

  theCamera.begin();
  theCamera.startStreaming(true, CamCB);

  theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_DAYLIGHT);
}

/* loop() of Arduino */
void loop() {
  sleep(1);
}
