#pragma once

#include "ofMain.h"

#include "ofxOpenCv.h"


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

    int         threshold;
    bool        bLearnBakground;

    uint64_t    startTime;    // time 0 for logged data
    int         grabInterval; // in milliseconds
    int         logInterval;  // in milliseconds
    uint64_t    lastFrameTime; // milliseconds
    uint64_t    lastLogTime;  // milliseconds
    int         minBlobArea;
    int         maxBlobArea;
    int         maxBlobs;
    int         totalBlobArea;
    ostream*    out;
    ofstream    fout;
    string      logfn;



};

