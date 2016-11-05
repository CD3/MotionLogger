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

  int width,height;
  #ifdef _USE_LIVE_VIDEO
  vidSource.setVerbose(true);
  vidSource.setup(WIDTH,HEIGHT);
  #else
  vidSource.load("input.mov");
  vidSource.play();
  vidSource.setLoopState(OF_LOOP_NORMAL);
  #endif
  width  = vidSource.getWidth();
  height = vidSource.getHeight();

  colorImg.allocate(width,height);
  grayImage.allocate(width,height);
  grayBg.allocate(width,height);
  grayDiff.allocate(width,height);

  bLearnBakground = true;
  threshold = 80;

  grabInterval = 0;
  logInterval = -1;
  lastFrameTime = 0;
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

  if( grabInterval > -1 && time - lastFrameTime - grabInterval > 0 )
  {
    vidSource.update();
    bNewFrame = vidSource.isFrameNew();
    lastFrameTime = time;
  }

  if (bNewFrame){

    colorImg.setFromPixels(vidSource.getPixels());

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
    totalBlobArea = 0;
    for (int i = 0; i < contourFinder.nBlobs; i++)
      totalBlobArea += contourFinder.blobs[i].area;

    // log data
    if( logInterval > -1 && time - lastLogTime - logInterval > 0 )
    {
      *out << time - startTime;
      for (int i = 0; i < contourFinder.nBlobs; i++){
        *out << " "
             << contourFinder.blobs[i].boundingRect.getCenter().x
             << " "
             << contourFinder.blobs[i].boundingRect.getCenter().y;
        totalBlobArea += contourFinder.blobs[i].area;
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


  int width  = 640;
  int height = 360;
  int pad    = 20;
  int time = ofGetElapsedTimeMillis() - startTime;
  

  grayImage.draw(pad,pad,width,height);
  ofDrawBitmapString(string("grayscale image"), pad,pad );
  grayBg.draw(pad+width+pad,pad,width,height);
  ofDrawBitmapString(string("background image"), pad+width+pad,pad );
  grayDiff.draw(pad,pad+height+pad,width,height);
  ofDrawBitmapString(string("subtracted image"), pad,pad+height+pad );
  contourFinder.draw(pad,pad+height+pad,width,height);

  // displaye status information
  ofSetHexColor(0xffffff);
  stringstream reportStr;

  reportStr << "status:" << endl
            << "   time (ms): "                  << time                 << endl
            << "   grab interval (ms): "         << grabInterval         << endl
            << "   log interval (ms): "          << logInterval          << endl
            << "   threshold: "                  << threshold            << endl
            << "   blob area min/max (pixels):"  << minBlobArea << "/" << maxBlobArea << endl
            << "   num blobs/total blob area: "  << contourFinder.nBlobs << "/" << totalBlobArea << endl
            << "   source: " << vidSource.getWidth() << "x" << vidSource.getHeight() << " @ " << ofGetFrameRate() << " fps" << endl
            << "   output to: "
            ;
  if( out == &cout )
    reportStr << "console" << endl;
  else
    reportStr << "file (" << logfn << ")" << endl;
  reportStr << "commands:" << endl
            << "   ' ' (spacebar) to capture background image" << endl
            << "   '+/-' increase/decrease threshold for blob detection" << endl
            << "   './,' increase/decrease log interval by 1 ms"         << endl
            << "   '>/<' increase/decrease log interval by 1 s"          << endl
            << "   'f/c' send log data to file/console"                  << endl
            << "   's/a' increase/decrease max blob area by 1 pixel"     << endl
            << "   'x/z' increase/decrease min blob area by 1 pixel"     << endl
            << "   'S/A' increase/decrease max blob area by 1000 pixels" << endl
            << "   'X/Z' increase/decrease min blob area by 1000 pixels" << endl
            ;

            
  ofDrawBitmapString(reportStr.str(), pad+width+pad, pad+height+pad);



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


    case 'a':
      maxBlobArea -= 1;
      break;
    case 's':
      maxBlobArea += 1;
      break;
    case 'z':
      minBlobArea -= 1;
      break;
    case 'x':
      minBlobArea += 1;
      break;

    case 'A':
      maxBlobArea -= 1000;
      break;
    case 'S':
      maxBlobArea += 1000;
      break;
    case 'Z':
      minBlobArea -= 1000;
      break;
    case 'X':
      minBlobArea += 1000;
      break;

  }
  if (maxBlobArea < minBlobArea) maxBlobArea = minBlobArea;
  if (minBlobArea < 0) minBlobArea = 0;
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
