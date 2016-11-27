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
  _migrate(savedSettings, "video.source", "webcam.id");

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
  SET( "video.source"        , (int)0               ); // the device to use for video (-1 is video file)
  SET( "player.filename"     , (string)"input.mov"  ); // name of file to load video from
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
  currVidSource = get<int>("video.source");
  blobs_threshold = get<int>("blobs.threshold");
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


  if( currVidSource < 0 )
    vidSource.reset( new videoSourceAdapter<ofVideoPlayer>() );
  else
    vidSource.reset( new videoSourceAdapter<ofVideoGrabber>() );

  vidSource->setup( settings );
  vidSource->start( );
  
  int width  = vidSource->getWidth();
  int height = vidSource->getHeight();

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

  currFrameTime = vidSource->update();
  bNewFrame = vidSource->isFrameNew();

  if (bNewFrame){

    colorImg.setFromPixels(vidSource->getPixels());

    grayImage = colorImg;
    if (bLearnBakground == true){
      grayBg = grayImage;    // the = sign copys the pixels from grayImage into grayBg (operator overloading)
      bLearnBakground = false;
    }

    // take the abs value of the difference between background and incoming and then threshold:
    grayDiff.absDiff(grayBg, grayImage);
    grayDiff.threshold(blobs_threshold);

    // find contours
    contourFinder.findContours(grayDiff, blobs_minarea, blobs_maxarea, blobs_num, true);  // find holes
    totalBlobArea = 0;
    for (int i = 0; i < contourFinder.nBlobs; i++)
      totalBlobArea += contourFinder.blobs[i].area;

    // log data
    if( logInterval > -1 && currFrameTime - lastLogTime > logInterval )
    {
      *out << currFrameTime - startTime;
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
      lastLogTime = currFrameTime;
    }
  }

}

//--------------------------------------------------------------
void ofApp::draw(){



  int width  = 640;
  int height = 360;
  int pad    = 20;
  int time = currFrameTime - startTime;
  

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
            << "   log interval (ms): "          << logInterval          << endl
            << "   threshold: "                  << blobs_threshold      << endl
            << "   blob area min/max (pixels):"  << blobs_minarea << "/" << blobs_maxarea << endl
            << "   num blobs/total blob area: "  << contourFinder.nBlobs << "/" << totalBlobArea << endl
            << "   source: " << vidSource->getWidth() << "x" << vidSource->getHeight() << " @ " << ofGetFrameRate() << " fps" << endl
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
      if(blobs_threshold < 255)
        blobs_threshold ++;
      break;
    case '-':
      if(blobs_threshold > 0)
        blobs_threshold --;
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
  set( "blobs.threshold",   (int)blobs_threshold );
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
