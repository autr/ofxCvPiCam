#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(30);
    cam.setup(640,480,true); 
    showGui = false;

}

//--------------------------------------------------------------
void ofApp::update(){
    	if (  tex.getWidth() != cam.width || tex.getHeight() != cam.height ) {
            ofLog() << "allocated texture???";
    		tex.allocate( cam.width, cam.height, GL_RGB);
    	}	
        tex.loadData( cam.grab().getData(), cam.width, cam.height, GL_RGB );
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    if (cam.isReceiving) {
        int w = ofGetWidth();
        int h = ofGetHeight();
        int ww = w/2;
        int hh = (ww/cam.width)*cam.height;
        tex.draw(0,0,w,h);
    }
    if (showGui) gui.draw();
}

void ofApp::setView( ofParameterGroup & g ) {
    
    gui.clear();
    gui.setup( g );
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
    
    if (key == ' ') showGui = !showGui;
    ofLog() << "Key pressed: " << (char)key;
    if (key == '1') setView(cam.groupA);
    if (key == '2') setView(cam.groupB);
    if (key == '3') setView(cam.groupC);
    if (key == '4') setView(cam.groupD);
}
