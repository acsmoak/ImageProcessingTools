Name: Mandy Smoak 

Date: October 4, 2016



Program Description:

  	-reads, writes, displays images using OIIO and OpenGL
  	-handles input from keyboard
  	-converts rgb pixel to hsv format
  	-performs masking of the image depending on h,s and v values
  	-composites an image supporting 4 channels over an image supporting 3 channels

Advanced Extension:

	-In addition to the above, this program performs spill suppression


Instructions:

	$ make all	 
	 
	$ ./alphamask dhouse.png (performs masking on dhouse.png)
	$ ./alphamask dhouse.png fg.png (performs masking on dhouse.png 
				and saves masked image as fg.png after pressing W/w)
	$ ./alphamask -ss dhouse.png (performs masking and spill supression)
	$ ./alphamask -ss dhouse.png fg.png (performs masking and spill 
				supressionon on dhouse.png and saves masked image as 
				fg.png after pressing W/w)
	$ ./composite fg.png bg.png (performs A over B compositing)
         
	$ make clean
	 

Note:	 Press w to write the currently displayed image to a file
		 Press q or ESC to quit the program


