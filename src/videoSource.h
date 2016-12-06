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
    virtual void update() = 0;
    virtual void stop() = 0;
    virtual bool isPaused() = 0;

    virtual uint64_t getCurrentFrameTime() = 0;
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
    bool paused;
    uint64_t currentFrameTime;

  public:
    videoSourceAdapter(){}

    void setup( ofxXmlSettings &s ){}
    void close( ){};

    void start()          {}
    void update()         {t.update();}
    void stop()           {}
    bool isPaused()       {return t.isPaused();}

    int getWidth()        {return t.getWidth();}
    int getHeight()       {return t.getHeight();}
    ofPixels& getPixels() {return t.getPixels();}
    bool isFrameNew()     {return t.isFrameNew();}
    uint64_t getCurrentFrameTime() {return currentFrameTime;}
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
  paused = false;
}

template<>
inline
void videoSourceAdapter<ofVideoGrabber>::start( )
{
  paused = false;
}

template<>
inline
void videoSourceAdapter<ofVideoGrabber>::stop( )
{
  paused = true;
}

template<>
inline
bool videoSourceAdapter<ofVideoGrabber>::isPaused( )
{
  return paused;
}

template<>
inline
void videoSourceAdapter<ofVideoGrabber>::update( )
{
  if(paused)
    return;

  uint64_t stime,etime;
  stime = etime = 0;
  stime = ofGetElapsedTimeMillis();
  t.update();
  etime = ofGetElapsedTimeMillis();
  // use the average time at which the image was aquired.
  currentFrameTime = (stime+etime)/2;
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
void videoSourceAdapter<ofVideoPlayer>::update( )
{
  t.update();

  currentFrameTime = t.getCurrentFrame()*t.getDuration()*1000/t.getTotalNumFrames();
}

