/**
* @author George Profenza ,based on code from:
* @author http://github.com/raspberrypi/userland
* @author Samarth Manoj Brahmbhatt, University of Pennsyalvania -> cv::Mat wrapper
* @author Tasanakorn  -> opencv_demo.c
* @author Jason Van Cleave (jvcleave) -> captureApplication mmal openframeworks demo
**/
#pragma once

#include "ofMain.h"

#ifdef TARGET_RASPBERRY_PI

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/mmal_parameters_camera.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#else
enum {
    MMAL_PARAMETER_SATURATION,
    MMAL_PARAMETER_SHARPNESS,
    MMAL_PARAMETER_CONTRAST,
    MMAL_PARAMETER_BRIGHTNESS,
    MMAL_PARAMETER_SHUTTER_SPEED,
    MMAL_PARAMETER_ISO,
    MMAL_PARAMETER_VIDEO_STABILISATION,
    MMAL_PARAMETER_EXPOSURE_COMP
};

#endif

#ifdef OFXADDON_OFXCV
    #include <opencv2/opencv.hpp>
#endif

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT   1
#define MMAL_CAMERA_CAPTURE_PORT 2

class ofxPiCam
{
public:

    ofEventListener saturationListener;
    ofEventListener sharpnessListener;
    ofEventListener contrastListener;
    ofEventListener brightnessListener;
    ofEventListener shutterSpeedListener;
    ofEventListener videoStabiliseListener;
    ofEventListener  exposureCompensationListener;
    ofEventListener  ISOListener;

    ofParameterGroup group;
    ofParameterGroup groupA, groupB, groupC, groupD;

    ofParameter<int> shutterSpeed;
    ofParameter<int> saturation;
    ofParameter<int> sharpness;
    ofParameter<int> contrast;
    ofParameter<int> brightness;

    ofParameter<int>  ISO;
    ofParameter<bool> videoStabilise;
    ofParameter<int>  exposureCompensation;
    ofParameter<int> exposureMode;
    ofParameter<int> flickerAvoidMode;
    ofParameter<int> exposureMeteringMode;
    ofParameter<int> awbMode;

    ofParameter<int> rotation;
    ofParameter<bool> flipHorz;
    ofParameter<bool> flipVert;
    ofParameter<ofVec4f> roi;

    ofParameter<ofVec2f> awbGains;
    ofParameter<bool> colourFXEnable;
    ofParameter<ofVec2f> colourFX;
    ofParameter<int> imageFX;



    typedef struct
    {
       int enable;       /// Turn colourFX on or off
       int u,v;          /// U and V to use
    } MMAL_PARAM_COLOURFX_T;
    typedef struct
    {
       double x;
       double y;
       double w;
       double h;
    } PARAM_FLOAT_RECT_T;

    ofxPiCam();
    ~ofxPiCam();

    bool isFrameNew() {return newFrame;};

    void setup();

    static ofPixels * image;
    static bool isReceiving;
    
#ifdef OFXADDON_OFXCV
    static Mat cvImage;
    static void set_image(cv::Mat _image) {
        newFrame = true;
        image = _image;
    }
    cv::Mat grabCv() {
        newFrame = false;
        return cvImage;
    }
#endif
    static bool newFrame;

    static int width, height;
    
    // ofxCv
    
    void setup(int _w, int _h, bool _color);
    void close();

    ofPixels & grab() { newFrame = false; return *image; }


#ifdef TARGET_RASPBERRY_PI
    static MMAL_POOL_T *camera_video_port_pool;
    int mmal_status_to_int(MMAL_STATUS_T status)
    {
        
       if (status == MMAL_SUCCESS)
          return 0;
       else
       {
          switch (status)
          {
          case MMAL_ENOMEM :   ofLogError("Out of memory"); break;
          case MMAL_ENOSPC :   ofLogError("Out of resources (other than memory)"); break;
          case MMAL_EINVAL:    ofLogError("Argument is invalid"); break;
          case MMAL_ENOSYS :   ofLogError("Function not implemented"); break;
          case MMAL_ENOENT :   ofLogError("No such file or directory"); break;
          case MMAL_ENXIO :    ofLogError("No such device or address"); break;
          case MMAL_EIO :      ofLogError("I/O error"); break;
          case MMAL_ESPIPE :   ofLogError("Illegal seek"); break;
          case MMAL_ECORRUPT : ofLogError("Data is corrupt \attention FIXME: not POSIX"); break;
          case MMAL_ENOTREADY :ofLogError("Component is not ready \attention FIXME: not POSIX"); break;
          case MMAL_ECONFIG :  ofLogError("Component is not configured \attention FIXME: not POSIX"); break;
          case MMAL_EISCONN :  ofLogError("Port is already connected "); break;
          case MMAL_ENOTCONN : ofLogError("Port is disconnected"); break;
          case MMAL_EAGAIN :   ofLogError("Resource temporarily unavailable. Try again later"); break;
          case MMAL_EFAULT :   ofLogError("Bad address"); break;
          default :            ofLogError("Unknown status error"); break;
          }

          return 1;
       }
    }
#endif
    //*/
    
private:
  
    // labels...
    

    string exposureModeLabels[14];
    string exposureMeteringModeLabels[5];
    string awbModeLabels[12];
    string imageFXLabelLabels[24];
    string drcModeLabels[5];
    string flickerAvoidModeLabels[5];

    void initLabels();

    // mmal mode calls...

    void initParameters();

    void setImageFX(int & v);
    void setColourFX(ofVec2f & v);
    void setAWBGains( ofVec2f & v);
    void setROI(ofVec4f & rect);
    void setFlips( bool & b );
    void setRotation(int & v);
    void setUInt32(int parameterEnum, int & v);
    void setInt32(int parameterEnum, int & v);
    void setRationalInt(int parameterEnum, int & v);
    void setBoolean(int parameterEnum, bool & b);
    void setExposureMode(int & v);
    void setExposureMeteringMode(int & v);
    void setAWBMode(int & v);
    void setFlickerAvoidMode(int & v);
    void setSaturation( int & v );
    void setSharpness( int & v );
    void setContrast( int & v );
    void setBrightness( int & v );
    void setShutterSpeed( int & v );
    void setISO( int & v );
    void setVideoStabilise( bool & v );
    void setExposureCompensation( int & v );
    
#ifdef TARGET_RASPBERRY_PI
    bool color;
    MMAL_COMPONENT_T *camera;
    MMAL_COMPONENT_T *preview;
    MMAL_ES_FORMAT_T *format;
    MMAL_STATUS_T status;
    MMAL_PORT_T *camera_preview_port, *camera_video_port, *camera_still_port;
    MMAL_PORT_T *preview_input_port;
    MMAL_CONNECTION_T *camera_preview_connection;
#endif

};

#ifdef TARGET_RASPBERRY_PI
static void color_callback(MMAL_PORT_T *, MMAL_BUFFER_HEADER_T *);
static void gray_callback(MMAL_PORT_T *, MMAL_BUFFER_HEADER_T *);
#endif
