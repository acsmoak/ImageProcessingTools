/*
  Name: Mandy Smoak
  Date: October 4, 2016

  Project: Homework 3
  Filename: alphamask.cpp

  This application performs masking on an image based on the the HSV value. The program reads 
  in an input image, converts the image from the RGB color system to the HSV color system, 
  and computes an alpha value for each pixel based on the hue, saturation, and value of the pixel.
  Additionally, it performs spill surpression.
*/


#include"OpenImageIO/imageio.h"
OIIO_NAMESPACE_USING

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#include<GL/glut.h>
#endif

#include<iostream>
using namespace std;

// creates pixel structure
class rgbaPixel {
public:
  float r,g,b,a;

  float& operator[] (const unsigned int index) {
    if (index == 0) return r; 
    else if(index == 1) return g;
    else if (index == 2) return b;
    else if (index == 3) return a;
    else {
      cout<<"Invalid index."<<endl;
      exit(-2);
    }
  }
};

static rgbaPixel **image, **maskedImage;
static vector<float> pixels;
int width, height, channels;
const char * outputImage;

// reads the image
void readImage(const char * filename) {

  ImageInput * img = ImageInput::open(filename);
  if (!img) {
    std::cout<<std::endl<<"Filename is invalid."<<std::endl;
    exit(-1);
  }

  ImageSpec imageSpec;
  
  img->open(filename , imageSpec);

  width = imageSpec.width;
  height = imageSpec.height;
  channels = imageSpec.nchannels;

  image = new rgbaPixel * [height];
  image[0] = new rgbaPixel [width*height];

  for(int i = 1; i < height; ++i) 
    image[i] = image[i-1] + width;

  pixels.resize(width * height * channels);

  if(!img->read_image(TypeDesc::FLOAT , &pixels[0])) {
    cout<<"Read error. Could not read from the file "<< filename <<endl;
    cout<< img->geterror() <<endl;
    delete img;
    exit(-1);
  }
  
  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      image[height-row-1][col].r = pixels[(row*width+col)*channels + 0];
      image[height-row-1][col].g = pixels[(row*width+col)*channels + 1];
      image[height-row-1][col].b = pixels[(row*width+col)*channels + 2];

      if(channels == 3)
        image[height-row-1][col].a = 1.0;
      else
        image[height-row-1][col].a = pixels[(row*width+col)*channels + 3];
    }
  }
  img->close();
  delete img;
}

// converts RGB values to HSV values
void RGBtoHSV(double red, double green, double blue, double & h, double & s, double & v){

  double max, min, delta;

  max = std::max(red, std::max(green, blue));
  min = std::min(red, std::min(green, blue));

  v = max;  // value is maximum of r, g, b

  if (max == 0) {  // saturation and hue 0 if value is 0
    s = 0; 
    h = 0; 
  } 
  else { 
    s = (max - min)/max;  // saturation is color purity on scale 0 - 1

    delta = max - min; 
    if (delta == 0)  // hue doesn't matter if saturation is 0 
      h = 0; 
    else { 
      if(red == max) {  // otherwise, determine hue on scale 0 - 360
        h = (green - blue)/delta; 
      } 
      else if(green == max) {
        h = 2.0 + (blue - red)/delta; 
      } 
      else {  // (blue == max) 
        h = 4.0 + (red - green)/delta; 
      }
      h = h*60.0; 
      if (h < 0) 
        h = h + 360.0; 
    } 
  } 
}

// performs masking
void mask(int choice) {

  maskedImage = new rgbaPixel * [height];
  maskedImage[0] = new rgbaPixel[width*height];

  for(int i = 1; i < height; ++i)
    maskedImage[i] = maskedImage[i-1] + width;

  double h, s, v;

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {
      
      RGBtoHSV(image[row][col].r, image[row][col].g, image[row][col].b, h, s, v);

      // alphamasking using h,s and v values
      if((h > 90 and h < 150) and (s > 0.01 and s < 1.00001) and (v > 0.01 and v < 1.00001))
        maskedImage[row][col].a = 0.0;
      else maskedImage[row][col].a = 1.0;

      // pre-multiplying each color channel
      maskedImage[row][col].r = maskedImage[row][col].a*image[row][col].r;
      
      //supressing green spill if -ss is inputted
      if(choice == 2) {
        maskedImage[row][col].g = maskedImage[row][col].a*(std::min(image[row][col].r, std::min(image[row][col].g, image[row][col].b)));
      }
      else 
        maskedImage[row][col].g = maskedImage[row][col].a*image[row][col].g;

      maskedImage[row][col].b = maskedImage[row][col].a*image[row][col].b;
    }
  }
}

// writes the image when user presses W/w
void writeImage() {
  
  pixels.resize(width*height*4);
 
  // flips the image for writing
  int iterator = 0;
  for( int i = height-1; i >= 0 ; i--) {
    for(int j = 0; j < width ; j++) {
      pixels[iterator++] = maskedImage[i][j].r;
      pixels[iterator++] = maskedImage[i][j].g;
      pixels[iterator++] = maskedImage[i][j].b;
      pixels[iterator++] = maskedImage[i][j].a;
    }
  }

  ImageOutput *out = ImageOutput::create(outputImage);

  ImageSpec spec (width, height, 4, TypeDesc::FLOAT);

  out->open(outputImage , spec);
  if(out->write_image(TypeDesc::FLOAT, &pixels[0]))
    cout<<"Image written successfully."<<endl;
  else 
    cout<<"Something went wrong while writing the image. "<<geterror()<<endl;
  out->close();

}

// displays the masked image
void displayImage() {
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, width, height);
  glRasterPos2i(0,0);
  glDrawPixels(width, height, GL_RGBA,GL_FLOAT, maskedImage[0]);
  glFlush();
}

// decides what to do when keyboard key is hit
void handleKey(unsigned char key, int x, int y) {
  switch(key) {
    case 'w':
    case 'W':
      if(outputImage) writeImage();
      else cout<<"Output file not specified."<<endl;
      break;
    case 'q':
    case 'Q':
    case 27 :
      exit(0);
      break;
    default:
      return;
      
  }
}


int main(int argc , char ** argv) {
  int option = 1;
  if(argc < 2) {
    cout<<"You need to specify at least one valid filename for alphamasking."<<endl;
    return -1;  
  }


// decide what to do depending on the parameters passed by user from command-line.
  if(argc == 2) {
    // no -ss flag given by user
    readImage(argv[1]);
  }
  else if(argc == 3) {
    string s1 = argv[1];
    if(s1 == "-ss") { // only spill supression (no writing)
      option = 2;
      readImage(argv[2]);
    }
    else { // writing (no spill supression)
      readImage(argv[1]);
      outputImage = argv[2];
      cout<<"Press W to write the image."<<endl;
    } 
  }
  else if(argc == 4) {
    string s2 = argv[1];
    string s3 = argv[2];
    if(s2 == "-ss") { // spill supression and writing
      option = 2;
      readImage(argv[2]);
      outputImage = argv[3];
      cout<<"Press W to write the image."<<endl;
    }   
  }

  mask(option);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("Alphamasked Image");

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);
  glClearColor(1,1,1,1);

  glutMainLoop();

  return 0;
};
        
