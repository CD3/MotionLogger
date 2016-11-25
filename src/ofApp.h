#pragma once

#include "ofConstants.h"
#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxXmlSettings.h"

#define _USE_LIVE_VIDEO    // uncomment this to use a live camera
                           // otherwise, application exects a file named 'input.mov' to be in the data directory.


class ofApp : public ofBaseApp{

  public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);    

    #ifdef _USE_LIVE_VIDEO
      ofVideoGrabber    vidSource;
    #else
      ofVideoPlayer     vidSource;
    #endif

    ofxCvColorImage      colorImg;

    ofxCvGrayscaleImage   grayImage;
    ofxCvGrayscaleImage   grayBg;
    ofxCvGrayscaleImage   grayDiff;

    ofxCvContourFinder   contourFinder;

    void saveCurrentImage();

    bool        bLearnBakground;

    size_t      totalBlobArea;
    ostream*    out;
    ofstream    fout;
    string      logfn;

    // configuration
    ofxXmlSettings settings;
    void loadSettings(string fn);
    void saveSettings(string fn);
    string settings_fn;

    // cached config vars
    size_t      webcam_width;  // width of webcam image (in pixels)
    size_t      webcam_height; // height of webcam image (in pixels)
    size_t      threshold;
    uint64_t    startTime;     // time 0 for logged data
    uint64_t    lastFrameTime; // milliseconds
    uint64_t    lastLogTime;   // milliseconds
    int64_t     grabInterval;  // in milliseconds
    int64_t     logInterval;   // in milliseconds

    size_t      blobs_minarea;
    size_t      blobs_maxarea;
    size_t      blobs_num;

};

