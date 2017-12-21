/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"

#include "sj_common.h"

/************************************************************
************************************************************/

/**************************************************
**************************************************/
class THREAD_FFT : public ofThread, private Noncopyable{
private:
	/****************************************
	****************************************/
	/********************
	********************/
	double Gain_Monitor[AUDIO_BUF_SIZE];
	double Gain_LineIn[AUDIO_BUF_SIZE];
	double Gain_Environment[AUDIO_BUF_SIZE];
	double Adjusted__Gain_Monitor[AUDIO_BUF_SIZE];
	double Adjusted__Gain_LineIn[AUDIO_BUF_SIZE];
	
	const int N;
	
	vector<float> fft_window;
	vector<double> sintbl;
	vector<int> bitrev;
	
	float LastInt;
	
	/****************************************
	function
	****************************************/
	/********************
	singleton
	********************/
	THREAD_FFT();
	~THREAD_FFT();
	THREAD_FFT(const THREAD_FFT&); // Copy constructor. no define.
	THREAD_FFT& operator=(const THREAD_FFT&); // コピー代入演算子. no define.
	
	/********************
	********************/
	void threadedFunction();
	
	int fft(double x[], double y[], int IsReverse = false);
	void make_bitrev(void);
	void make_sintbl(void);
	
	double get_max_of_Env(int from, int to);
	double get_ave_of_Env(int from, int to);
	void AudioSample_fft_LPF_saveToArray(const vector<float> &AudioSample, double Gain[], float dt);
	
public:
	/****************************************
	****************************************/
	/********************
	********************/
	static THREAD_FFT* getInstance(){
		static THREAD_FFT inst;
		return &inst;
	}
	
	void exit();
	void setup();
	void update();
	
	void update__Gain(const vector<float> &AudioSample_L, const vector<float> &AudioSample_R);
	double get_LevOfEnv_L();
	double get_LevOfEnv_H();
	
	void get_ParamToDraw(double DispMonitor[], double DispLinein[], double DispEnvironment_L[], double DispEnvironment_H[], int size);
};



