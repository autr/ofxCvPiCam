#pragma once

#include "ofMain.h"
#include "ofxPiCam.h"
#include "ofxGui.h"
class ofApp : public ofBaseApp{

	public:

        void setup();
        void update();
        void draw();
        void keyPressed(int key);
        void setView( ofParameterGroup & g );
		

        bool showGui;
        ofxPiCam cam;

        ofxPanel gui;

    
        ofPixels frame;
        ofTexture tex;
    

};

