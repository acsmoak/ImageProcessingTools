
Name: Mandy Smoak

Date: September 7, 2016


Project Description:  

    This program reads an image from a file, 
    displays the image, and writes the displayed image to an image file. OpenGL and GLUT 
    are used for displaying images. In addition, it will handle almost any type of image 
    file by making use of OpenImageIO’s API for the reading and writing of images.

Advanced Extension: 

    In addition, I have provided a reshape callback 
    routine that responds to the user resizing the display  window.   
    If the user increases the size of the display window so that it is bigger
    than the image, the image should remain centered in the window

Known Problems: 

    After resizing/scaling the window (using advanced extension),
    the image written is offcenter. 



Instructions:

    $ make clean && make

    $ ./imgview or ./imgview image.jpg

    Press r to read and display a new image form a file

    Scale up or down the window

    Press w to write the currently displayed image to a file

    Press q or ESC to quit the program

