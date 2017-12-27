#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( int argc, char** argv ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	/********************
	********************/
	int soundStream_Input_DeviceId;
	int soundStream_Output_DeviceId;

	/********************
	********************/
	if(argc < 3){
		/********************
		********************/
		printf("> exe AudioInput AudioOutput\n");
		
		/********************
		********************/
		soundStream_Input_DeviceId = -1;
		soundStream_Output_DeviceId = -1;
		
		ofRunApp(new ofApp(soundStream_Input_DeviceId, soundStream_Output_DeviceId));
		
	}else{
		/********************
		********************/
		soundStream_Input_DeviceId = atoi(argv[1]);
		soundStream_Output_DeviceId = atoi(argv[2]);
		
		ofRunApp(new ofApp(soundStream_Input_DeviceId, soundStream_Output_DeviceId));
	}
}
