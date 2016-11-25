#pragma once

#include "ofConstants.h"
#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxXmlSettings.h"

#define _USE_LIVE_VIDEO    // uncomment this to use a live camera
                           // otherwise, application exects a file named 'input.mov' to be in the data directory.

//class videoSourceInterface
//{
//};

//template<typename T>
//class videoSourceAdapter : public videoSourceInterface
//{
//};

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
    string settings_fn;           // name of file to save/load settings
    ofxXmlSettings settings;      // xml settings object
    void loadSettings(string fn); // load settings from a file
    void saveSettings(string fn); // save settings to a file


    int  migrated = 0;
    bool _migrate( ofxXmlSettings& s, string t, string f );
    bool _has( ofxXmlSettings& s, string k );
    template<typename T>
    T    _get( ofxXmlSettings& s, string k );
    template<typename T>
    void _set( ofxXmlSettings& s, string k, T v, bool overwrite );


    bool has( string k ){return _has(settings,k);}
    template<typename T>
    T    get( string k ){return _get<T>(settings,k);}
    template<typename T>
    void set( string k, T v, bool overwrite = true ){_set(settings,k,v,overwrite);}

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

    void testing();
};


template<typename T>
T ofApp::_get( ofxXmlSettings& s, string k )
{
  vector<string> keys = ofSplitString( k, ".", true );
  size_t N = keys.size();
  T ret;
  bool missing = false;

  for(int i = 0; i < N-1; i++)
  {
    if( !s.pushTag( keys[i] ) )
    {
      missing = true;
      break;
    }
  }

  if(!missing)
    ret = s.getValue( keys[N-1], T() );
  else
    ret = T();

  while( s.getPushLevel() )
    s.popTag();

  return ret;
}

template<typename T>
void ofApp::_set( ofxXmlSettings& s, string k, T v, bool overwrite )
{
  vector<string> keys = ofSplitString( k, ".", true );

  size_t N = keys.size();

  for(int i = 0; i < N-1; i++)
  {
    if(!s.tagExists(keys[i]))
      s.addTag(keys[i]);
    s.pushTag( keys[i] );
  }

  if( overwrite || !s.tagExists(keys[N-1]) )
    s.setValue( keys[N-1], v );

  while( s.getPushLevel() )
    s.popTag();

}

inline
bool ofApp::_has( ofxXmlSettings& s, string k )
{
  vector<string> keys = ofSplitString( k, ".", true );
  size_t N = keys.size();
  bool missing = false;

  for(int i = 0; i < N; i++)
  {
    if( !s.pushTag( keys[i] ) )
    {
      missing = true;
      break;
    }
  }

  while( s.getPushLevel() )
    s.popTag();

  return !missing;
}

inline
bool ofApp::_migrate( ofxXmlSettings& s, string t , string f )
{
  if( _has(s,f) )
    _set(s, t, _get<string>(s,f), true );
  else
    return false;

  migrated += 1;
  return true;
}




#undef SETUP
#undef TEARDOWN



