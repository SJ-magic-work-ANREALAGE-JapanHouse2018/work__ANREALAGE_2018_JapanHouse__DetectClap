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
	HEIGHT = 900,
};

enum{
	NUM_SPLIT_DISP = 6,
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

enum{
	NUM__FOCUSED_GAIN_TO_DETECT_CLAP = 2,
	NUM__FREQ_MASKS_TO_DETECT_CLAP = 10,
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
	
	ofxFloatSlider gui__FocusedGain[NUM__FOCUSED_GAIN_TO_DETECT_CLAP];
	ofxIntSlider gui__FocusedGain_FreqFrom[NUM__FOCUSED_GAIN_TO_DETECT_CLAP];
	ofxIntSlider gui__FocusedGain_FreqWidth[NUM__FOCUSED_GAIN_TO_DETECT_CLAP];
	
	/* */
	ofxGuiGroup GuiGroup_DetectClap_LowFreq;
	ofxFloatSlider gui__Clap_LowFreq_Thresh_L;
	ofxFloatSlider gui__Clap_LowFreq_Thresh_H;
	
	ofxIntSlider gui__Clap_LowFreq_FftFreq_From;
	ofxIntSlider gui__Clap_LowFreq_FftFreq_To;
	
	ofxFloatSlider gui__DeltaClap_LowFreq_Thresh_H;
	
	/* */
	ofxGuiGroup GuiGroup_DetectClap_HighFreq;
	ofxFloatSlider gui__Clap_HighFreq_Thresh_L;
	ofxFloatSlider gui__Clap_HighFreq_Thresh_H;
	
	ofxIntSlider gui__Clap_HighFreq_FftFreq_From;
	ofxIntSlider gui__Clap_HighFreq_FftFreq_To;
	
	ofxFloatSlider gui__DeltaClap_HighFreq_Thresh_H;
	
	/* */
	ofxGuiGroup GuiGroup_DetectClap_Mask;
	
	ofxIntSlider gui__ClapMask_FreqFrom[NUM__FREQ_MASKS_TO_DETECT_CLAP];
	ofxIntSlider gui__ClapMask_FreqWidth[NUM__FREQ_MASKS_TO_DETECT_CLAP];
	
	
	/* */
	ofxGuiGroup GuiGroup_Graph;
	ofxFloatSlider gui__Graph_ofs_x;
	ofxFloatSlider gui__w_Graph_EnvL;
	
	ofxFloatSlider gui__DispMax_GainMonitor;
	
	ofxFloatSlider gui__DispMax_GainEnv_LowFreq;
	ofxFloatSlider gui__DispMax_GainEnv_HighFreq;
	
	ofxFloatSlider gui__DispMax_DeltaEnv_L;
	ofxFloatSlider gui__DispMax_DeltaEnv_H;
	
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


