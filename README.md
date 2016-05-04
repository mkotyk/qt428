
qt428
=====
This hack allows you to stream the live video feed from a QSee QT428 DVR
security camera to an H.264 video player.

I started by trying out zmodopipe, but found it did not work with my DVR,
possibly since I had upgraded the firmware to 3.2.0.

This code was created from reverse engineering ethernet packet samples and
refering to the zmodopipe code.  Many of the structures and fields are unknown
or guesses.

The only video player I found that could reliably play the H.264 video stream
was mplayer.

To view multiple channels, simply start multiple instance of the program with
different channel arguments.


Compiling
---------
I've included a simple Makefile.  The code is standard C++ and does not use
any unusual libraries.  I've only tried it in Linux, but it should be
relatively easy to port to other platforms.

License
-------
Apache 2.0 License

