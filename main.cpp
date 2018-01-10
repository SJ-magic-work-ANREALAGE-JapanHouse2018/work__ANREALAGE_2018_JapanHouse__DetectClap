#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( int argc, char** argv ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	/********************
	********************/
	int BootMode;
	int soundStream_Input_DeviceId;
	int soundStream_Output_DeviceId;

	/********************
	********************/
	if(argc < 4){
		/********************
		********************/
		printf("> exe BootMode AudioInput AudioOutput\n");
		printf("> BootMode\n");
		printf("\t0:Release\n");
		printf("\t1:Debug\n");
		
		/********************
		********************/
		BootMode = 0;
		soundStream_Input_DeviceId = -1;
		soundStream_Output_DeviceId = -1;
		
		ofRunApp(new ofApp(BootMode, soundStream_Input_DeviceId, soundStream_Output_DeviceId));
		
	}else{
		/********************
		********************/
		BootMode = atoi(argv[1]);
		soundStream_Input_DeviceId = atoi(argv[2]);
		soundStream_Output_DeviceId = atoi(argv[3]);
		
		ofRunApp(new ofApp(BootMode, soundStream_Input_DeviceId, soundStream_Output_DeviceId));
	}
}
