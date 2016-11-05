# MotionLogger

MotionLogger is a simple OpenFrameworks application that can track and log the position of objects in a video feed. It is based on the OpenFrameworks openCV example, and was created to collect experimental data in an Advanced Lab course at Fort Hays State University.

## Building

This repository is an OpenFrameworks project. To build the application, you need to have OpenFrameworks installed. Then place this directory in the myApps directory of the OpenFrameworks installation (usually
at of_vx.x.x_msys2_release/apps/myApps/MotionLogger. If you don't put this directory in the myApps directory, then you will have to edit the `Makefile` to point OF_ROOT at the OpenFrameworks install. 

To build, just run make.

    > make

This will create an executable name `MotionLogger` in the `bin/` directory. To run the application, run this executable

    > ./bin/MotionLogger

You could also double click on this file's icon.
