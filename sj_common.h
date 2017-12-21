/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "stdio.h"

#include "ofMain.h"
#include "ofxGui.h"

/************************************************************
************************************************************/
// #define SJ_RELEASE

/************************************************************
************************************************************/
#define ERROR_MSG(); printf("Error in %s:%d\n", __FILE__, __LINE__);

enum{
	WIDTH = 1300,
	HEIGHT = 720,
};

enum{
	BUF_SIZE = 1000,
	LARGE_BUF_SIZE = 6000,
};

enum{
	AUDIO_BUF_SIZE = 512,
	
	AUDIO_BUFFERS = 2,
	AUDIO_SAMPLERATE = 44100,
};

/************************************************************
************************************************************/

/**************************************************
**************************************************/
class Noncopyable{
protected:
	Noncopyable() {}
	~Noncopyable() {}

private:
	void operator =(const Noncopyable& src);
	Noncopyable(const Noncopyable& src);
};

/**************************************************
**************************************************/
class GUI_GLOBAL{
private:
	/****************************************
	****************************************/
	
public:
	/****************************************
	****************************************/
	void setup(string GuiName, string FileName = "gui.xml", float x = 10, float y = 10);
	
	/****************************************
	****************************************/
	ofxPanel gui;
	
	/* */
	ofxFloatSlider gui__FtfGain_LPF_dt;
	
	/* */
	ofxGuiGroup GuiGroup_GainAdjust;
	ofxFloatSlider gui__AdjustGain;
	
	/* */
	ofxGuiGroup GuiGroup_DetectClap_LowFreq;
	ofxFloatSlider gui__Clap_LowFreq_Thresh_L;
	ofxFloatSlider gui__Clap_LowFreq_Thresh_H;
	
	ofxIntSlider gui__Clap_LowFreq_FftFreq_From;
	ofxIntSlider gui__Clap_LowFreq_FftFreq_To;
	
	/* */
	ofxGuiGroup GuiGroup_DetectClap_HighFreq;
	ofxFloatSlider gui__Clap_HighFreq_Thresh_L;
	ofxFloatSlider gui__Clap_HighFreq_Thresh_H;
	
	ofxIntSlider gui__Clap_HighFreq_FftFreq_From;
	ofxIntSlider gui__Clap_HighFreq_FftFreq_To;
	
	/* */
	ofxGuiGroup GuiGroup_Graph;
	ofxFloatSlider gui__Graph_ofs_x;
	ofxFloatSlider gui__w_Graph_EnvL;
	
	ofxFloatSlider gui__Disp_FftGainMax_Monitor;
	
	ofxFloatSlider gui__Disp_FftGainMax_Diff_LowFreq;
	ofxFloatSlider gui__Disp_FftGainMax_Diff_HighFreq;
	
	ofxFloatSlider gui__Graph_space;
	ofxFloatSlider gui__Graph_w;
};

/************************************************************
************************************************************/
extern GUI_GLOBAL* Gui_Global;

extern int GPIO_0;
extern int GPIO_1;

/************************************************************
************************************************************/


