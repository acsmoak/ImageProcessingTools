/*
  Name: Mandy Smoak
  Date: October 4, 2016

  Project: Homework 3
  Filename: composite.cpp

  This application reads in two color images and their alpha channels, and displays their 
  composite (A over B). If the alpha channel is missing for the second image, the program 
  assumes that the image is a background image (i.e. an image that is completely opaque). 
  The program displays the composite of A and B, and allows the user to write to an optional 
  third file containing the newly composited image.

  Usage: README
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

// crates pixel structure
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

static rgbaPixel **images[2], **compositeImage;
static vector<float> pixels;
int width[2], height[2], channels[2], background, foreground;
const char * outputImage;


// reads the image
void readImage(const char * filename , int index) {

  ImageInput * img = ImageInput::open(filename);
  if (!img) {
    std::cout<<std::endl<<"Filename is invalid."<<std::endl;
    exit(-1);
  }

  ImageSpec imageSpec;

  img->open(filename , imageSpec);

  width[index] = imageSpec.width;
  height[index] = imageSpec.height;
  channels[index] = imageSpec.nchannels;

  images[index] = new rgbaPixel * [height[index]];
  images[index][0] = new rgbaPixel [width[index]*height[index]];

  for(int i = 1; i < height[index]; ++i) 
    images[index][i] = images[index][i-1] + width[index];

  pixels.resize(width[index]*height[index]*channels[index]);

  if(!img->read_image(TypeDesc::FLOAT , &pixels[0])) {
    cout<<"Read error. Could not read from the file "<< filename <<endl;
    cout<< img->geterror() <<endl;
    delete img;
    exit(-1);
  }
  
  for(int row = 0; row < height[index]; ++row) {
    for(int col = 0; col < width[index]; ++col) {

      images[index][height[index]-row-1][col].r = pixels[(row*width[index]+col)*channels[index] + 0];
      images[index][height[index]-row-1][col].g = pixels[(row*width[index]+col)*channels[index] + 1];
      images[index][height[index]-row-1][col].b = pixels[(row*width[index]+col)*channels[index] + 2];

      if(channels[index] == 3)
        images[index][height[index]-row-1][col].a = 1.0;
      else
        images[index][height[index]-row-1][col].a = pixels[(row*width[index]+col)*channels[index] + 3];
    }
  }
  img->close();
  delete img;
}


// performs A over B operation
void composite() {
  
  float alpha;
  
  int startX = 0, startY = 0; 

  compositeImage = new rgbaPixel * [height[background]];
  compositeImage[0] = new rgbaPixel [width[background]*height[background]];

  for(int i = 1; i < height[background]; ++i)
    compositeImage[i] = compositeImage[i-1] + width[background];

  for(int row = 0; (startY+row) < height[background] && row < height[foreground]; ++row) {
    for(int col = 0; (startX+col) < width[background] && col < width[foreground] ; ++col) {

      alpha = images[foreground][row][col].a;

      // avoids errors if height and width of both images are different
      int x = startX + col;
      int y = startY + row;

      compositeImage[row][col].r = alpha*images[foreground][row][col].r + ((1-alpha)*images[background][y][x].r);
      compositeImage[row][col].g = alpha*images[foreground][row][col].g + ((1-alpha)*images[background][y][x].g);
      compositeImage[row][col].b = alpha*images[foreground][row][col].b + ((1-alpha)*images[background][y][x].b);
      compositeImage[row][col].a = alpha*images[foreground][row][col].a + ((1-alpha)*images[background][y][x].a); 

    }
  }

}


// writes the image when user presses W/w
void writeImage() {
  
  pixels.resize(width[background]*height[background]*4);

  // flips the image for writing
  int iterator = 0;
  for( int i = height[background]-1; i >= 0 ; i--) {
    for(int j = 0; j < width[background] ; j++) {
      pixels[iterator++] = compositeImage[i][j].r;
      pixels[iterator++] = compositeImage[i][j].g;
      pixels[iterator++] = compositeImage[i][j].b;
      pixels[iterator++] = compositeImage[i][j].a;
    }
  }

  ImageOutput *out = ImageOutput::create(outputImage);

  ImageSpec spec (width[background], height[background], 4, TypeDesc::FLOAT);

  out->open(outputImage , spec);
  if(out->write_image(TypeDesc::FLOAT, &pixels[0]))
    cout<<"Image written successfully."<<endl;
  else 
    cout<<"Something went wrong while writing the image. "<<geterror()<<endl;
  out->close();

}

// displays the composite image
void displayImage() {
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, width[background], height[background]);
  glRasterPos2i(0,0);
  glDrawPixels(width[background], height[background], GL_RGBA,GL_FLOAT, compositeImage[0]);
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

  if(argc < 3) {
    cout<<"You need to specify at least two valid filenames for composting."<<endl;
    return -1;
  } 

  // stores first image in images[0] and second image in images[1]
  readImage(argv[1] , 0);
  readImage(argv[2] , 1);

  // decides background and foreground images depending on number of channels
  if(channels[0] == 3 and channels[1] == 4) {
    background = 0;
    foreground = 1;
    composite(); 
  }
  else if(channels[1] == 3 and channels[0] == 4) {
    background = 1;
    foreground = 0;
    composite(); 
  }
  else {
    cout<<"You need to specify one image with three channels and one with four channels."<<endl;
    return -1;
  }

  if(argc == 4) {
    cout<<"Press W to write the composite image."<<endl;
    outputImage = argv[3];
  }

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(width[background], height[background]);
  glutCreateWindow("Composite Image");

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width[background], 0, height[background]);
  glClearColor(1,1,1,1);

  glutMainLoop();

 
  return 0;
}
