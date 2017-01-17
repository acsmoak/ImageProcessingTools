/*
   Name: Mandy Smoak
   Date: November 6, 2016

   Project: Homework 5
   Filenme: warper.cpp

   This program acts as a warping tool for affine and projective warps. The program reads in 
   user input to build a tranformation matrix. Once the user presses 'd' (done) the program calculates 
   the tranformed image and displays it in an OpenGL display window. If an output file was 
   specified in the command line arguments, the program will write the image to the specified file.
   The program performs image rotation, scaling, translation, shearing, flipping, perspective warping, 
   and twirling based on user input.

*/

#include"OpenImageIO/imageio.h"
OIIO_NAMESPACE_USING

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include<GL/glut.h> 
#endif

#include "matrix.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;
using std::string;


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

static rgbaPixel **image, **transformedImage;
static vector<float> pixels;
int width, height, channels, outputWidth, outputHeight;
const char * inputImage, * outputImage;
bool twirl = false;

// initializes 2D array of type rgbaPixel
void initRGBAPixel2DArray(rgbaPixel ** &p, int w, int h) {
  p = new rgbaPixel * [h];
  p[0] = new rgbaPixel[w*h];
  
  for(int i = 1; i < h; ++i)
    p[i] = p[i-1] + w;
}

// reads the image
void readImage(const char * filename) {

  ImageInput * img = ImageInput::open(filename);
  if (!img) {
    std::cout<<std::endl<<"Read Error: Could not open "<< filename <<std::endl;
    exit(-1);
  }

  ImageSpec imageSpec;

  img->open(filename , imageSpec);

  width = imageSpec.width;
  height = imageSpec.height;
  channels = imageSpec.nchannels;

  initRGBAPixel2DArray(image, width, height);

  pixels.resize(width*height*channels);

  if(!img->read_image(TypeDesc::FLOAT , &pixels[0])) {
    cout<<"Read error: Could not read from "<< filename <<endl;
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


// writes the image when user presses W/w
void writeImage(const char * filename) {

  if(filename == NULL) {
    cout<<"Write error: Filename not provided."<<endl;
    exit(-1);
  }  
  
  ImageOutput * out = ImageOutput::create(filename);
  if (!out) {
    std::cerr << "Write Error: Could not create: " << filename << geterror();
  }

  ImageSpec spec(outputWidth, outputHeight, channels, TypeDesc::FLOAT);
  
  out->open(filename , spec);
  pixels.resize(outputWidth*outputHeight*channels);
  int iterator = 0;
  for(int row = outputHeight-1; row >= 0; --row) {
    for(int col = 0; col < outputWidth; ++ col) {
      pixels[iterator++] = transformedImage[row][col].r;
      pixels[iterator++] = transformedImage[row][col].g;
      pixels[iterator++] = transformedImage[row][col].b;
      if(channels == 4)
        pixels[iterator++] = transformedImage[row][col].a;
    }
  }

  if(out->write_image(TypeDesc::FLOAT, &pixels[0]))
    cout<<"Image written successfully."<<endl;
  else
    cout<<"Write Error: Something went wrong while writing the image. "<<geterror()<<endl;
  out->close();
  delete out;

}

// displays the image
void displayImage() {
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0,0);
  glViewport(0, 0, outputWidth, outputHeight);  
  glDrawPixels(outputWidth, outputHeight, GL_RGBA,GL_FLOAT, transformedImage[0]);
  glutSwapBuffers();
  glFlush();
}

// converts input to lowercase
void toLowerCase(char * c) {

  if(c != NULL) {
    for(int i = 0; c[i] != '\0'; ++i) {
      if(c[i] >= 'A' && c[i] <= 'Z')
        c[i] += ('a' - 'A');
    }
  }
}

// performs rotation of image using theta provided by user
void performRotate(Matrix3D &M , double theta) {

  Matrix3D rotation;
  double angleInRad, cosTheta, sinTheta;

  angleInRad = PI * theta/180.0;
  cosTheta = cos(angleInRad);
  sinTheta = sin(angleInRad);

  // setting up rotation matrix
  rotation[0][0] = cosTheta;
  rotation[0][1] = -sinTheta;
  rotation[1][0] = sinTheta;
  rotation[1][1] = (double)cosTheta;

  Matrix3D result = rotation * M;

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}

// performs scaling of image by using sX and sY provided by user
void performScale(Matrix3D &M, double sX, double sY) {

  Matrix3D scale;

  if(sX == 0.0 || sY == 0.0) {
    cout<<"Error: Scale factor cannot be 0."<<endl;
    return;
  }

  // setting up scaling matrix
  scale[0][0] = sX;
  scale[1][1] = sY;
 
  Matrix3D result = scale * M;

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}

// performs translation of image by using dX and dY provided by user
void performTranslate(Matrix3D &M, double dX, double dY) {

  Matrix3D translate;

  // setting up translation matrix
  translate[0][2] = dX;
  translate[1][2] = dY;
 
  Matrix3D result = translate * M;

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}


// performs shearing on image by using hX and hY provided by user
void performShear(Matrix3D &M, double hX, double hY) {

  Matrix3D shear;

  // setting up shearing matrix
  shear[0][1] = hX;
  shear[1][0] = hY;
 
  Matrix3D result = shear * M;

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}

// adjusts tranform matrix for a flip
void performFlip(Matrix3D &M, int fX, int fY) {
   
  Matrix3D flip;

  // setting up flip matrix
  if (fX == 1)flip[0][0] = -1.0;  // flips over y-axis
  if (fY == 1) flip[1][1] = -1.0;  // flips over x-axis

  Matrix3D result = flip * M;
  
  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}

// adjusts tranform matrix for a perspective
void performPerspective(Matrix3D &M, double pX, double pY) {
  
  Matrix3D perspective;
 
  // setting up perspective matrix
  perspective[2][0] = pX;
  perspective[2][1] = pY;

  Matrix3D result = perspective * M;

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}

// (x,y) <=> (u,v)
void mapPixelsToTransformedImage(double &u, double &v, int &row, int &col) {
  if(u >= 0 && u < width && v >= 0 && v < height) {
    if(round(v) > height-1) v = height-1;
    else v = round(v);
    if(round(u) > width-1) u = width-1;
    else u = round(u);
    transformedImage[row][col] = image[(int)v][(int)u];
  }
  else {
    transformedImage[row][col].r = 0.0;
    transformedImage[row][col].g = 0.0;
    transformedImage[row][col].b = 0.0;
  }
}


void performAffineTransform(Matrix3D &M) {

  // four corners of the original image  
  Vector2D inputLeftBottom;  
  inputLeftBottom.x = 0.0;
  inputLeftBottom.y = 0.0;
 
  Vector2D inputLeftTop;
  inputLeftTop.x = 0.0;
  inputLeftTop.y = double(height)-1;
  
  Vector2D inputRightBottom;  
  inputRightBottom.x = double(width)-1;
  inputRightBottom.y = 0.0;
  
  Vector2D inputRightTop;
  inputRightTop.x = double(width)-1; 
  inputRightTop.y = double(height)-1.0; 

  // mapping four corners of transformed image
  Vector2D outputLeftBottom = M * inputLeftBottom;
  Vector2D outputLeftTop = M * inputLeftTop;
  Vector2D outputRightBottom = M * inputRightBottom;
  Vector2D outputRightTop = M * inputRightTop;

  // calculating width and height of transformed image
  outputWidth = std::max(outputLeftBottom.x ,
                std::max(outputLeftTop.x ,
                std::max(outputRightBottom.x, outputRightTop.x))) -
                std::min(outputLeftBottom.x,
                std::min(outputLeftTop.x ,
                std::min(outputRightBottom.x, outputRightTop.x))); 
  
  outputHeight = std::max(outputLeftBottom.y ,
                 std::max(outputLeftTop.y ,
                 std::max(outputRightBottom.y, outputRightTop.y))) -
                 std::min(outputLeftBottom.y,
                 std::min(outputLeftTop.y,
                 std::min(outputRightBottom.y, outputRightTop.y)));

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  double x = std::min(outputLeftBottom.x,
            std::min(outputLeftTop.x,
            std::min(outputRightBottom.x, outputRightTop.x)));

  // getting y co-ordinate of origin of transformed image
  double y = std::min(outputLeftBottom.y,
            std::min(outputLeftTop.y,
            std::min(outputRightBottom.y, outputRightTop.y)));

  Matrix3D origin;
  origin[0][0] = x;
  origin[1][0] = y;
  origin[2][0] = 0.0;
  origin[1][1] = 0.0;
  origin[2][2] = 0.0;
  
  Matrix3D inverseOfM = M.inverse();

  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {

      Matrix3D outputPixel;
      outputPixel[0][0] = col;
      outputPixel[1][0] = row;
      outputPixel[2][0] = 1.0;
      outputPixel[1][1] = 0.0;
      outputPixel[2][2] = 0.0;
      
      // account for change in origin in transformation
      outputPixel[0][0] = outputPixel[0][0] + origin[0][0];
      outputPixel[1][0] = outputPixel[1][0] + origin[1][0];
      outputPixel[2][0] = outputPixel[2][0] + origin[2][0];
      
      Matrix3D inputPixel; 
      inputPixel = inverseOfM * outputPixel;

      double u = inputPixel[0][0]/inputPixel[2][0];
      double v = inputPixel[1][0]/inputPixel[2][0];

      mapPixelsToTransformedImage(u, v, row, col);

    }
  }
}

// performs twirling of image using s, cX and cY provided by user
void performTwirl(double strength, double cX, double cY) {

  // dimensions of transformed image would be same as original image
  Vector2D outputLeftBottom;  
  outputLeftBottom.x = 0.0;
  outputLeftBottom.y = 0.0;
 
  Vector2D outputLeftTop;
  outputLeftTop.x = 0.0;
  outputLeftTop.y = double(height)-1;
  
  Vector2D outputRightBottom;  
  outputRightBottom.x = double(width)-1;
  outputRightBottom.y = 0.0;
  
  Vector2D outputRightTop;
  outputRightTop.x = double(width)-1; 
  outputRightTop.y = double(height)-1.0; 

  outputWidth = width;
  outputHeight = height;

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  double x = std::min(outputLeftBottom.x,
            std::min(outputLeftTop.x,
            std::min(outputRightBottom.x, outputRightTop.x)));

  // getting y co-ordinate of origin of transformed image
  double y = std::min(outputLeftBottom.y,
            std::min(outputLeftTop.y,
            std::min(outputRightBottom.y, outputRightTop.y)));

  Matrix3D origin;
  origin[0][0] = x;
  origin[1][0] = y;
  origin[2][0] = 0.0;
  origin[1][1] = 0.0;
  origin[2][2] = 0.0;

  int centerX, centerY;
  centerX = (int)(cX * outputWidth);
  centerY = (int)(cY * outputHeight);
  double minDim = std::min(outputWidth, outputHeight);
  double dstX, dstY, r, angle;

  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {
      dstX = col - centerX;
      dstY = row - centerY;

      // calculating distance between pixel and center co-ordinate
      r = sqrt(dstX*dstX + dstY*dstY);

      // calculating angle of rotation between pixel and center co-ordinate
      angle = atan2(dstY , dstX);

      Matrix3D outputPixel;
      outputPixel[0][0] = col;
      outputPixel[1][0] = row;
      outputPixel[2][0] = 1.0;
      outputPixel[1][1] = 0.0;
      outputPixel[2][2] = 0.0;

      // account for change in origin in transformation
      outputPixel[0][0] = outputPixel[0][0] + origin[0][0];
      outputPixel[1][0] = outputPixel[1][0] + origin[1][0];
      outputPixel[2][0] = outputPixel[2][0] + origin[2][0];

      double u = r * cos(angle + strength * (r - minDim)/minDim) + centerX;
      double v = r * sin(angle + strength * (r - minDim)/minDim) + centerY;

      mapPixelsToTransformedImage(u, v, row, col);

    }
  }
}

void displayAffTransfUsage() {
  cout<<"Commands: 'r' -- rotate, 's' -- scale, 't' -- translate"<<endl;
  cout<<"          'h' -- shear, 'f' -- flip, 'p' -- perspective warp"<<endl;
  cout<<"          'n' -- twirl and 'd' -> done."<<endl;
  cout<<"You may use multiple commands except for 'n'."<<endl;
  cout<<"Enter 'd' command to indicate that you are done."<<endl;
  cout<<"Enter 'w' command to write the image to a file."<<endl;
}

void processInput(Matrix3D &M) {
  char command[1024];
  bool done;
  double theta, sX, sY, dX, dY, hX, hY, fX, fY, pX, pY, cX, cY, s;
  
  displayAffTransfUsage();

  for(done = false; !done;) {
    cout<<"> ";
    cin>>command;
    toLowerCase(command);
    
    if(strcmp(command, "d") == 0) done = true;
    else if(strlen(command) != 1) displayAffTransfUsage();
    else {
      switch(command[0]) {
        case 'r':
          cout<<"Rotating: Please enter the theta value: "<<endl;
          cin >> theta;
          performRotate(M , theta);
          break;
        case 's':
          cout<<"Scaling: Please enter the scaling factors sX and sY:"<<endl;
          cin>>sX>>sY;
          performScale(M, sX, sY);
          break;
        case 't':
          cout<<"Translating: Please enter the translating factors dX and dY:"<<endl;
          cin>>dX>>dY;
          performTranslate(M, dX, dY);
          break;
        case 'h':
          cout<<"Shearing: Please enter the shearing factors hX and hY:"<<endl;
          cin>>hX>>hY;
          performShear(M, hX, hY);
          break;
        case 'f':
          cout<<"Flipping: Please enter the flip factors fX and fY:"<<endl;
          cin>>fX>>fY;
          performFlip(M, fX, fY);
          break;  
        case 'p':
          cout<<"Perspective: Please enter the perspective factors pX and pY:"<<endl;
          cin>>pX>>pY;
          performPerspective(M, pX, pY);
          break;                  
        case 'n':
          done = true;
          twirl = true;
          cout<<"Twirl: Please enter the twirl factors cX, cY, and strength:"<<endl;
          cin>>cX>>cY>>s;
          performTwirl(s, cX, cY);
          break;
        case 'd':
          done = true;
          break;
        default:
          displayAffTransfUsage();
          break; 
      }
    } 
  }
}

void handleKey(unsigned char key, int x, int y) {
  switch(key) {
    case 'w':
    case 'W':
      writeImage(outputImage);
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

void displayError() {
  cout<<"Usage:"<<endl;
  cout<<"./warper input.img [output.img]"<<endl;
}

int main(int argc , char * argv[]) {

  if(argc < 2) {
    displayError();
    return -1;
  }
  
  inputImage = argv[1];
  readImage(inputImage);

  if(argc == 3) {
    outputImage = argv[2];
    cout<<"Press W to write the displayed image."<<endl;
  }

  Matrix3D M;
  processInput(M);

  if(!twirl) performAffineTransform(M);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(outputWidth, outputHeight);
  glutCreateWindow("Warping");

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, outputWidth, 0, outputHeight);

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glutMainLoop();
 
  return 0;

}


