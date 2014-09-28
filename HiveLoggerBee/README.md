#Who this repository is for

These are the libraries for the Apitronics Bee, which are built on top of the Xmegaduino IDE. You can either download the ready made Xmegaduino IDEs from our website that will have everything already configured for you.
TODO: put links here when they're ready

Or you can use this repository to set it up from scratch. This is particularly useful if you want to help in the development of this project since you will have everything easily linked up to GitHub.

#Setting up a development machine

##Download Xmegaduino IDE
First you need to download the arduino-1.0.1-rx2-xmegaduino-beta4b. What's important here is that we are still using arduino-1.0.1 (1.5 isn't quite stable yet) but that you have a more recent toolchain than the beta4 release.

[Download the IDE here](https://github.com/apitronics/Bee/releases/tag/beta-release).

##Add in our libraries

(1) 
In Mac/Linux:
Edit the boards.txt file in ```~/arduino-1.0.1-rx2-xmegaduino-beta4b/hardware/xmegaduino/```. Add the following at the top, bottom, middle, or anywhere really. For Mac, once Arduino is moved into Applications, boards.txt will be in ```/Applications/Arduino.app/Contents/Resources/Java/hardware/xmegaduino/```.

```
##############################################################

api.name=Apitronics Bee

api.build.mcu=atxmega128a3
api.build.f_cpu=32000000L
api.build.core=xmega
api.build.variant=api

api.upload.tool=avrdude
api.upload.protocol=avr109
api.upload.speed=115200
api.upload.maximum_size=131072

##############################################################
```
In Windows:
Skip this step

(2) In variants, create a soft link to the api directory in this repository.

In Mac/Linux:

```ln -s ~/Bee/api ~/arduino-1.0.1-rx2-xmegaduino-beta4b/hardware/xmegaduino/variants/api```
Note that we assumed that this repository and the Xmegaduino are both in your home directory. You might need to fix these paths.

In Windows:
Skip this step

(3) Open up the Xmegaduino IDE and go to: `File-->Preferences` and make the sketchbook location the `sketches` directory in this repository. Restart the IDE and all the libraries will be loaded.

Now you're good to go!

#Improvements to Think of
* changing sample rates (and other settings) remotely
* add automation to framework; how does it interact with sensors?

