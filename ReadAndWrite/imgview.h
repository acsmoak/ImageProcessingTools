//imgview.cpp

/*
Name: Mandy Smoak
Date: 9/6/2016
Project: Homework 1
Filename: imgview.h
This application is a simple image viewer with image read and write capabilities. It
utilizes OpenGL for window control and drawing. Additionally, it uses OpenImageIO library for C++ to handle all image reading, writing, and the underlying functionality for pixmap population.
Usage: See README
*/


#include <OpenImageIO/imageio.h>
#include <cstdlib>
#include <iostream>

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

using namespace std;
OIIO_NAMESPACE_USING

/* Default width and height values */
#define WIDTH	    600	// window dimensions
#define HEIGHT		600


/* Struct for image data */
struct image_t{
	unsigned char *pixmap;
	int w, h;
	int channels;
};

/* Functions and a brief desprion of what task it performs*/

/* Reshape Callback Routine: sets up the viewport and drawing coordinates
This routine is called when the window is created and every time the window
is resized, by the program or by the user
*/
void handleReshape(int w, int h);




/* Handle Display callback routine. If image has been loaded, 
reset the raster position to the bottom left corner and draw pixels of 
w x h contained in the associated pixmap
*/
void handleDisplay();




/*
Read image file specified by user and prepare for draw
*/
void readImage(string infilename, int argc);




/*
Write image currently displayed in GLUT window to filename specified by user.
*/
void writeImage();




/* This routine is called every time a key is pressed on the keyboard*/
void handleKey(unsigned char key, int x, int y);




