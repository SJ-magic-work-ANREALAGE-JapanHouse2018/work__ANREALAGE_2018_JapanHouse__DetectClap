/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"

#include "sj_common.h"
#include "th_fft.h"
#include "sj_OSC.h"

/************************************************************
************************************************************/

/**************************************************
**************************************************/
struct AUDIO_SAMPLE{
	vector<float> Left;
	vector<float> Right;
	
	void resize(int size){
		Left.resize(size);
		Right.resize(size);
	}
};

/**************************************************
**************************************************/
struct VBO_SET{
	ofVbo Vbo;
	vector<ofVec3f> VboVerts;
	vector<ofFloatColor> VboColor;
	
	void setup(int size){
		VboVerts.resize(size);
		VboColor.resize(size);
		
		Vbo.setVertexData(&VboVerts[0], VboVerts.size(), GL_DYNAMIC_DRAW);
		Vbo.setColorData(&VboColor[0], VboColor.size(), GL_DYNAMIC_DRAW);
	}
	
	void set_singleColor(const ofColor& color){
		for(int i = 0; i < VboColor.size(); i++) { VboColor[i].set( double(color.r)/255, double(color.g)/255, double(color.b)/255, double(color.a)/255); }
	}
	
	void update(){
		Vbo.updateVertexData(&VboVerts[0], VboVerts.size());
		Vbo.updateColorData(&VboColor[0], VboColor.size());
	}
	
	void draw(int drawMode){
		Vbo.draw(drawMode, 0, VboVerts.size());
	}
	
	void draw(int drawMode, int total){
		if(VboVerts.size() < total) total = VboVerts.size();
		Vbo.draw(drawMode, 0, total);
	}
};


/**************************************************
**************************************************/
class ofApp : public ofBaseApp{
private:
	/****************************************
	****************************************/
	enum{
		NUM_TIME_POINTS = 1200,
	};
	
	enum{
		NUM_AUTOGAIN_LOOPS = 10,
	};
	
	enum STATE_CLAP{
		STATE_CLAP_WAIT,
		STATE_CLAP_ECHO,
	};
	
	enum{
		FONT_S,
		FONT_M,
		FONT_L,
		
		NUM_FONT_SIZE,
	};
	
	/****************************************
	parameter
	****************************************/
	FILE* fp;
	
	/********************
	********************/
	int soundStream_Input_DeviceId;
	int soundStream_Output_DeviceId;
	ofSoundStream soundStream;
	
	AUDIO_SAMPLE AudioSample;
	
	OSC_TARGET Osc_video;
	
	/********************
	********************/
	/*
	bool b_Calib;
	bool b_Calib_start;
	int c_CalibedLoop;
	int id_AutoAdjust_Gain;
	
	vector<double> DiffGain_Resluts;
	double Optimum_AdjustGain_Candidates[NUM_AUTOGAIN_LOOPS];

	bool b_Draw_DiffOfGain;
	*/
	
	/********************
	********************/
	bool b_DispGui;
	bool b_PauseGraph;
	
	THREAD_FFT* fft_thread;
	
	STATE_CLAP StateClap_L;
	STATE_CLAP StateClap_H;
	STATE_CLAP StateClap_AND;
	
	ofTrueTypeFont font[NUM_FONT_SIZE];
	
	/********************
	********************/
	ofSoundPlayer music;
	
	/********************
	Graph
	********************/
	double DispMonitor[AUDIO_BUF_SIZE/2];
	double DispLinein[AUDIO_BUF_SIZE/2];
	double DispEnvironment_L[AUDIO_BUF_SIZE/2];
	double DispEnvironment_H[AUDIO_BUF_SIZE/2];
	
	double Lev_OfEnvironment_L;
	double Lev_OfEnvironment_H;
	
	VBO_SET Vboset_Monitor;
	VBO_SET Vboset_Linein;
	VBO_SET Vboset_Env_L;
	VBO_SET Vboset_Env_H;
	VBO_SET Vboset_Clap_L;
	VBO_SET Vboset_Clap_H;
	VBO_SET Vboset_Clap_AND;
	
	int png_id;
	
	/****************************************
	method
	****************************************/
	/********************
	gui
	********************/
	void setup_Gui();
	void Guis_LoadSetting();
	bool checkif_FileExist(const char* FileName);
	void drawGuis();
	void remove_xml();
	
	/********************
	********************/
	void audioIn(float *input, int bufferSize, int nChannels);
	void audioOut(float *output, int bufferSize, int nChannels);
	
	void ReStart();
	
	void StateChart_Clap_LH(STATE_CLAP& StateClap, float thresh_L, float thresh_H, float Lev_OfEnvironment);
	void StateChart_Clap_AND();
	
	/********************
	********************/
	/*
	void StartStop_AutoCalibration();
	void AutoCalibration_SaveResult();
	void Judge_and_Fix_OptimumGain();
	void cal_BestGain_of_this_Loop();
	*/
	
	/********************
	Graph
	********************/
	void RefreshVerts();
	void Refresh_BarColor();
	
	void draw_monitor();
	void draw_Env_L();
	void draw_Env_H();
	void draw_StateChart_Clap();
	
	void draw_ClapThresh(float y_threshL, float y_threshH, float Disp_x_max, float map_Disp_y_Max);
	void draw_LevOfEnv(int x_from, int x_to, float Lev_OfEnvironment, float Disp_x_max, float map_Disp_y_max);
	
	void draw_time();
		
public:
	/****************************************
	****************************************/
	ofApp(int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId);
	~ofApp();
	
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
};
