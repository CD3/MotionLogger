/**
 * The videoSourceInterface class defines a common interface for video sources
 * used by this app, including webcams and video files. The videoSourceAdapter class
 * wrapps the openframeworks video grabber and video player classes so that the
 * app can switch between them at runtime (instead of compile time)
 */
class videoSourceInterface
{
  public:
    virtual void setup( ofxXmlSettings &s ) = 0;
    virtual void close( ) = 0;

    virtual void start() = 0;
    virtual uint64_t update() = 0;
    virtual void stop() = 0;

    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
    virtual ofPixels& getPixels() = 0;
    virtual bool isFrameNew() = 0;
};

template<typename T>
class videoSourceAdapter : public videoSourceInterface
{
  private:
    T t;

  public:
    videoSourceAdapter(){}

    void setup( ofxXmlSettings &s ){}
    void close( ){};

    void start()          {}
    uint64_t update()     { t.update(); return 0;}
    void stop()           {}

    int getWidth()        {return t.getWidth();}
    int getHeight()       {return t.getHeight();}
    ofPixels& getPixels() {return t.getPixels();}
    bool isFrameNew()     {return t.isFrameNew();}
};

template<>
inline
void videoSourceAdapter<ofVideoGrabber>::setup( ofxXmlSettings &s )
{
  s.pushTag("webcam");
  t.setVerbose(true);
  t.setDeviceID( s.getValue("id",0) );
  t.setup(s.getValue("width",0),s.getValue("height",0));
  // put the actual width and height back in the settings
  s.setValue( "width",  t.getWidth() );
  s.setValue( "height", t.getHeight() );
  s.popTag();
}

template<>
inline
uint64_t videoSourceAdapter<ofVideoGrabber>::update( )
{
  uint64_t time,stime,etime;
  stime = etime = 0;
  stime = ofGetElapsedTimeMillis();
  t.update();
  etime = ofGetElapsedTimeMillis();
  // use the average time at which the image was aquired.
  time = (stime+etime)/2;

  return time;
}



template<>
inline
void videoSourceAdapter<ofVideoPlayer>::setup( ofxXmlSettings &s )
{
  s.pushTag("player");
  string fn = s.getValue("filename", "INVALID");
  if( fn.find('.') == 0 || fn.find('/') == 0 )
    fn = ofFilePath::getAbsolutePath(fn,false);
  else
    fn = ofFilePath::getAbsolutePath(fn,true);

  if( !ofFile::doesFileExist(fn,false) )
  {
    ofLog(OF_LOG_ERROR) << "Video file (" << fn << ") was not found." << endl;
    ofExit();
  }
  else
  {
    t.load(fn);
    t.setLoopState(OF_LOOP_NORMAL);
  }
  s.popTag();
}

template<>
inline
void videoSourceAdapter<ofVideoPlayer>::start( )
{
  t.play();
}

template<>
inline
uint64_t videoSourceAdapter<ofVideoPlayer>::update( )
{
  t.update();

  return t.getCurrentFrame()*t.getDuration()*1000/t.getTotalNumFrames();
}

