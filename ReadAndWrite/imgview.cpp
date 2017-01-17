//imgview.cpp

/*
Name: Mandy Smoak
Date: 9/6/2016
Project: Homework 1
Filename: imgview.cpp
This application is a simple image viewer with image read and write capabilities. It
utilizes OpenGL for window control and drawing. Additionally, it uses OpenImageIO library for C++ to handle all image reading, writing, and the underlying functionality for pixmap population.
Usage: See README
*/

#include "imgview.h"

image_t container;




void handleReshape(int w, int h){  
  float ratio = 1.0;
	
  if(w < container.w || h < container.h) {
	  const float xratio = w / (container.w * 1.0);
	  const float yratio = h / (container.h * 1.0);
	  ratio = xratio < yratio ? xratio : yratio;
	  glPixelZoom(ratio, ratio);
  }
	
  const int x = (int)((w - (container.w * ratio)) / 2);
  const int y = (int)((h - (container.h * ratio)) / 2);
  glViewport(x, y, w - x, h - y);
  
  /* Swap to projection matrix for Ortho2D operation, load Identity Matrix, perform
     operation, and then switch back to modelview matrix */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, container.w, 0, container.h);
  glMatrixMode(GL_MODELVIEW);
  return;
}



void handleDisplay(){
  glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color
  glRasterPos2i(0, 0);
  glDrawPixels(container.w, container.h, GL_RGB, GL_UNSIGNED_BYTE, container.pixmap);
  glutSwapBuffers();

  glFlush();
}




void readImage(string infilename, int argc){

  /* If call is made from keypress, and not from command line input, prompt
     user for input */
  if (argc == 0){
    cout << "Enter input image filename: ";
    cin >> infilename;
  }
 
  ImageInput *infile = ImageInput::open(infilename);
  const ImageSpec &spec = infile->spec();
  container.w = spec.width;
  container.h = spec.height;
  container.channels = spec.nchannels;
  container.pixmap = new unsigned char[container.channels * container.w * container.h];
  
  glutReshapeWindow(container.w, container.h);  
  
  int scanlinesize = container.w * container.channels * sizeof(container.pixmap[0]);
  infile->read_image(TypeDesc::UINT8, (char *)container.pixmap+(container.h-1)*scanlinesize , AutoStride, -scanlinesize, AutoStride);
  
  glutPostRedisplay();
    
  infile->close ();
  delete infile;
  
 return;
  
}




void writeImage(){
//  unsigned char pixmap[4 * container.w * container.h];
  string outfilename;
  
  /* Read pixels on front frame buffer and store in associated pixmap of size 
     4 * window width * window height */
  glReadBuffer(GL_FRONT);
  
  if(container.channels == 3){
    glReadPixels(0, 0, container.w, container.h, GL_RGB, GL_UNSIGNED_BYTE, container.pixmap);
  }
  else{
	glReadPixels(0, 0, container.w, container.h, GL_RGB, GL_UNSIGNED_BYTE, container.pixmap);
  }
  
  /* Prompt user for output filename and write image */
  cout << "Enter output image filename: ";
  cin >> outfilename;

  /* Create the oiio file handler for the image */
  ImageOutput *outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  /* Open a file for writing the image. The file header will indicate an image of
   width w, height h, and 4 channels per pixel (RGBA). All channels will be of
   type unsigned char */
  ImageSpec spec(container.w, container.h, container.channels, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    delete outfile;
    return;
  }
  
  /* Write the image to the file. All channel values in the pixmap are taken to be
     unsigned chars */
  int scanlinesize = container.w * container.channels * sizeof(container.pixmap[0]);
  if(!outfile->write_image(TypeDesc::UINT8, (char *)container.pixmap+(container.h-1)*scanlinesize , AutoStride, -scanlinesize, AutoStride)){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    delete outfile;
    return;
  }
    
  /* Close the image file after the image is written */
  if(!outfile->close()){
    cerr << "Could not close " << outfilename << ", error = " << geterror() << endl;
    delete outfile;
    return;
  }

  /* Free up space associated with the oiio file handler */
  delete outfile;
  
  return;
}




void handleKey(unsigned char key, int x, int y){
  
  switch(key){
    case 'r':		// r - Prompt user for input image filename
    case 'R':
      readImage("", 0);
      break;
      
    case 'w':		// w - Prompt user for output image filename
    case 'W':
      writeImage();
      break;
      
    case 'q':		// q - Quit
    case 'Q':
    case 27:		// esc - Quit
      exit(0);
      
    default:		// not a valid key - just ignore it
      return;
  }
}




int main(int argc, char* argv[]){
   
  /* Start up the glut utilities */
  glutInit(&argc, argv);
  
  /* Create the graphics window, giving width, height, and title text */
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(500, 300);
  glutCreateWindow("Image View");
  
  /* Specify window clear (background) color to be opaque black */
  glClearColor(0, 0, 0, 0);
  
  /* Set up the callback routines to be called when glutMainLoop() detects
     an event */
  glutReshapeFunc(handleReshape); // window resize callback  
  glutDisplayFunc(handleDisplay);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback  
  
  /* Reads in specified file written in commandline */
  if(argc == 2){
	readImage(argv[1], argc);
  }
  
  /*Routine that loops forever looking for events. It calls the registered
    callback routine to handle each event that is detected */
  glutMainLoop();
 
  return 0;
}



