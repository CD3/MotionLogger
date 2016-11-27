# MotionLogger

MotionLogger is a simple OpenFrameworks application that can track and log the position of objects in a video feed. It is based on the OpenFrameworks openCV example, and was created to collect experimental data in an Advanced Lab course at Fort Hays State University.

## Building

This repository is an OpenFrameworks project that can be built with `make`, so
it should work on Linux, Mac, and Windows with msys2. The easiest way to build
the proejct is to simply clone this repository into the `myApps` directory of
your OpenFrameworks directory and run `make`. If you are using msys2 on Windows, you
ned to install git first:

    > pacman --sync git # run this if you are using msys2 on Windows
    > cd path/to/of_release_directory
    > cd apps/myApps
    > git clone https://github.com/CD3/MotionLogger.git
    > cd MotionLogger
    > make
    
This will create an executable name `MotionLogger` in the `bin/` directory. To run the application, run this executable

    > ./bin/MotionLogger

You could also double click on this file's icon.

## Updating

To get the latest version of this app, just do a `git pull` from the project directory. If your working
directory is "dirty", `git` may complain. You can clean it using the `git clean` command, but this will delete all
files that are not in the repository or ignored by git.

    > git clean -f -d
    > git pull origin
