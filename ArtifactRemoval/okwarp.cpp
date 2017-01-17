/*
   Name: Mandy Smoak
   Date: November 15, 2016 

   Project: Homework 6
   Filenme: okwarp.cpp

   This program performs a perspective warp and twirl on an image specified by the user. 
   It then repairs the warped image by removing magnification artifacts (using bilinear interpolation), 
   or can both magnification and minification artifacts (using supersampling). Once the image is repaired, 
   the user can toggle between the repaired and the original. 
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
#include <math.h>

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
bool twirl = false, bilinear = false, jittered = false;
float pX, pY, cX, cY, s;
int centerX, centerY, totalSamples = 16;
float dstX, dstY, r, angle, minDim;

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

// performs perspective warping on image by using pX and pY provided by user
void performPerspective(Matrix3D &M, float pX, float pY) {
  
  Matrix3D perspective;
 
  // setting up perspective matrix
  perspective[2][0] = pX;
  perspective[2][1] = pY;

  Matrix3D result = perspective * M;

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = result[row][col];
}

/*
  Routine to inverse map (x, y) output image spatial coordinates
  into (u, v) input image spatial coordinates

  Call routine with (x, y) spatial coordinates in the output
  image. Returns (u, v) spatial coordinates in the input image,
  after applying the inverse map. Note: (u, v) and (x, y) are not 
  rounded to integers, since they are true spatial coordinates.
 
  inwidth and inheight are the input image dimensions
  outwidth and outheight are the output image dimensions
*/
void inv_map(float x, float y, float &u, float &v, int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = sqrt(x);			        // inverse in x direction is sqrt
  v = 0.5 * (1 + sin(y * PI));  // inverse in y direction is offset sine

  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
}
/* 

Initiates the inverse map to cyle through each pixel in input image. Doesn't work...

void initiateInverseMap(Matrix3D &M) {
  
  float u, v;
  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {
	  row = (float) row;
	  col = (float) col; 	
      inv_map(row, col, u, v, width, height, width, height);
    }
  }
  glRasterPos2i(0,0);
  glDrawPixels(outputWidth, outputHeight, GL_RGBA, GL_FLOAT, transformedImage[0]);
  glutSwapBuffers();
}

*/

// (x,y) <=> (u,v)
void mapPixelsToTransformedImage(float &u, float &v, int &row, int &col) {
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


// removes magnification artifacts using bilinear interpolation
void removeMagnificationArtifacts(float &u, float &v, int &row, int &col) { 
  int j,k;
  float a, b;

  j = floor(u);
  k = floor(v);

  // using horizontal and vertical offsets in u and v space 
  a = u-j;
  b = v-k;

  Matrix3D v1, v2;
  
  v1[0][0] = 1-b;
  v1[0][1] = b;
  v1[1][1] = 0;
  v1[2][2] = 0;
  
  v2[0][0] = 1-a;
  v2[1][0] = a;
  v2[1][1] = 0;
  v2[2][2] = 0;  
  
  if(j >=0 && j < width-1 && k >= 0 && k < height-1) {
    for(int i = 0; i < 3; ++i) {

      // taking four nearest neighbors 
      Matrix3D fourNeighbors;
    
      fourNeighbors[0][0] = image[k][j][i];
      fourNeighbors[0][1] = image[k][j+1][i];
      fourNeighbors[1][0] = image[k+1][j][i];
      fourNeighbors[1][1] = image[k+1][j+1][i];
      fourNeighbors[2][2] = 0;
      
      Matrix3D productMatrix = v1 * fourNeighbors * v2;
      transformedImage[row][col][i] = productMatrix[0][0];
    }
  }
  else {
    transformedImage[row][col].r = 0.0;
    transformedImage[row][col].g = 0.0;
    transformedImage[row][col].b = 0.0;
  }
}


void performAffineTransform(Matrix3D &M) {

  // four corners of the original image  
  Matrix3D inputLeftBottom;  
  inputLeftBottom[0][0] = 0.0;
  inputLeftBottom[1][0] = 0.0;
  inputLeftBottom[2][0] = 1.0;
  inputLeftBottom[1][1] = 0.0;
  inputLeftBottom[2][2] = 0.0;
 
  Matrix3D inputLeftTop;
  inputLeftTop[0][0] = 0.0;
  inputLeftTop[1][0] = float(height)-1;
  inputLeftTop[2][0] = 1.0;
  inputLeftTop[1][1] = 0.0;
  inputLeftTop[2][2] = 0.0;
    
  Matrix3D inputRightBottom;  
  inputRightBottom[0][0] = float(width)-1;
  inputRightBottom[1][0] = 0.0;
  inputRightBottom[2][0] = 1.0;
  inputRightBottom[1][1] = 0.0;
  inputRightBottom[2][2] = 0.0;
    
  Matrix3D inputRightTop;
  inputRightTop[0][0] = float(width)-1; 
  inputRightTop[1][0] = float(height)-1; 
  inputRightTop[2][0] = 1.0;
  inputRightTop[1][1] = 0.0;
  inputRightTop[2][2] = 0.0;
  
  // mapping four corners of transformed image
  Matrix3D outputLeftBottom = M * inputLeftBottom;
  Matrix3D outputLeftTop = M * inputLeftTop;
  Matrix3D outputRightBottom = M * inputRightBottom;
  Matrix3D outputRightTop = M * inputRightTop;

  // normalizing w component
  if(outputLeftBottom[2][0] != 1) {
    outputLeftBottom[0][0] /= outputLeftBottom[2][0];
    outputLeftBottom[1][0] /= outputLeftBottom[2][0]; 
  }
  if(outputLeftTop[2][0] != 1) {
    outputLeftTop[0][0] /= outputLeftTop[2][0];
    outputLeftTop[1][0] /= outputLeftTop[2][0];
  }
  if(outputRightBottom[2][0] != 1) {
    outputRightBottom[0][0] /= outputRightBottom[2][0];
    outputRightBottom[1][0] /= outputRightBottom[2][0];
  }
  if(outputRightTop[2][0] != 1) {
    outputRightTop[0][0] /= outputRightTop[2][0];
    outputRightTop[1][0] /= outputRightTop[2][0];
}

  // calculating width and height of transformed image
  outputWidth = std::max(outputLeftBottom[0][0] ,
                std::max(outputLeftTop[0][0] ,
                std::max(outputRightBottom[0][0], outputRightTop[0][0]))) -
                std::min(outputLeftBottom[0][0],
                std::min(outputLeftTop[0][0] ,
                std::min(outputRightBottom[0][0], outputRightTop[0][0]))); 
  
  outputHeight = std::max(outputLeftBottom[1][0] ,
                 std::max(outputLeftTop[1][0] ,
                 std::max(outputRightBottom[1][0], outputRightTop[1][0]))) -
                 std::min(outputLeftBottom[1][0],
                 std::min(outputLeftTop[1][0],
                 std::min(outputRightBottom[1][0], outputRightTop[1][0])));

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  float x = std::min(outputLeftBottom[0][0],
            std::min(outputLeftTop[0][0],
            std::min(outputRightBottom[0][0], outputRightTop[0][0])));

  // getting y co-ordinate of origin of transformed image
  float y = std::min(outputLeftBottom[1][0],
            std::min(outputLeftTop[1][0],
            std::min(outputRightBottom[1][0], outputRightTop[1][0])));

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

      float u = inputPixel[0][0]/inputPixel[2][0];
      float v = inputPixel[1][0]/inputPixel[2][0];


      if(!bilinear) mapPixelsToTransformedImage(u, v, row, col); // just perform the transformation
      else removeMagnificationArtifacts(u, v, row, col); // perform transformation and remove magnification artificats

    }
  }
}


void removeBothArtifacts(Matrix3D &M) {

  // four corners of the original image  
  Matrix3D inputLeftBottom;  
  inputLeftBottom[0][0] = 0.0;
  inputLeftBottom[1][0] = 0.0;
  inputLeftBottom[2][0] = 1.0;
  inputLeftBottom[1][1] = 0.0;
  inputLeftBottom[2][2] = 0.0;
 
  Matrix3D inputLeftTop;
  inputLeftTop[0][0] = 0.0;
  inputLeftTop[1][0] = float(height)-1;
  inputLeftTop[2][0] = 1.0;
  inputLeftTop[1][1] = 0.0;
  inputLeftTop[2][2] = 0.0;
    
  Matrix3D inputRightBottom;  
  inputRightBottom[0][0] = float(width)-1;
  inputRightBottom[1][0] = 0.0;
  inputRightBottom[2][0] = 1.0;  
  inputRightBottom[1][1] = 0.0;
  inputRightBottom[2][2] = 0.0;
    
  Matrix3D inputRightTop;
  inputRightTop[0][0] = float(width)-1.0; 
  inputRightTop[1][0] = float(height)-1.0; 
  inputRightTop[2][0] = 1.0;  
  inputRightTop[1][1] = 0.0;
  inputRightTop[2][2] = 0.0;
  
  // mapping four corners of transformed image
  Matrix3D outputLeftBottom = M * inputLeftBottom;
  Matrix3D outputLeftTop = M * inputLeftTop;
  Matrix3D outputRightBottom = M * inputRightBottom;
  Matrix3D outputRightTop = M * inputRightTop;

  // normalizing w component
  if(outputLeftBottom[2][0] != 1.0) {
    outputLeftBottom[0][0] /= outputLeftBottom[2][0];
    outputLeftBottom[1][0] /= outputLeftBottom[2][0]; 
  }
  if(outputLeftTop[2][0] != 1.0) {
    outputLeftTop[0][0] /= outputLeftTop[2][0];
    outputLeftTop[1][0] /= outputLeftTop[2][0];
  }
  if(outputRightBottom[2][0] != 1.0) {
    outputRightBottom[0][0] /= outputRightBottom[2][0];
    outputRightBottom[1][0] /= outputRightBottom[2][0];
  }
  if(outputRightTop[2][0] != 1.0) {
    outputRightTop[0][0] /= outputRightTop[2][0];
    outputRightTop[1][0] /= outputRightTop[2][0];
}

  // calculating width and height of transformed image
  outputWidth = std::max(outputLeftBottom[0][0] ,
                std::max(outputLeftTop[0][0] ,
                std::max(outputRightBottom[0][0], outputRightTop[0][0]))) -
                std::min(outputLeftBottom[0][0],
                std::min(outputLeftTop[0][0] ,
                std::min(outputRightBottom[0][0], outputRightTop[0][0]))); 
  
  outputHeight = std::max(outputLeftBottom[1][0] ,
                 std::max(outputLeftTop[1][0] ,
                 std::max(outputRightBottom[1][0], outputRightTop[1][0])))                  -
                 std::min(outputLeftBottom[1][0],
                 std::min(outputLeftTop[1][0],
                 std::min(outputRightBottom[1][0], outputRightTop[1][0])));

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  float x = std::min(outputLeftBottom[0][0],
            std::min(outputLeftTop[0][0],
            std::min(outputRightBottom[0][0], outputRightTop[0][0])));

  // getting y co-ordinate of origin of transformed image
  float y = std::min(outputLeftBottom[1][0],
            std::min(outputLeftTop[1][0],
            std::min(outputRightBottom[1][0], outputRightTop[1][0])));

  Matrix3D origin;
  origin[0][0] = x;
  origin[1][0] = y;
  origin[1][1] = 0.0;
  origin[2][2] = 0.0;
  
  Matrix3D inverseOfM = M.inverse();

  float *vecOfX, *vecOfY, subOfX, subOfY, a, b, rgb[3] = {};
  int cntOfX = 0, cntOfY = 0, j, k;
  bool carryOn = false;

  vecOfX = new float[totalSamples];
  vecOfY = new float[totalSamples];
  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {

      cntOfX = 0;
      cntOfY = 0;
      rgb[0] = 0.0;
      rgb[1] = 0.0;
      rgb[2] = 0.0;

      for(int i = 0; i < totalSamples; ++i) {
        do {
          carryOn = false;
          float ry = ((float)rand()/(RAND_MAX));
          for(int j = 0; j < cntOfY; ++j) {
            if(vecOfY[j] == ry) carryOn = true;
          }
          if(!carryOn) vecOfY[cntOfY++] = ry - 0.5;
        } while(carryOn);
        do {
          carryOn = false;
          float rx = ((float)rand()/(RAND_MAX));
          for(int j = 0; j < cntOfX; ++j) {
            if(vecOfX[j] == rx) carryOn = true;
          }
          if(!carryOn) vecOfX[cntOfX++] = rx - 0.5;
        } while(carryOn);
      }

      for(int i = 0; i < totalSamples; ++i) {
        subOfY = row - vecOfY[i];
        subOfX = col - vecOfX[i];

        Matrix3D outputPixel;
        outputPixel[0][0] = subOfX;
        outputPixel[1][0] = subOfY;
        outputPixel[2][0] = 1.0;
        outputPixel[1][1] = 0.0;
        outputPixel[2][2] = 0.0;
  
        // account for change in origin in transformation
	 	outputPixel[0][0] = outputPixel[0][0] + origin[0][0];
		outputPixel[1][0] = outputPixel[1][0] + origin[1][0];
		outputPixel[2][0] = outputPixel[2][0] + origin[2][0];
         
        float u, v; 
        if(!twirl) { // perspective
					  	
          Matrix3D inputPixel = inverseOfM * outputPixel;

          u = inputPixel[0][0]/inputPixel[2][0];
          v = inputPixel[1][0]/inputPixel[2][0];
        }
        else { // twirl
          dstX = subOfX - centerX; 
          dstY = subOfY - centerY;
          r = sqrt(dstX*dstX + dstY*dstY);
          angle = atan2(dstY, dstX);
          u = r * cos(angle + s * (r - minDim)/minDim) + centerX;
          v = r * sin(angle + s * (r - minDim)/minDim) + centerY;
        } 
        
        j = floor(u);
        k = floor(v);
 
        a = u - j;
        b = v - k;

		Matrix3D v1, v2;
		
		v1[0][0] = 1-b;
		v1[0][1] = b;
		v1[1][1] = 0;
		v1[2][2] = 0;
  
		v2[0][0] = 1-a;
		v2[1][0] = a;
		v2[1][1] = 0;
		v2[2][2] = 0; 

        if(j >= 0 && j < width-1 && k >= 0 && k < height-1) {
          for(int m = 0; m < 3; ++m) {
            
            // taking four nearest neighbors
            Matrix3D fourNeighbors;
			
			fourNeighbors[0][0] = image[k][j][m];
			fourNeighbors[0][1] = image[k][j+1][m];
			fourNeighbors[1][0] = image[k+1][j][m];
			fourNeighbors[1][1] = image[k+1][j+1][m];
			fourNeighbors[2][2] = 0;
			
			Matrix3D productMatrix = v1 * fourNeighbors * v2;
			rgb[m] += productMatrix[0][0];

          }
        }
      }
      transformedImage[row][col].r = rgb[0]/totalSamples;
      transformedImage[row][col].g = rgb[1]/totalSamples;
      transformedImage[row][col].b = rgb[2]/totalSamples; 

    }
  }
  cout<<"Both artifact removals finished.\n";
}

// performs twirling of image using s, cX and cY provided by user
void performTwirl() {

  // dimensions of transformed image would be same as original image 
  Matrix3D outputLeftBottom;  
  outputLeftBottom[0][0] = 0.0;
  outputLeftBottom[1][0] = 0.0;
  outputLeftBottom[2][0] = 1.0;
  outputLeftBottom[1][1] = 0.0;
  outputLeftBottom[2][2] = 0.0;
 
  Matrix3D outputLeftTop;
  outputLeftTop[0][0] = 0.0;
  outputLeftTop[1][0] = float(height)-1;
  outputLeftTop[2][0] = 1.0;
  outputLeftTop[1][1] = 0.0;
  outputLeftTop[2][2] = 0.0;
    
  Matrix3D outputRightBottom;  
  outputRightBottom[0][0] = float(width)-1;
  outputRightBottom[1][0] = 0.0;
  outputRightBottom[2][0] = 1.0;
  outputRightBottom[1][1] = 0.0;
  outputRightBottom[2][2] = 0.0;
    
  Matrix3D outputRightTop;
  outputRightTop[0][0] = float(width)-1; 
  outputRightTop[1][0] = float(height)-1; 
  outputRightTop[2][0] = 1.0;
  outputRightTop[1][1] = 0.0;
  outputRightTop[2][2] = 0.0;
 
  outputWidth = width;
  outputHeight = height;

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  float x = std::min(outputLeftBottom[0][0],
            std::min(outputLeftTop[0][0],
            std::min(outputRightBottom[0][0], outputRightTop[0][0])));

  // getting y co-ordinate of origin of transformed image
  float y = std::min(outputLeftBottom[1][0],
            std::min(outputLeftTop[1][0],
            std::min(outputRightBottom[1][0], outputRightTop[1][0])));

  Matrix3D origin;
  origin[0][0] = x;
  origin[1][0] = y;
  origin[2][0] = 0.0;
  origin[1][1] = 0.0;
  origin[2][2] = 0.0;


  centerX = (int)(cX * outputWidth);
  centerY = (int)(cY * outputHeight);
  minDim = std::min(outputWidth, outputHeight);

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

      float u = r * cos(angle + s * (r - minDim)/minDim) + centerX;
      float v = r * sin(angle + s * (r - minDim)/minDim) + centerY;

      if(!bilinear) mapPixelsToTransformedImage(u, v, row, col); // just perform the transformation
      else removeMagnificationArtifacts(u, v, row, col); // perform transformation and remove magnification artificats

    }
  }
}

void displayAffTransfUsage() {
  cout<<"Commands: 'p' -- perspective warp"<<endl;
  cout<<"          'n' -- twirl"<<endl;
  cout<<"Enter 'd' command to indicate that you are done."<<endl;
  cout<<"Enter 'w' command to write the image to a file."<<endl;
}

void processInput(Matrix3D &M) {
  char command[1024];
  bool done;
  
  displayAffTransfUsage();

  for(done = false; !done;) {
    cout<<"> ";
    cin>>command;
    toLowerCase(command);
    
    if(strcmp(command, "d") == 0) done = true;
    else if(strlen(command) != 1) displayAffTransfUsage();
    else {
      switch(command[0]) {
//        case 'i':
//          initiateInverseMap(M);
//          break; 		  
        case 'p':
          cout<<"Perspective: Please enter the perspective factors pX and pY:"<<endl;
          cin>>pX>>pY;
          performPerspective(M, pX, pY);
          done = true;
          break;                  
        case 'n':
          done = true;
          twirl = true;
          cout<<"Twirl: Please enter the twirl factors cX, cY, and strength:"<<endl;
          cin>>cX>>cY>>s;
          performTwirl();
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
	  
	// writes images  
    case 'w':
    case 'W':      
      writeImage(outputImage);
      break;
      
    // repairs both artifacts  
    case 'r':
    case 'R':
      {
        jittered = !jittered; // toggle between transformed and repaired image
        Matrix3D M1;
        
        if(jittered) {
		  // remove both from perspective warp	
          if(!twirl) { 
            performPerspective(M1, pX, pY);
            removeBothArtifacts(M1);
          }
          //remove both from twirl
          else {
            performTwirl();
            removeBothArtifacts(M1);
          }
        }
        else {
          bilinear = false;
          // return to perspective warp (no repairs)
          if(!twirl) {
            performPerspective(M1, pX, pY);
            performAffineTransform(M1);
          }
          // return to twirl (no repairs)
          else performTwirl();
        }
        glRasterPos2i(0,0);
        glDrawPixels(outputWidth, outputHeight, GL_RGBA, GL_FLOAT, transformedImage[0]);
        glutSwapBuffers();
        break;
      }
    case 'b':
    case 'B':
      {
        bilinear = !bilinear; // toggle between transformed and repaired image
        Matrix3D M1;
        
        if(!twirl) {
          performPerspective(M1, pX, pY);
          performAffineTransform(M1);
        }
        else
        performTwirl();
        glRasterPos2i(0,0);
        glDrawPixels(outputWidth, outputHeight, GL_RGBA, GL_FLOAT, transformedImage[0]);
        glutSwapBuffers();
        break;
      }
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
  cout<<"./okwarp input.img [output.img]"<<endl;
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


