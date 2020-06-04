#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    cam.setup(320,240,true); 
    showGui = false;

}

//--------------------------------------------------------------
void ofApp::update(){
    if (cam.isFrameNew()) {
        frame = cam.grab();
    	if ( !tex.isAllocated() || ( ( tex.getWidth() != frame.getWidth() ) || ( tex.getHeight() != frame.getHeight() ) ) ) {
            ofLog() << "allocated texture";
    		tex.allocate( frame.getWidth(), frame.getHeight(), GL_RGB);
    	}	
        tex.loadData( frame.getData() );
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofImage img;
    img.setFromPixels( cam.grab() );
    int w = ofGetWidth();
    int h = ofGetHeight();
    int ww = w/2;
    int hh = (ww/cam.getWidth())*cam.getHeight();
    img.draw(0,0,ww,hh);
    tex.draw(ww,0,ww,hh);
    if (showGui) gui.draw();
}

void setView( ofParameterGroup & g ) {
    gui.clear();
    gui.setup( g );
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
    
    if (key == ' ') showGui = !showGui;

    if (key == '1') setView(cam.groupA);
    if (key == '2') setView(cam.groupB);
    if (key == '3') setView(cam.groupC);
    if (key == '4') setView(cam.groupD);
}
