/**
* @author George Profenza ,based on code from:
* @author http://github.com/raspberrypi/userland
* @author Samarth Manoj Brahmbhatt, University of Pennsyalvania -> cv::Mat wrapper
* @author Tasanakorn  -> opencv_demo.c
* @author Jason Van Cleave (jvcleave) -> captureApplication mmal openframeworks demo
**/
#pragma once

#include "ofMain.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/mmal_parameters_camera.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#ifdef OFXADDON_OFXCV
    #include <opencv2/opencv.hpp>
#endif

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT   1
#define MMAL_CAMERA_CAPTURE_PORT 2

class ofxPiCam
{
public:


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
    static MMAL_POOL_T *camera_video_port_pool;
    
    // ofxCv
    
    void setup(int _w, int _h, bool _color);
    void close();

    ofPixels & grab() { newFrame = false; return *image; }

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

    int setImageFX(int & v);
    int setColourFX(ofVec2f & v);
    int setAWBGains( ofVec2f & v);
    int setROI(ofVec4f & rect);
    int setFlips( bool & b );
    int setRotation(int & v);
    int setUInt32(int parameterEnum, int & v);
    int setInt32(int parameterEnum, int & v);
    int setRationalInt(int parameterEnum, int & v);
    int setBoolean(int parameterEnum, bool & b);
    int setExposureMode(int & v);
    int setExposureMeteringMode(int & v);
    int setAWBMode(int & v);
    int setFlickerAvoidMode(int & v);

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
    //*/
private:
    bool color;
    MMAL_COMPONENT_T *camera;
    MMAL_COMPONENT_T *preview;
    MMAL_ES_FORMAT_T *format;
    MMAL_STATUS_T status;
    MMAL_PORT_T *camera_preview_port, *camera_video_port, *camera_still_port;
    MMAL_PORT_T *preview_input_port;
    MMAL_CONNECTION_T *camera_preview_connection;

};

static void color_callback(MMAL_PORT_T *, MMAL_BUFFER_HEADER_T *);
static void gray_callback(MMAL_PORT_T *, MMAL_BUFFER_HEADER_T *);
