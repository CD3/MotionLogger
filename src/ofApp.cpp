#include "ofApp.h"


// configuration settings
void ofApp::loadSettings(string fn)
{
  ofxXmlSettings savedSettings;
  // try to load settings from a file
  if(!savedSettings.loadFile(fn))
    ofLog( OF_LOG_NOTICE ) << "Settings could not be loaded from file." << endl;
  else
    ofLog( OF_LOG_NOTICE ) << "Loaded settings from file." << endl;

  // setting migrations
  // when you want to change a setting name, just add an entry here to convert
  // the old setting name to the new setting name.
  _migrate(savedSettings, "webcam.id", "webcam_id");
  _migrate(savedSettings, "webcam.width", "webcam_width");
  _migrate(savedSettings, "webcam.height", "webcam_height");
  _migrate(savedSettings, "logging.file.prefix", "logfileprefix");
  _migrate(savedSettings, "logging.interval", "loginterval");
  _migrate(savedSettings, "blobs.num", "maxblobs");
  _migrate(savedSettings, "blobs.minarea", "minblobarea");
  _migrate(savedSettings, "blobs.maxarea", "maxblobarea");
  _migrate(savedSettings, "blobs.threshold", "threshold");

  if(migrated)
  {
    ofLog( OF_LOG_WARNING ) << "Your current settings file an old version." << endl
                            << "Future versions of this app *should* continue to support this version" << endl
                            << "(" << migrated << " settings were automatically updated)" << endl
                            << "but it is recommended that you update to the latest version by saving"  << endl
                            << "a new settings file."  << endl;
  }



#define SET(name,default) \
  if( _has( savedSettings, name ) ) \
    set( name, _get<string>( savedSettings, name ), false ); \
  else \
    set( name, default ); \

  // set defaults for any missing settings
  SET( "webcam.id"           , (int)0               ); // the webcamera to use
  SET( "webcam.width"        , (int)1000            ); // the webcamera resolution
  SET( "webcam.height"       , (int)1080            );
  SET( "logging.file.prefix" , (string)"data"       ); // prefix for log file
  SET( "logging.interval"    , (int)-1              ); // log interval
  SET( "blobs.num"           , (int)1               ); // maximum number of blobs to identify
  SET( "blobs.minarea"       , (int)9               ); // min and max areas to consider for a blob
  SET( "blobs.maxarea"       , (int)(1920*1080)/100 );
  SET( "blobs.threshold"     , (int)50              ); // image contrast used for blob detection in gray background diff image

  string tmp;
  settings.copyXmlToString( tmp );
  ofLog( OF_LOG_NOTICE ) << "Settings:" << endl;
  ofLog( OF_LOG_NOTICE ) << tmp << endl;

#undef SET


  // cached settings
  threshold     = get<int>("blobs.threshold");
  blobs_minarea = get<int>("blobs.minarea");
  blobs_maxarea = get<int>("blobs.maxarea");
  blobs_num     = get<int>("blobs.num");
  logInterval   = get<int>("logging.interval");


}

void ofApp::saveSettings(string fn)
{
  settings.saveFile(fn);
}

// just a place to put testing code. not to be used in production.
void ofApp::testing()
{
  cout << "TESTING: '" << has( "one" ) << "'" << endl;
  cout << "TESTING: '" << has( "one.two.three" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one.two.three" ) << "'" << endl;

  set("one.two.three", "123");

  cout << "TESTING: '" << has( "one" ) << "'" << endl;
  cout << "TESTING: '" << has( "one.two.three" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one.two.three" ) << "'" << endl;
  cout << "TESTING: '" << get<int>( "one.two.three" ) << "'" << endl;

  set("one.two.three", "1234");

  cout << "TESTING: '" << has( "one" ) << "'" << endl;
  cout << "TESTING: '" << has( "one.two.three" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one.two.three" ) << "'" << endl;

  set("one.two.three", "1234", false);

  cout << "TESTING: '" << has( "one" ) << "'" << endl;
  cout << "TESTING: '" << has( "one.two.three" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one" ) << "'" << endl;
  cout << "TESTING: '" << get<string>( "one.two.three" ) << "'" << endl;

  string tmp;
  settings.copyXmlToString( tmp );
  cout << "TESTING: " << tmp << endl;
}

//--------------------------------------------------------------
void ofApp::setup(){

  //testing();

  //ofSetLogLevel( OF_LOG_VERBOSE );
  ofSetLogLevel( OF_LOG_NOTICE  );

  
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
  vidSource.setDeviceID( get<int>("webcam.id") );
  vidSource.setup(get<int>("webcam.width"),get<int>("webcam.height"));
  // put the actual width and height back in the settings
  set( "webcam.width", vidSource.getWidth() );
  set( "webcam.height", vidSource.getHeight() );
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
  ss << get<string>("logging.file.prefix")<< "-";
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
    contourFinder.findContours(grayDiff, blobs_minarea, blobs_maxarea, blobs_num, true);  // find holes
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



  int width  = 640;
  int height = 360;
  int pad    = 20;
  int time = ofGetElapsedTimeMillis() - startTime;
  

  // draw the incoming, the grayscale, the bg and the thresholded difference
  ofSetHexColor(0xffffff);
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
            << "   blob area min/max (pixels):"  << blobs_minarea << "/" << blobs_maxarea << endl
            << "   num blobs/total blob area: "  << contourFinder.nBlobs << "/" << totalBlobArea << endl
            << "   source: " << vidSource.getWidth() << "x" << vidSource.getHeight() << " @ " << ofGetFrameRate() << " fps" << endl
            << "   output to: "
            ;
  if( out == &cout )
    reportStr << "console" << endl;
  else
    reportStr << "file (" << logfn << ")" << endl;
  reportStr << "commands:" << endl
            << "   ' ' (spacebar) capture background image"               << endl
            << "   '+/-' increase/decrease threshold for blob detection"  << endl
            << "   './,' increase/decrease log interval by 1 ms"          << endl
            << "   '>/<' increase/decrease log interval by 1 s"           << endl
            << "   'f/c' send log data to file/console"                   << endl
            << "   'r'   log timer to 0"                                  << endl
            << "   ']/[' increase/decrease max blob area by 1 pixel"      << endl
            << "   ''/;' increase/decrease min blob area by 1 pixel"      << endl
            << "   '}/{' increase/decrease max blob area by 1000 pixels"  << endl
            << "   '\"/:' increase/decrease min blob area by 1000 pixels" << endl
            << "   's'   save settings to a file"                         << endl
            << "   'd'   delete settings file"                            << endl
            << "   'i'   save image of screen"                            << endl
            ;

            
  ofDrawBitmapString(reportStr.str(), pad+width+pad, pad+height+pad);



}

//--------------------------------------------------------------
void ofApp::saveCurrentImage()
{
  stringstream ss;
  ss << settings.getValue("imagefileprefix","screenshot")<< "-";
  ss << ofGetYear();
  ss << ofGetMonth();
  ss << ofGetDay();
  ss << ofGetHours();
  ss << ofGetMinutes();
  ss << ".png";
  ofImage img;
  img.setFromPixels( colorImg.getPixels() );
  img.save( ss.str() );
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  ofLog( OF_LOG_VERBOSE ) << "Key: " << key << endl;

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
      blobs_maxarea -= 1;
      break;
    case ']':
      blobs_maxarea += 1;
      break;
    case '{':
      blobs_maxarea -= 1000;
      break;
    case '}':
      blobs_maxarea += 1000;
      break;



    case ';':
      blobs_minarea -= 1;
      break;
    case '\'':
      blobs_minarea += 1;
      break;
   case ':':
      blobs_minarea -= 1000;
      break;
    case '"':
      blobs_minarea += 1000;
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
    case 'i':
      saveCurrentImage();
      break;

  }

  if (logInterval < -1) logInterval = -1;
  if (blobs_maxarea < blobs_minarea) blobs_maxarea = blobs_minarea;
  if (blobs_minarea < 0) blobs_minarea = 0;

  // we need to put updated settings into the settings object in case the user want to write
  // them to file later.
  set( "blobs.num",    (int)blobs_num );
  set( "blobs.maxarea", (int)blobs_maxarea );
  set( "blobs.minarea", (int)blobs_minarea );
  set( "blobs.threshold",   (int)threshold );
  set( "logging.interval",  (int)logInterval );
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
