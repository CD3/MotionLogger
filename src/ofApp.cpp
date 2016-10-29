#include "ofApp.h"

// configuration

// size of image grabbed from web cam
#define WIDTH 1280
#define HEIGHT 720
// string that will be prefixed to log file.
// resulting filename will be prefix-timestamp.txt
#define LOGFILEPREFIX "data"
// min and max blob size area.
// this can be used to limit the range of sizes
// that the software will consider for an object.
#define MINBLOBAREA 20;
#define MAXBLOBAREA (1280*720/3);
// maximum number of blobs to search for
#define MAXBLOBS 1

//--------------------------------------------------------------
void ofApp::setup(){

  #ifdef _USE_LIVE_VIDEO
        vidGrabber.setVerbose(true);
        vidGrabber.setup(WIDTH,HEIGHT);
  #else
        vidPlayer.load("fingers.mov");
        vidPlayer.play();
        vidPlayer.setLoopState(OF_LOOP_NORMAL);
  #endif

  colorImg.allocate(WIDTH,HEIGHT);
  grayImage.allocate(WIDTH,HEIGHT);
  grayBg.allocate(WIDTH,HEIGHT);
  grayDiff.allocate(WIDTH,HEIGHT);

  bLearnBakground = true;
  threshold = 80;

  grabInterval = 0;
  logInterval = -1;
  lastGrabTime = 0;
  lastLogTime = 0;

  minBlobArea = MINBLOBAREA;
  maxBlobArea = MAXBLOBAREA;
  maxBlobs    = MAXBLOBS;

  stringstream ss;
  ss << LOGFILEPREFIX << "-";
  ss << ofGetYear();
  ss << ofGetMonth();
  ss << ofGetDay();
  ss << ofGetHours();
  ss << ofGetMinutes();
  ss << ".txt";
  logfn = ss.str();
  fout.open(logfn);
  fout << "# time(ms) x(pixel) y(pixel) .. repeated for each blob\n";

  out = &cout;

  startTime = 0;

}

//--------------------------------------------------------------
void ofApp::update(){
  int time = ofGetElapsedTimeMillis();
    
  ofBackground(100,100,100);
  bool bNewFrame = false;

  #ifdef _USE_LIVE_VIDEO
  if( grabInterval > -1 && time - lastGrabTime - grabInterval > 0 )
  {
    vidGrabber.update();
    bNewFrame = vidGrabber.isFrameNew();
    lastGrabTime = time;
  }
  #else
  vidPlayer.update();
  bNewFrame = vidPlayer.isFrameNew();
  #endif

  if (bNewFrame){

    #ifdef _USE_LIVE_VIDEO
            colorImg.setFromPixels(vidGrabber.getPixels());
      #else
            colorImg.setFromPixels(vidPlayer.getPixels());
        #endif

        grayImage = colorImg;
    if (bLearnBakground == true){
      grayBg = grayImage;    // the = sign copys the pixels from grayImage into grayBg (operator overloading)
      bLearnBakground = false;
    }

    // take the abs value of the difference between background and incoming and then threshold:
    grayDiff.absDiff(grayBg, grayImage);
    grayDiff.threshold(threshold);

    // find contours
    contourFinder.findContours(grayDiff, minBlobArea, maxBlobArea, maxBlobs, true);  // find holes

    // log data
    if( logInterval > -1 && time - lastLogTime - logInterval > 0 )
    {
      *out << time - startTime;
      for (int i = 0; i < contourFinder.nBlobs; i++){
        *out << " "
             << contourFinder.blobs[i].boundingRect.getCenter().x
             << " "
             << contourFinder.blobs[i].boundingRect.getCenter().y;
      }
      *out << endl;
      lastLogTime = time;
    }
  }

}

//--------------------------------------------------------------
void ofApp::draw(){

  // draw the incoming, the grayscale, the bg and the thresholded difference
  ofSetHexColor(0xffffff);


  int width  = 320;
  int height = 240;
  int time = ofGetElapsedTimeMillis() - startTime;
  

  grayImage.draw(20,20,width,height);
  grayBg.draw(360,20,width,height);
  grayDiff.draw(20,280,width,height);
  // then draw the contours:
  ofFill();
  ofSetHexColor(0x333333);
  ofDrawRectangle(360,280,width,height);
  ofSetHexColor(0xffffff);

  // draw the whole contour finder
  contourFinder.draw(360,280,width,height);

  // finally, a report:
  ofSetHexColor(0xffffff);
  stringstream reportStr;

  reportStr << "status:" << endl
            << "   time (ms): "          << time                 << endl
            << "   grab interval (ms): " << grabInterval         << endl
            << "   log interval (ms): "  << logInterval          << endl
            << "   threshold: "          << threshold            << endl
            << "   num blobs found: "    << contourFinder.nBlobs << endl
            << "   fps: " << ofGetFrameRate() << " @ " << WIDTH << "x" << HEIGHT << endl
            << "   output to ";
  if( *out == cout )
    reportStr << "console" << endl;
  else
    reportStr << "file (" << logfn << ")" << endl;
    ofDrawBitmapString(reportStr.str(), 20, 600);
  reportStr.str(string());

  reportStr << "commands:" << endl
              << "   ' ' (spacebar) to capture background image" << endl
              << "   '+' increase threshold for blob detection" << endl
              << "   '-' decrease threshold for blob detection" << endl
              << "   '.' increase log interval by 1 ms"         << endl
              << "   ',' decrease log interval by 1 ms"         << endl
              << "   '>' increase log interval by 1 s"          << endl
              << "   '<' decrease log interval by 1 s"          << endl
              << "   'f' send log data to file"                 << endl
              << "   'c' send log data to console"              << endl;



  ofDrawBitmapString(reportStr.str(), 360, 600);


}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

  switch (key){
    case ' ':
      bLearnBakground = true;
      break;
    case '+':
      threshold ++;
      if (threshold > 255) threshold = 255;
      break;
    case '.':
      logInterval++;
      break;
    case ',':
      logInterval--;
      if (logInterval < -1) logInterval = -1;
      break;
    case '>':
      logInterval += 1000;
      break;
    case '<':
      logInterval -= 1000;
      if (logInterval < -1) logInterval = -1;
      break;
    case 'f':
      out = &fout;
      break;
    case 'c':
      out = &cout;
      break;
    case 'r':
      startTime = ofGetElapsedTimeMillis();
      break;
  }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
