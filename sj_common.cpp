/************************************************************
************************************************************/
#include "sj_common.h"

/************************************************************
************************************************************/
int GPIO_0 = 0;
int GPIO_1 = 0;

/************************************************************
************************************************************/
GUI_GLOBAL* Gui_Global;


/************************************************************
************************************************************/

/******************************
******************************/
void GUI_GLOBAL::setup(string GuiName, string FileName, float x, float y)
{
	/********************
	********************/
	gui.setup(GuiName.c_str(), FileName.c_str(), x, y);
	
	/********************
	********************/
	gui.add(gui__FtfGain_LPF_dt.setup("LPF", 0.2, 0, 1.0));
	
	GuiGroup_GainAdjust.setup("Adjust Gain");
		GuiGroup_GainAdjust.add(gui__AdjustGain.setup("Adjust Gain", 1.0, 0.1, 3.0));
	gui.add(&GuiGroup_GainAdjust);
	
	GuiGroup_DetectClap_LowFreq.setup("Detect Clap Low");
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_Thresh_H.setup("Thresh H", 0.001, 0, 0.01));
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_Thresh_L.setup("Thresh L", 0.00075, 0, 0.01));
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_FftFreq_From.setup("Freq From", 6, 0, 255));
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_FftFreq_To.setup("Freq To", 35, 0, 255));
	gui.add(&GuiGroup_DetectClap_LowFreq);
	
	GuiGroup_DetectClap_HighFreq.setup("Detect Clap High");
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_Thresh_H.setup("Thresh H", 5e-5, 0, 3e-4));
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_Thresh_L.setup("Thresh L", 3e-5, 0, 3e-4));
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_FftFreq_From.setup("Freq From", 165, 0, 255));
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_FftFreq_To.setup("Freq To", 225, 0, 255));
	gui.add(&GuiGroup_DetectClap_HighFreq);
	
	GuiGroup_Graph.setup("Graph");
		GuiGroup_Graph.add(gui__Graph_ofs_x.setup("ofs x", 10, 10, ofGetWidth()));
		GuiGroup_Graph.add(gui__w_Graph_EnvL.setup("w EnvL", 450, 10, ofGetWidth()));
		
		GuiGroup_Graph.add(gui__Disp_FftGainMax_Monitor.setup("DispMax:monitor", 0.02, 0, 0.05));
		GuiGroup_Graph.add(gui__Disp_FftGainMax_Diff_LowFreq.setup("DispMax:diff_L", 0.01, 0, 0.05));
		GuiGroup_Graph.add(gui__Disp_FftGainMax_Diff_HighFreq.setup("DispMax:diff_H", 4e-4, 0, 0.001));
		GuiGroup_Graph.add(gui__Graph_space.setup("space", 5, 1, 30));
		GuiGroup_Graph.add(gui__Graph_w.setup("width", 2, 1, 10));
	gui.add(&GuiGroup_Graph);
}


