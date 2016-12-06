#include "ofApp.h"


// just a place to put testing code. not to be used in production.
void ofApp::testing()
{
}

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
  _migrate(savedSettings, "detection.grayscale.threshold", "blobs.threshold");
  _migrate(savedSettings, "detection.blobs.num", "blobs.num");
  _migrate(savedSettings, "detection.blobs.minarea", "blobs.minarea");
  _migrate(savedSettings, "detection.blobs.maxarea", "blobs.maxarea");

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
  SET( "video.source",                  (int)0               ) ; // the device to use for video (-1 is video file)
  SET( "player.filename",               (string)"input.mov"  ) ; // name of file to load video from
  SET( "webcam.width",                  (int)1000            ) ; // the webcamera resolution
  SET( "webcam.height",                 (int)1080            ) ;
  SET( "logging.file.prefix",           (string)"data"       ) ; // prefix for log file
  SET( "logging.interval",              (int)-1              ) ; // log interval
  SET( "detection.blobs.num",                     (int)1               ) ; // maximum number of blobs to identify
  SET( "detection.blobs.minarea",                 (int)9               ) ; // min and max areas to consider for a blob
  SET( "detection.blobs.maxarea",                 (int)(1920*1080)/100 ) ;
  SET( "detection.grayscale.threshold", (int)50              ) ; // image contrast used for blob detection in gray background diff image
  SET( "detection.color.red",           (int)255             ) ;
  SET( "detection.color.green",         (int)255             ) ;
  SET( "detection.color.blue",          (int)255             ) ;
  SET( "detection.color.radius",        (int)20              ) ;

  string tmp;
  settings.copyXmlToString( tmp );
  ofLog( OF_LOG_NOTICE ) << "Settings:" << endl;
  ofLog( OF_LOG_NOTICE ) << tmp << endl;

#undef SET


  // cached settings
  currVidSource = get<int>("video.source");
  blobs_minarea = get<int>("detection.blobs.minarea");
  blobs_maxarea = get<int>("detection.blobs.maxarea");
  blobs_num     = get<int>("detection.blobs.num");
  logInterval   = get<int>("logging.interval");

  blobs_threshold = get<int>("detection.grayscale.threshold");

  colorMask_color.r = get<int>("detection.color.red");
  colorMask_color.g = get<int>("detection.color.green");
  colorMask_color.b = get<int>("detection.color.blue");
  colorMask_radius  = get<int>("detection.color.radius");


}

//--------------------------------------------------------------
void ofApp::saveSettings(string fn)
{
  settings.saveFile(fn);
}

//--------------------------------------------------------------
void ofApp::setup()
{

  //testing();

  ofSetLogLevel( OF_LOG_VERBOSE );
  //ofSetLogLevel( OF_LOG_NOTICE  );

  
  // load settings
  settings_fn = "settings.xml";
  loadSettings(settings_fn);

  // initialize video source and images

  if( currVidSource < 0 )
    vidSource.reset( new videoSourceAdapter<ofVideoPlayer>() );
  else
    vidSource.reset( new videoSourceAdapter<ofVideoGrabber>() );

  vidSource->setup( settings );
  vidSource->start( );
  
  int width  = vidSource->getWidth();
  int height = vidSource->getHeight();

  rawImage.allocate(width,height);
  colorMasked.allocate(width,height);
  grayImage.allocate(width,height);
  grayBg.allocate(width,height);
  contourImage.allocate(width,height);


  // setup logging
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

  startTime   = 0;
  lastLogTime = 0;


  ofBackground(100,100,100); 
}

//--------------------------------------------------------------
void ofApp::update()
{
    
  bool bNewFrame = false;

  vidSource->update();
  currFrameTime = vidSource->getCurrentFrameTime();
  bNewFrame = vidSource->isFrameNew();

  if (bNewFrame || vidSource->isPaused()){

    rawImage.setFromPixels(vidSource->getPixels());

    if(mode == GRAYSCALE)
    {
      // the contour finder requires a gray image
      grayImage = rawImage;
      // take the abs value of the difference between background
      // and incoming and then threshold:
      contourImage.absDiff(grayBg, grayImage);
      contourImage.threshold(blobs_threshold);
    }

    if(mode == COLOR)
    {
      ofPixels pixels = rawImage.getPixels();
      int width = pixels.getWidth();
      int height= pixels.getHeight();
      ofColor c;

      for(int i = 0; i < width*height; i++)
      {
        c.r = pixels[3*i+0];
        c.g = pixels[3*i+1];
        c.b = pixels[3*i+2];
        if( distanceToColor( c ) > colorMask_radius )
        {
          pixels[3*i+0] = 0;
          pixels[3*i+1] = 0;
          pixels[3*i+2] = 0;
        }
      }
      colorMasked.setFromPixels(pixels);
      contourImage = colorMasked;
      contourImage.threshold(1);

    }

    // find contours
    contourFinder.findContours(contourImage, blobs_minarea, blobs_maxarea, blobs_num, true);  // find holes
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
void ofApp::draw()
{



  int width  = preview_image_width;
  int height = preview_image_height;
  int pad    = preview_image_pad;
  

  // draw the incoming, the grayscale, the bg and the thresholded difference
  int bgColor = 0xffffff;
  ofSetHexColor(bgColor);


  if(mode==GRAYSCALE)
  {
    grayImage.draw(pad,pad,width,height);
    ofDrawBitmapString(string("grayscale image"), pad,pad );
    grayBg.draw(pad+width+pad,pad,width,height);
    ofDrawBitmapString(string("background image"), pad+width+pad,pad );
    contourImage.draw(pad,pad+height+pad,width,height);
    ofDrawBitmapString(string("detection image"), pad,pad+height+pad );
    contourFinder.draw(pad,pad+height+pad,width,height);
  }
  if(mode==COLOR)
  {
    rawImage.draw(pad,pad,width,height);
    ofDrawBitmapString(string("raw image"), pad,pad );
    colorMasked.draw(pad+width+pad,pad,width,height);
    ofDrawBitmapString(string("color masked image"), pad+width+pad,pad );
    contourImage.draw(pad,pad+height+pad,width,height);
    ofDrawBitmapString(string("detection image"), pad,pad+height+pad );
    contourFinder.draw(pad,pad+height+pad,width,height);
  }

  // displaye status information
  ofSetHexColor(0xffffff);
  ofDrawBitmapString(buildStatusString(), pad+width+pad, pad+height+pad);
  if(mode==COLOR)
  {
  ofSetColor(colorMask_color);
  ofDrawRectangle(pad+width+pad+width-50,pad+height+pad,50,50);
  ofSetHexColor(bgColor);
  }

}

string ofApp::buildStatusString()
{
  uint64_t time = currFrameTime - startTime;
  stringstream statusStr;
  statusStr << "mode: " << getModeString() << endl;
  statusStr << "status:" << endl;
  statusStr << "   time (ms): "                  << time                   << endl;
  statusStr << "   log interval (ms): "          << logInterval            << endl;
  if( mode == GRAYSCALE )
  statusStr << "   threshold: "                  << blobs_threshold        << endl;
  if( mode == COLOR     )
  statusStr << "   color mask (red): "           << (int)colorMask_color.r << endl;
  if( mode == COLOR     )
  statusStr << "   color mask (green): "         << (int)colorMask_color.g << endl;
  if( mode == COLOR     )
  statusStr << "   color mask (blue): "          << (int)colorMask_color.b << endl;
  if( mode == COLOR     )
  statusStr << "   color mask radius: "          << colorMask_radius       << endl;
  statusStr << "   blob area min/max (pixels):"  << blobs_minarea << "/"   << blobs_maxarea << endl;
  statusStr << "   num blobs/total blob area: "  << contourFinder.nBlobs   << "/" << totalBlobArea << endl;
  statusStr << "   source: " << vidSource->getWidth() << "x" << vidSource->getHeight() << " @ " << ofGetFrameRate() << " fps" << endl;
  statusStr << "   output to: ";
  if( out == &cout )
    statusStr << "console" << endl;
  else
    statusStr << "file (" << logfn << ")" << endl;
  statusStr << "commands:"                                                << endl;
  statusStr << "   ' '   capture background image"                        << endl;
  if( mode == GRAYSCALE )
  statusStr << "   '+/-' increase/decrease threshold for blob detection"  << endl;
  if( mode == COLOR     )
  statusStr << "   'r/R' increase/decrease color mask red component"      << endl;
  if( mode == COLOR     )
  statusStr << "   'g/G' increase/decrease color mask green component"    << endl;
  if( mode == COLOR     )
  statusStr << "   'b/B' increase/decrease color mask blue component"     << endl;
  if( mode == COLOR     )
  statusStr << "   '+/-' increase/decrease color mask radius"             << endl;
  statusStr << "   './,' increase/decrease log interval by 1 ms"          << endl;
  statusStr << "   '>/<' increase/decrease log interval by 1 s"           << endl;
  statusStr << "   'f/c' send log data to file/console"                   << endl;
  statusStr << "   't'   set log timer back to 0"                         << endl;
  statusStr << "   ']/[' increase/decrease max blob area by 1 pixel"      << endl;
  statusStr << "   ''/;' increase/decrease min blob area by 1 pixel"      << endl;
  statusStr << "   '}/{' increase/decrease max blob area by 1000 pixels"  << endl;
  statusStr << "   '\"/:' increase/decrease min blob area by 1000 pixels" << endl;
  statusStr << "   'p'   pause/play video"                                << endl;
  statusStr << "   's'   save settings to a file"                         << endl;
  statusStr << "   'd'   delete settings file"                            << endl;
  statusStr << "   'i'   save image of screen"                            << endl;
            ;

  return statusStr.str();
            
}

float ofApp::distanceToColor( ofColor c )
{
  float d = 0;
  d += pow(c.r-colorMask_color.r,2);
  d += pow(c.g-colorMask_color.g,2);
  d += pow(c.b-colorMask_color.b,2);
  return sqrt( d );
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
  img.setFromPixels( rawImage.getPixels() );
  img.save( ss.str() );
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
  ofLog( OF_LOG_VERBOSE ) << "Key: " << key << endl;

  // COMMON COMMANDS
  switch (key){
    case '1':
      mode = GRAYSCALE;
      break;
    case '2':
      mode = COLOR;
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
    case 't':
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

    case 'p':
      if(vidSource->isPaused())
        vidSource->start();
      else
        vidSource->stop();
      break;

  }

  if( mode == GRAYSCALE)
  {
    switch (key){
      case ' ':
        grayBg = grayImage;
        break;
      case '+':
        if(blobs_threshold < 255)
          blobs_threshold ++;
        break;
      case '-':
        if(blobs_threshold > 0)
          blobs_threshold --;
        break;
    }
  }

  if( mode == COLOR )
  {
    switch (key){
      case 'r':
        colorMask_color.r -= 1;
        break;
      case 'R':
        colorMask_color.r += 1;
        break;
      case 'g':
        colorMask_color.g -= 1;
        break;
      case 'G':
        colorMask_color.g += 1;
        break;
      case 'b':
        colorMask_color.b -= 1;
        break;
      case 'B':
        colorMask_color.b += 1;
        break;
      case '+':
        colorMask_radius++;
        break;
      case '-':
        colorMask_radius--;
        break;
    }
  }




  if (logInterval < -1) logInterval = -1;
  if (blobs_maxarea < blobs_minarea) blobs_maxarea = blobs_minarea;
  if (blobs_minarea < 0) blobs_minarea = 0;

  // we need to put updated settings into the settings object in case the user want to write
  // them to file later.
  set( "logging.interval",              (int)logInterval );
  set( "detection.blobs.num",           (int)blobs_num );
  set( "detection.blobs.maxarea",       (int)blobs_maxarea );
  set( "detection.blobs.minarea",       (int)blobs_minarea );
  set( "detection.grayscale.threshold", (int)blobs_threshold );
  set( "detection.color.radius",        (int)colorMask_radius );
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y )
{

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{

  if(mode == COLOR)
  {
    if( x > preview_image_pad
    &&  x < (preview_image_pad + preview_image_width)
    &&  y > preview_image_pad
    &&  y < (preview_image_pad + preview_image_height) )
    {
      // user clicked inside of the color image
      int W = rawImage.getWidth();
      int H = rawImage.getHeight();
      int xx = ofMap( x-preview_image_pad, 0, preview_image_width , 0, W );
      int yy = ofMap( y-preview_image_pad, 0, preview_image_height, 0, H );
      int ii = yy*W + xx;

      ofLog(OF_LOG_VERBOSE) << "getting color at : "<<xx<<","<<yy<<" (" << ii << ")...";
      colorMask_color.r = rawImage.getPixels()[3*ii+0];
      colorMask_color.g = rawImage.getPixels()[3*ii+1];
      colorMask_color.b = rawImage.getPixels()[3*ii+2];
      ofLog(OF_LOG_VERBOSE) << "done." << endl;

      set( "detection.color.red",   colorMask_color.r);
      set( "detection.color.green", colorMask_color.g);
      set( "detection.color.blue",  colorMask_color.b);
    }

  }

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{ 

}
