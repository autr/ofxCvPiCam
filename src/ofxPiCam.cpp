#include "ofxPiCam.h"

using namespace std;

int ofxPiCam::width = 0;
int ofxPiCam::height = 0;
ofPixels * ofxPiCam::image = new ofPixels();
bool ofxPiCam::newFrame = false;

#ifdef OFXADDON_OFXCV
Mat ofxPiCam::cvImage = Mat();
#endif

#ifdef TARGET_RASPBERRY_PI

MMAL_POOL_T * ofxPiCam::camera_video_port_pool = NULL;
static void gray_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    MMAL_BUFFER_HEADER_T *new_buffer;
    mmal_buffer_header_mem_lock(buffer);
    
    // get grayscale pixels
    
    unsigned char* pointer = (unsigned char *)(buffer -> data);
    
    ofxPiCam::image->setFromPixels( pointer, ofxPiCam::width, ofxPiCam::height, 1);
#ifdef OFXADDON_OFXCV
    ofxPiCam::set_image(Mat(ofxPiCam::height, ofxPiCam::width, CV_8UC1, pointer));
#endif
    if (!isReceiving) isReceiving = true;
    
    mmal_buffer_header_release(buffer);
    if (port->is_enabled) {
        MMAL_STATUS_T status;
        new_buffer = mmal_queue_get(ofxPiCam::camera_video_port_pool->queue);
        if (new_buffer)
            status = mmal_port_send_buffer(port, new_buffer);
        if (!new_buffer || status != MMAL_SUCCESS)
            ofLogVerbose() << ("Unable to return a buffer to the video port\n");
    }
}

static void color_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    MMAL_BUFFER_HEADER_T *new_buffer;
    mmal_buffer_header_mem_lock(buffer);
    
    // get color pixels
    
    unsigned char* pointer = (unsigned char *)(buffer -> data);
    
    ofxPiCam::image->setFromPixels( pointer, ofxPiCam::width, ofxPiCam::height, 3);
    ofxPiCam::image->swapRgb();
    if (!isReceiving) isReceiving = true;

#ifdef OFXADDON_OFXCV
    ofxPiCam::set_image(Mat(ofxPiCam::height, ofxPiCam::width, CV_8UC3, pointer));
#endif
    
    mmal_buffer_header_release(buffer);
    if (port->is_enabled) {
        MMAL_STATUS_T status;
        new_buffer = mmal_queue_get(ofxPiCam::camera_video_port_pool->queue);
        if (new_buffer)
            status = mmal_port_send_buffer(port, new_buffer);
        if (!new_buffer || status != MMAL_SUCCESS)
            ofLogVerbose() << ("Unable to return a buffer to the video port\n");
    }
}

#endif

void ofxPiCam::setup(int _w,int _h,bool _color)
{
    
    initLabels();
    initParameters();
    
#ifdef TARGET_RASPBERRY_PI
    color = _color;
    width = _w;
    height = _h;
    camera = 0;
    preview = 0;
    camera_preview_port = NULL;
    camera_video_port = NULL;
    camera_still_port = NULL;
    preview_input_port = NULL;
    camera_preview_connection = 0;

    bcm_host_init();
    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
    if (status != MMAL_SUCCESS) {
        ofLog(OF_LOG_ERROR, "Error: creating camera \n");
    }
    camera_preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
    camera_video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
    camera_still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];
    {
        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config = {{ MMAL_PARAMETER_CAMERA_CONFIG, sizeof (cam_config)}, width, height, 0, 0,width, height, 3, 0, 1, MMAL_PARAM_TIMESTAMP_MODE_RESET_STC };
        mmal_port_parameter_set(camera->control, &cam_config.hdr);
    }
    format = camera_video_port->format;
    if(color){
        format->encoding = MMAL_ENCODING_BGR24;
        format->encoding_variant = MMAL_ENCODING_BGR24;
    }else{
        format->encoding = MMAL_ENCODING_I420;
        format->encoding_variant = MMAL_ENCODING_I420;
    }
    format->es->video.width = width;
    format->es->video.height = height;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = width;
    format->es->video.crop.height = height;
    format->es->video.frame_rate.num = 30;
    format->es->video.frame_rate.den = 1;

    camera_video_port->buffer_size = width * height * 3 / 2;
    camera_video_port->buffer_num = 1;

    status = mmal_port_format_commit(camera_video_port);

    if (status != MMAL_SUCCESS) {
        //printf("Error: unable to commit camera video port format (%u)\n", status);
        ofLog(OF_LOG_ERROR, "Error: unable to commit camera video port format \n");
    }
    // create pool form camera video port
    camera_video_port_pool = (MMAL_POOL_T *) mmal_port_pool_create(camera_video_port,
                                                                   camera_video_port->buffer_num, camera_video_port->buffer_size);
    if(color) {
        status = mmal_port_enable(camera_video_port, color_callback);
        if (status != MMAL_SUCCESS)
            ofLog(OF_LOG_ERROR, "Error: unable to enable camera video port \n");
        else
            ofLogVerbose() << "Attached color callback" << endl;
    }
    else {
        status = mmal_port_enable(camera_video_port, gray_callback);
        if (status != MMAL_SUCCESS)
            ofLog(OF_LOG_ERROR, "Error: unable to enable camera video port \n");
        else
            ofLogVerbose() << "Attached gray callback" << endl;
    }
    status = mmal_component_enable(camera);
    // Send all the buffers to the camera video port
    int num = mmal_queue_length(camera_video_port_pool->queue);
    int q;
    for (q = 0; q < num; q++) {
        MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(camera_video_port_pool->queue);
        if (!buffer) {
            ofLogVerbose() << "Unable to get a required buffer " << q << " from pool queue\n";
        }
        if (mmal_port_send_buffer(camera_video_port, buffer) != MMAL_SUCCESS) {
            ofLogVerbose() << "Unable to send a buffer to encoder output port " << q;
        }
    }
    if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
        //printf("%s: Failed to start capture\n", __func__);
        ofLog(OF_LOG_ERROR, "Failed to start capture");
    }
    ofLogVerbose() << "Capture started" << endl;
#else
    ofLogNotice("ofxPiCam") << "is not being initialised; use ofVideoGrabber instead.";
    
#endif
}
void close() {

    // mmal_component_create
    // mmal_port_parameter_set
    // mmal_port_format_commit
    // mmal_port_pool_create
    // mmal_port_enable


    // MMAL_COMPONENT_T *camera;
    // MMAL_COMPONENT_T *preview;
    // MMAL_ES_FORMAT_T *format;
    // MMAL_STATUS_T status;
    // MMAL_PORT_T *camera_preview_port, *camera_video_port, *camera_still_port;
    // MMAL_PORT_T *preview_input_port;
    // MMAL_CONNECTION_T *camera_preview_connection;

    // mmal_component_disable(camera);
    // mmal_component_destroy(camera);

}
ofxPiCam::ofxPiCam()
{
    ofLogVerbose() << "ofxPiCam()";
}
ofxPiCam::~ofxPiCam()
{
    ofLogVerbose() << "~ofxPiCam";
}
//settings
//*


void ofxPiCam::initParameters() {


    groupA.add(saturation.set("saturation",0,-100,100));
    groupA.add(sharpness.set("sharpness",0,-100,100));
    groupA.add(contrast.set("contrast",0,-100,100));
    groupA.add(brightness.set("brightness",50,0,100));
    groupA.add(awbMode.set("autoWhiteBalanceMode",0,0,10));
    groupA.add( awbGains.set("awbGains", ofVec2f(0,0), ofVec2f(0,0), ofVec2f(1,1)));

    saturation.newListener([this](int & v){ setRationalInt( MMAL_PARAMETER_SATURATION, v ); });
    sharpness.newListener([this](int & v){ setRationalInt( MMAL_PARAMETER_SHARPNESS, v ); });
    contrast.newListener([this](int & v){ setRationalInt( MMAL_PARAMETER_CONTRAST, v ); });
    brightness.newListener([this](int & v){ setRationalInt( MMAL_PARAMETER_BRIGHTNESS, v ); });
    awbMode.addListener( this, &ofxPiCam::setAWBMode );
    awbGains.addListener( this, &ofxPiCam::setAWBGains);

    groupB.add(shutterSpeed.set("shutterSpeed",0,0,330000)); // (in micro seconds)
    groupB.add(ISO.set("ISO",300,100,800));
    groupB.add(videoStabilise.set("videoStabilise",false));
    groupB.add(exposureCompensation.set("exposureCompensation",0,-10,10));
    groupB.add(exposureMode.set("exposureMode",0,0,13));
    groupB.add(flickerAvoidMode.set("flickerAvoidMode", 0, 0, 2));
    groupB.add(exposureMeteringMode.set("exposureMeteringMode",0,0,4));

    shutterSpeed.newListener([this](int & v){ setUInt32( MMAL_PARAMETER_SHUTTER_SPEED, v ); });
    ISO.newListener([this](int & v){ setUInt32( MMAL_PARAMETER_ISO, v ); });
    videoStabilise.newListener([this](bool & b){ setBoolean( MMAL_PARAMETER_VIDEO_STABILISATION, b ); });
    exposureCompensation.newListener([this](int & v){ setInt32( MMAL_PARAMETER_EXPOSURE_COMP, v ); });
    exposureMode.addListener( this, &ofxPiCam::setExposureMode);
    flickerAvoidMode.addListener( this, &ofxPiCam::setFlickerAvoidMode );
    exposureMeteringMode.addListener( this, &ofxPiCam::setExposureMeteringMode );

    groupC.add( rotation.set("rotation", 0, 0, 3));
    groupC.add( flipHorz.set("flipHorz", false));
    groupC.add( flipVert.set("flipVert", false));
    groupC.add( roi.set("ROI", ofVec4f(0,0,1,1),ofVec4f(0,0,0.01,0.01),ofVec4f(0.99,0.99,1,1) ) );

    rotation.addListener( this, &ofxPiCam::setRotation);
    flipHorz.addListener( this, &ofxPiCam::setFlips);
    flipVert.addListener( this, &ofxPiCam::setFlips);
    roi.addListener(this, &ofxPiCam::setROI);

    groupD.add( colourFX.set("colourFX", ofVec2f(0,0), ofVec2f(0,0), ofVec2f(1,1)));
    groupD.add( imageFX.set("imageFX", 0, 0, 23));

    colourFX.addListener( this, &ofxPiCam::setColourFX);
    imageFX.addListener( this, &ofxPiCam::setImageFX);

    group.add(groupA);
    group.add(groupB);
    group.add(groupC);
    group.add(groupD);

}







void ofxPiCam::setImageFX(int & v){

#ifdef TARGET_RASPBERRY_PI
    MMAL_PARAM_IMAGEFX_T imageFX = v;
    MMAL_PARAMETER_IMAGEFX_T imgFX = {{MMAL_PARAMETER_IMAGE_EFFECT,sizeof(imgFX)}, imageFX};
    
    if (!camera)
        return 1;
    
    mmal_status_to_int(mmal_port_parameter_set(camera->control, &imgFX.hdr));
#endif
}
void ofxPiCam::setColourFX(ofVec2f & v)
{
#ifdef TARGET_RASPBERRY_PI
   MMAL_PARAMETER_COLOURFX_T colfx = {{MMAL_PARAMETER_COLOUR_EFFECT,sizeof(colfx)}, 0, 0, 0};

   if (!camera)
      return 1;

   colfx.enable = colourFX->enable;
   colfx.u = v.x;
   colfx.v = v.y;

   mmal_status_to_int(mmal_port_parameter_set(camera->control, &colfx.hdr));
#endif

}
void ofxPiCam::setAWBGains( ofVec2f & v){
#ifdef TARGET_RASPBERRY_PI
    MMAL_PARAMETER_AWB_GAINS_T param = {{MMAL_PARAMETER_CUSTOM_AWB_GAINS,sizeof(param)}, {0,0}, {0,0}};
    
    if (!camera)
        return 1;
    
    param.r_gain.num = (unsigned int)(v.x * 65536);
    param.b_gain.num = (unsigned int)(v.y * 65536);
    param.r_gain.den = param.b_gain.den = 65536;
    mmal_status_to_int(mmal_port_parameter_set(camera->control, &param.hdr));
#endif
}
void ofxPiCam::setROI(ofVec4f & rect){
#ifdef TARGET_RASPBERRY_PI
    MMAL_PARAMETER_INPUT_CROP_T crop = {{MMAL_PARAMETER_INPUT_CROP, sizeof(MMAL_PARAMETER_INPUT_CROP_T)}};
    if(rect.x < 0) rect.x = 0;if(rect.x > 1) rect.x = 1;
    if(rect.y < 0) rect.y = 0;if(rect.y > 1) rect.y = 1;
    if(rect.z < 0) rect.z = 0;if(rect.z > 1) rect.z = 1;
    if(rect.w < 0) rect.w = 0;if(rect.w > 1) rect.w = 1;
    crop.rect.x = (65536 * rect.x);
    crop.rect.y = (65536 * rect.y);
    crop.rect.width = (65536 * rect.z);
    crop.rect.height = (65536 * rect.w);
    
    mmal_port_parameter_set(camera->control, &crop.hdr);
#endif
}void ofxPiCam::setFlips( bool & b ){
#ifdef TARGET_RASPBERRY_PI
    MMAL_PARAMETER_MIRROR_T mirror = {{MMAL_PARAMETER_MIRROR, sizeof(MMAL_PARAMETER_MIRROR_T)}, MMAL_PARAM_MIRROR_NONE};
    
    if (flipHorz && flipVert)
        mirror.value = MMAL_PARAM_MIRROR_BOTH;
    else
        if (flipHorz)
            mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
        else
            if (flipVert)
                mirror.value = MMAL_PARAM_MIRROR_VERTICAL;
    
    mmal_port_parameter_set(camera->output[0], &mirror.hdr);
    mmal_port_parameter_set(camera->output[1], &mirror.hdr);
    mmal_port_parameter_set(camera->output[2], &mirror.hdr);
#endif
}
void ofxPiCam::setRotation(int & v){
#ifdef TARGET_RASPBERRY_PI
    int ret;
    v *= 90;
    int my_rotation = ((v % 360 ) / 90) * 90;
    
    mmal_port_parameter_set_int32(camera->output[0], MMAL_PARAMETER_ROTATION, my_rotation);
    mmal_port_parameter_set_int32(camera->output[1], MMAL_PARAMETER_ROTATION, my_rotation);
    mmal_port_parameter_set_int32(camera->output[2], MMAL_PARAMETER_ROTATION, my_rotation);
#endif
}

void ofxPiCam::setUInt32(int parameterEnum, int & v) {
#ifdef TARGET_RASPBERRY_PI

    if (!camera) return 1;
    mmal_status_to_int(mmal_port_parameter_set_uint32(camera->control, parameterEnum, v));
#endif
}
void ofxPiCam::setInt32(int parameterEnum, int & v) {
#ifdef TARGET_RASPBERRY_PI

    if (!camera) return 1;
    mmal_status_to_int(mmal_port_parameter_set_int32(camera->control, parameterEnum, v));
#endif
}
void ofxPiCam::setRationalInt(int parameterEnum, int & v) {
#ifdef TARGET_RASPBERRY_PI

    if (!camera) return 1;
    mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, parameterEnum, v));
#endif
}void ofxPiCam::setBoolean(int parameterEnum, bool & b) {
#ifdef TARGET_RASPBERRY_PI

    if (!camera) return 1;
    mmal_status_to_int(mmal_port_parameter_set_boolean(camera->control, parameterEnum, v));
#endif
}
void ofxPiCam::setExposureMode(int & v){
#ifdef TARGET_RASPBERRY_PI

    MMAL_PARAM_EXPOSUREMODE_T mode = (v == exposureMode.getMax()) ? MMAL_PARAM_EXPOSUREMODE_MAX : v;
    MMAL_PARAMETER_EXPOSUREMODE_T exp_mode = {{MMAL_PARAMETER_EXPOSURE_MODE,sizeof(exp_mode)}, mode};
    
    if (!camera)
        return 1;
    
    mmal_status_to_int(mmal_port_parameter_set(camera->control, &exp_mode.hdr));
#endif
}
void ofxPiCam::setExposureMeteringMode(int & v){
#ifdef TARGET_RASPBERRY_PI


    MMAL_PARAM_EXPOSUREMETERINGMODE_T mode = (v == exposureMeteringMode.getMax()) ? MMAL_PARAM_EXPOSUREMETERINGMODE_MAX : v;
    MMAL_PARAMETER_EXPOSUREMETERINGMODE_T meter_mode = {{MMAL_PARAMETER_EXP_METERING_MODE,sizeof(meter_mode)},
        m_mode};
    if (!camera)
        return 1;
    
    mmal_status_to_int(mmal_port_parameter_set(camera->control, &meter_mode.hdr));
#endif
}
void ofxPiCam::setAWBMode(int & v){
#ifdef TARGET_RASPBERRY_PI
    
    MMAL_PARAM_AWBMODE_T awb_mode = (v == awbMode.getMax()) ? MMAL_PARAM_AWBMODE_MAX : v;
    MMAL_PARAMETER_AWBMODE_T param = {{MMAL_PARAMETER_AWB_MODE,sizeof(param)}, awb_mode};
    
    if (!camera)
        return 1;
    
    mmal_status_to_int(mmal_port_parameter_set(camera->control, &param.hdr));
#endif
}
void ofxPiCam::setFlickerAvoidMode(int & v){
#ifdef TARGET_RASPBERRY_PI
    
    MMAL_PARAM_FLICKERAVOID_T flicker_mode = (v == flickerAvoidMode.getMax()) ? MMAL_PARAM_FLICKERAVOID_MAX : v;
    MMAL_PARAMETER_FLICKERAVOID_T param = {{MMAL_PARAMETER_FLICKERAVOID_T,sizeof(param)}, flicker_mode};
    
    if (!camera)
        return 1;
    
    mmal_status_to_int(mmal_port_parameter_set(camera->control, &param.hdr));
#endif
}


void ofxPiCam::initLabels() {

    exposureMeteringModeLabels[0] = "average";
    exposureMeteringModeLabels[1] = "spot";
    exposureMeteringModeLabels[2] = "backlit";
    exposureMeteringModeLabels[3] = "matrix";
    exposureMeteringModeLabels[4] = "max";

    exposureModeLabels[0] = "off";
    exposureModeLabels[1] = "auto";
    exposureModeLabels[2] = "night";
    exposureModeLabels[3] = "night preview";
    exposureModeLabels[4] = "backlight";
    exposureModeLabels[5] = "spotlight";
    exposureModeLabels[6] = "sports";
    exposureModeLabels[7] = "snow";
    exposureModeLabels[8] = "beach";
    exposureModeLabels[9] = "very long";
    exposureModeLabels[10] = "fixed fps";
    exposureModeLabels[11] = "antishake";
    exposureModeLabels[12] = "fireworks";
    exposureModeLabels[13] = "max";


    drcModeLabels[0] = "off";
    drcModeLabels[1] = "low";
    drcModeLabels[2] = "medium";
    drcModeLabels[3] = "high";
    drcModeLabels[4] = "max";

    flickerAvoidModeLabels[0] = "off";
    flickerAvoidModeLabels[1] = "auto";
    flickerAvoidModeLabels[2] = "50hz";
    flickerAvoidModeLabels[3] = "60hz";
    flickerAvoidModeLabels[4] = "max";


    awbModeLabels[0] = "off";
    awbModeLabels[1] = "auto";
    awbModeLabels[2] = "sun";
    awbModeLabels[3] = "cloudy";
    awbModeLabels[4] = "shade";
    awbModeLabels[5] = "tungsten";
    awbModeLabels[6] = "fluorescent";
    awbModeLabels[7] = "incandescent";
    awbModeLabels[8] = "flash";
    awbModeLabels[9] = "horizon";
    awbModeLabels[10] = "greyworld (max)";
    awbModeLabels[11] = "max?";

    imageFXLabelLabels[ 0] = "none";
    imageFXLabelLabels[ 1] = "negative";
    imageFXLabelLabels[ 2] = "solarize";
    imageFXLabelLabels[ 3] = "posterize";
    imageFXLabelLabels[ 4] = "whiteboard";
    imageFXLabelLabels[ 5] = "blackboard";
    imageFXLabelLabels[ 6] = "sketch";
    imageFXLabelLabels[ 7] = "denoise";
    imageFXLabelLabels[ 8] = "emboss";
    imageFXLabelLabels[ 9] = "oilpaint";
    imageFXLabelLabels[10] = "hatch";
    imageFXLabelLabels[11] = "gpen";
    imageFXLabelLabels[12] = "pastel";
    imageFXLabelLabels[13] = "whatercolour";
    imageFXLabelLabels[14] = "film";
    imageFXLabelLabels[15] = "blur";
    imageFXLabelLabels[16] = "saturation";
    imageFXLabelLabels[17] = "colour swap";
    imageFXLabelLabels[18] = "washedout";
    imageFXLabelLabels[19] = "posterize";
    imageFXLabelLabels[20] = "colour point";
    imageFXLabelLabels[21] = "colour balance";
    imageFXLabelLabels[22] = "cartoon";
    imageFXLabelLabels[23] = "max";
}

