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
		
		GuiGroup_GainAdjust.add(gui__FocusedGain[0].setup("0:FocusedGain", 0.01, 0, 1.0));
		GuiGroup_GainAdjust.add(gui__FocusedGain_FreqFrom[0].setup("0:Freq From", 175, 0, 255));
		GuiGroup_GainAdjust.add(gui__FocusedGain_FreqWidth[0].setup("0:Width", 50, 0, 255));
		GuiGroup_GainAdjust.add(gui__FocusedGain[1].setup("1:FocusedGain", 1.0, 0, 1.0));
		GuiGroup_GainAdjust.add(gui__FocusedGain_FreqFrom[1].setup("1:Freq From", 255, 0, 255));
		GuiGroup_GainAdjust.add(gui__FocusedGain_FreqWidth[1].setup("1:Width", 0, 0, 255));
	gui.add(&GuiGroup_GainAdjust);
	
	GuiGroup_DetectClap_LowFreq.setup("Detect Clap Low");
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_Thresh_H.setup("Thresh H", 0.001, 0, 0.01));
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_Thresh_L.setup("Thresh L", 0.00075, 0, 0.01));
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_FftFreq_From.setup("Freq From", 12, 0, 255));
		GuiGroup_DetectClap_LowFreq.add(gui__Clap_LowFreq_FftFreq_To.setup("Freq To", 40, 0, 255));
		GuiGroup_DetectClap_LowFreq.add(gui__DeltaClap_LowFreq_Thresh_H.setup("d Thresh H", 0.015, 0, 0.1));
	gui.add(&GuiGroup_DetectClap_LowFreq);
	
	GuiGroup_DetectClap_HighFreq.setup("Detect Clap High");
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_Thresh_H.setup("Thresh H", 2e-5, 0, 3e-4));
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_Thresh_L.setup("Thresh L", 1.5e-5, 0, 3e-4));
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_FftFreq_From.setup("Freq From", 175, 0, 255));
		GuiGroup_DetectClap_HighFreq.add(gui__Clap_HighFreq_FftFreq_To.setup("Freq To", 225, 0, 255));
		GuiGroup_DetectClap_HighFreq.add(gui__DeltaClap_HighFreq_Thresh_H.setup("d Thresh H", 0.0002, 0, 0.01));
	gui.add(&GuiGroup_DetectClap_HighFreq);
	
	GuiGroup_DetectClap_Mask.setup("Detect Clap Mask");
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[0].setup("0:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[0].setup("0:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[1].setup("1:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[1].setup("1:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[2].setup("2:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[2].setup("2:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[3].setup("3:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[3].setup("3:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[4].setup("4:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[4].setup("4:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[5].setup("5:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[5].setup("5:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[6].setup("6:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[6].setup("6:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[7].setup("7:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[7].setup("7:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[8].setup("8:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[8].setup("8:Width", 0, 0, 40));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqFrom[9].setup("9:Freq From", 0, 0, 255));
		GuiGroup_DetectClap_Mask.add(gui__ClapMask_FreqWidth[9].setup("9:Width", 0, 0, 40));
	gui.add(&GuiGroup_DetectClap_Mask);
	GuiGroup_DetectClap_Mask.minimize();
	
	GuiGroup_Graph.setup("Graph");
		GuiGroup_Graph.add(gui__Graph_ofs_x.setup("ofs x", 10, 10, ofGetWidth()));
		GuiGroup_Graph.add(gui__w_Graph_EnvL.setup("w EnvL", 450, 10, ofGetWidth()));
		
		GuiGroup_Graph.add(gui__DispMax_GainMonitor.setup("Disp:monitor", 0.02, 0, 0.05));
		GuiGroup_Graph.add(gui__DispMax_GainEnv_LowFreq.setup("Disp:Env_L", 0.01, 0, 0.05));
		GuiGroup_Graph.add(gui__DispMax_GainEnv_HighFreq.setup("Disp:Env_H", 1e-4, 0, 0.001));
		GuiGroup_Graph.add(gui__DispMax_DeltaEnv_L.setup("Disp:dEnv_L", 0.06, 0, 0.5));
		GuiGroup_Graph.add(gui__DispMax_DeltaEnv_H.setup("Disp:dEnv_H", 0.0005, 0, 0.05));
		GuiGroup_Graph.add(gui__Graph_space.setup("space", 5, 1, 30));
		GuiGroup_Graph.add(gui__Graph_w.setup("width", 2, 1, 10));
	gui.add(&GuiGroup_Graph);
}


