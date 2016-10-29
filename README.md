# MotionLogger

MotionLogger is a simple OpenFrameworks application that can track and log the position of objects in a video feed. It is based on the OpenFrameworks openCV example, and was created to collect experimental data in an Advanced Lab course at Fort Hays State University.

## Building

This repository is an OpenFrameworks project. To build the application, you need to have OpenFrameworks installed. You then need to tell the project where your OpenFrameworks installation is, which you can do in a couple of ways.

- edit the `Makefile` to set the OF_ROOT variable to your installation.
- run the projectGenerator tool to update the project and select your OF installation.

After you have configured your OpenFrameworks installation, just run make.

    > make

This will create an executable name `MotionLogger` in the `bin/` directory. To run the application, run this executable

    > ./bin/MotionLogger

You could also double click on this file's icon.
