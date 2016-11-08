#include "ofApp.h"


// configuration settings
void ofApp::loadSettings(string fn)
{
  // try to load settings from a file
  if(!settings.loadFile(fn))
    cout << "Settings could not be loaded from file." << endl;
  else
    cout << "Loaded settings from file." << endl;

  #define SETSETTING(name,default) settings.setValue( name, settings.getValue( name, default ) );

  // set defaults for any missing settings
  SETSETTING( "webcam_width",  (int)1920             ); // the webcamera resolution
  SETSETTING( "webcam_height", (int)1080             );
  SETSETTING( "logfileprefix", (string)"data"        ); // prefix for log file
  SETSETTING( "loginterval",   (int)-1               ); // log interval
  SETSETTING( "maxblobs",      (int)1                ); // maximum number of blobs to identify
  SETSETTING( "minblobarea",   (int)9                ); // min and max areas to consider for a blob
  SETSETTING( "maxblobarea",   (int)(1920*1080)/100  );
  SETSETTING( "threshold",     (int)50               ); // image contrast used for blob detection in gray background diff image

  string tmp;
  settings.copyXmlToString( tmp );
  cout << "Settings:" << endl;
  cout << tmp << endl;

#undef SETSETTING


  // cached settings
  threshold     = settings.getValue("threshold",0);
  minBlobArea   = settings.getValue("minblobarea",0);
  maxBlobArea   = settings.getValue("maxblobarea",0);
  maxBlobs      = settings.getValue("maxblobs",0);
  logInterval   = settings.getValue("loginterval",-1);


}

void ofApp::saveSettings(string fn)
{
  settings.saveFile(fn);
}

//--------------------------------------------------------------
void ofApp::setup(){

  
  // saved/default settings
  settings_fn = "settings.xml";
  
  loadSettings(settings_fn);

  // derived settings

  lastLogTime   = 0;
  grabInterval  = 0;
  lastFrameTime = 0;


  int width,height;
  #ifdef _USE_LIVE_VIDEO
  vidSource.setVerbose(true);
  vidSource.setup(settings.getValue("webcam_width",0),settings.getValue("webcam_height",0));
  // put the actual width and height back in the settings
  settings.setValue( "webcam_width", vidSource.getWidth() );
  settings.setValue( "webcam_height", vidSource.getHeight() );
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



  stringstream ss;
  ss << settings.getValue("logfileprefix","_")<< "-";
  ss << ofGetYear();
  ss << ofGetMonth();
  ss << ofGetDay();
  ss << ofGetHours();
  ss << ofGetMinutes();
  ss << ".txt";
  logfn = ss.str();
  fout.open(logfn);
  fout << "# time(ms) x(pixel) y(pixel) blob_area(pixels).. repeated for each blob\n";

  out = &cout;

  startTime = 0;
  ofBackground(100,100,100);

}

//--------------------------------------------------------------
void ofApp::update(){
    
  bool bNewFrame = false;

  uint64_t time,stime,etime;
  stime = etime = 0;
  time = ofGetElapsedTimeMillis();
  if( grabInterval > -1 && time - lastFrameTime > grabInterval )
  {
    stime = ofGetElapsedTimeMillis();
    vidSource.update();
    etime = ofGetElapsedTimeMillis();
    time = (stime+etime)/2;
    // use the average time at which the image was aquired.
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
    if( logInterval > -1 && time - lastLogTime > logInterval )
    {
      *out << time - startTime;
      for (int i = 0; i < contourFinder.nBlobs; i++){
        *out << " "
             << contourFinder.blobs[i].boundingRect.getCenter().x
             << " "
             << contourFinder.blobs[i].boundingRect.getCenter().y
             << " "
             << contourFinder.blobs[i].area;
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
            << "   '+/-' increase/decrease threshold for blob detection"  << endl
            << "   './,' increase/decrease log interval by 1 ms"          << endl
            << "   '>/<' increase/decrease log interval by 1 s"           << endl
            << "   'f/c' send log data to file/console"                   << endl
            << "   ']/[' increase/decrease max blob area by 1 pixel"      << endl
            << "   ''/;' increase/decrease min blob area by 1 pixel"      << endl
            << "   '}/{' increase/decrease max blob area by 1000 pixels"   << endl
            << "   '\"/:' increase/decrease min blob area by 1000 pixels"  << endl
            << "   's'   save settings to a file                      "  << endl
            << "   'd'   delete settings file                          "  << endl
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
      if(threshold < 255)
        threshold ++;
      break;
    case '-':
      if(threshold > 0)
        threshold --;
      break;
    case '.':
      logInterval++;
      break;
    case ',':
      logInterval--;
      break;
    case '>':
      logInterval += 1000;
      break;
    case '<':
      logInterval -= 1000;
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


    case '[':
      maxBlobArea -= 1;
      break;
    case ']':
      maxBlobArea += 1;
      break;
    case '{':
      maxBlobArea -= 1000;
      break;
    case '}':
      maxBlobArea += 1000;
      break;



    case ';':
      minBlobArea -= 1;
      break;
    case '\'':
      minBlobArea += 1;
      break;
   case ':':
      minBlobArea -= 1000;
      break;
    case '"':
      minBlobArea += 1000;
      break;

    case 's':
      saveSettings(settings_fn);
      break;
    case 'd':
      ofFile::removeFile(settings_fn);
      break;
    case 'l':
      loadSettings(settings_fn);
      break;

  }

  if (logInterval < -1) logInterval = -1;
  if (maxBlobArea < minBlobArea) maxBlobArea = minBlobArea;
  if (minBlobArea < 0) minBlobArea = 0;

  // we need to put updated settings into the settings object in case the user want to write
  // them to file later.
  settings.setValue( "maxblobs",    (int)maxBlobs );
  settings.setValue( "maxblobarea", (int)maxBlobArea );
  settings.setValue( "minblobarea", (int)minBlobArea );
  settings.setValue( "threshold",   (int)threshold );
  settings.setValue( "loginterval", (int)logInterval );
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
