/************************************************************
************************************************************/
#include "th_fft.h"

/************************************************************
************************************************************/

/******************************
******************************/
THREAD_FFT::THREAD_FFT()
: N(AUDIO_BUF_SIZE)
, LastInt(0)
{
	/********************
	********************/
	/* 窓関数 */
	fft_window.resize(N);
	for(int i = 0; i < N; i++)	fft_window[i] = 0.5 - 0.5 * cos(2 * PI * i / N);
	
	sintbl.resize(N + N/4);
	bitrev.resize(N);
	
	make_bitrev();
	make_sintbl();
	
	/********************
	********************/
	setup();
}

/******************************
******************************/
THREAD_FFT::~THREAD_FFT()
{
}

/******************************
******************************/
void THREAD_FFT::threadedFunction()
{
	while(isThreadRunning()) {
		lock();
		
		unlock();
		
		
		sleep(1);
	}
}

/******************************
******************************/
void THREAD_FFT::exit()
{
}

/******************************
******************************/
void THREAD_FFT::setup()
{
	this->lock();
	for(int i = 0; i < AUDIO_BUF_SIZE; i++){
		Gain_Monitor[i] = 0;
		Gain_LineIn[i] = 0;
		Gain_Environment[i] = 0;
		Adjusted__Gain_Monitor[i] = 0;
		Adjusted__Gain_LineIn[i] = 0;
	}
	this->unlock();
}

/******************************
******************************/
float THREAD_FFT::get_AdjustGain(int freq_id)
{
	float Gain = Gui_Global->gui__AdjustGain;
	
	for(int i = 0; i < NUM__FOCUSED_GAIN_TO_DETECT_CLAP; i++){
		if(Gui_Global->gui__FocusedGain_FreqWidth[i] != 0){
			int freq_from = Gui_Global->gui__FocusedGain_FreqFrom[i];
			int freq_to =  Gui_Global->gui__FocusedGain_FreqFrom[i] + Gui_Global->gui__FocusedGain_FreqWidth[i] - 1;
			
			if( (freq_from <= freq_id) && (freq_id <= freq_to) ){
				Gain = Gui_Global->gui__FocusedGain[i];
			}
		}
	}
	
	return Gain;
}

/******************************
******************************/
void THREAD_FFT::update()
{
	this->lock();
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		Adjusted__Gain_Monitor[i] = Gain_Monitor[i];
		Adjusted__Gain_LineIn[i] = Gain_LineIn[i] * get_AdjustGain(i);
		
		Gain_Environment[i] = Adjusted__Gain_Monitor[i] - Adjusted__Gain_LineIn[i];
	}
	
	this->unlock();
}

/******************************
******************************/
double THREAD_FFT::get_LevOfEnv_L()
{
	// return get_max_of_Env(Gui_Global->gui__Clap_LowFreq_FftFreq_From, Gui_Global->gui__Clap_LowFreq_FftFreq_To);
	return get_ave_of_Env(Gui_Global->gui__Clap_LowFreq_FftFreq_From, Gui_Global->gui__Clap_LowFreq_FftFreq_To);
	// return get_ave_of_Env_around_max(Gui_Global->gui__Clap_LowFreq_FftFreq_From, Gui_Global->gui__Clap_LowFreq_FftFreq_To);
}

/******************************
******************************/
double THREAD_FFT::get_LevOfEnv_H()
{
	// return get_max_of_Env(Gui_Global->gui__Clap_HighFreq_FftFreq_From, Gui_Global->gui__Clap_HighFreq_FftFreq_To);
	return get_ave_of_Env(Gui_Global->gui__Clap_HighFreq_FftFreq_From, Gui_Global->gui__Clap_HighFreq_FftFreq_To);
}

/******************************
******************************/
bool THREAD_FFT::IsInMaskedFreq(int freq_id)
{
	for(int i = 0; i < NUM__FREQ_MASKS_TO_DETECT_CLAP; i++){
		if(Gui_Global->gui__ClapMask_FreqWidth[i] != 0){
			int Freqfrom	= Gui_Global->gui__ClapMask_FreqFrom[i];
			int FreqTo		= Gui_Global->gui__ClapMask_FreqFrom[i] + Gui_Global->gui__ClapMask_FreqWidth[i] - 1;
			
			if((Freqfrom <= freq_id) && (freq_id <= FreqTo)){
				return true;
			}
		}
	}
	
	return false;
}

/******************************
******************************/
double THREAD_FFT::get_max_of_Env(int from, int to)
{
	/********************
	********************/
	if((from < 0) || (AUDIO_BUF_SIZE/2 <= from) || (to < 0) || (AUDIO_BUF_SIZE/2 <= to)){
		ERROR_MSG();
		std::exit(1);
	}
	
	/********************
	from - toが全てmasked areaの場合、-1に張り付く.
	********************/
	double ret = -1;
	for(int i = from; i <= to; i++){
		if(ret < Gain_Environment[i]){ // IsInMaskedFreq()はcostが高いので、こちらを先に判別.
			if(!IsInMaskedFreq(i)) ret = Gain_Environment[i];
		}
	}
	
	return ret;
}

/******************************
******************************/
double THREAD_FFT::get_ave_of_Env_around_max(int from, int to)
{
	/********************
	********************/
	if((from < 0) || (AUDIO_BUF_SIZE/2 <= from) || (to < 0) || (AUDIO_BUF_SIZE/2 <= to)){
		ERROR_MSG();
		std::exit(1);
	}
	
	/********************
	********************/
	if(to <= from) return 0;
	
	/********************
	********************/
	int freq_id_of_max = -1;
	{
		double max_val = -1;
		for(int i = from; i <= to; i++){
			if(max_val < Gain_Environment[i]){// IsInMaskedFreq()はcostが高いので、こちらを先に判別.
				if(!IsInMaskedFreq(i)){
					max_val = Gain_Environment[i];
					freq_id_of_max = i;
				}
			}
		}
	}
	
	if(freq_id_of_max == -1) return 0;
	
	/********************
	********************/
	int freq_width = (to - from) * 3 / 5;
	
	int _from = freq_id_of_max - freq_width;
	if(_from < from)	from = from;
	else				from = _from;
	
	int _to = freq_id_of_max + freq_width;
	if(to < _to)	to = to;
	else			to = _to;
	
	/********************
	********************/
	double sum = 0;
	int num = 0;
	
	for(int i = from; i <= to; i++){
		if(!IsInMaskedFreq(i)){
			sum += Gain_Environment[i];
			num++;
		}
	}
	
	if(num == 0)	return 0;
	else			return sum / num;
}

/******************************
******************************/
double THREAD_FFT::get_ave_of_Env(int from, int to)
{
	/********************
	********************/
	if((from < 0) || (AUDIO_BUF_SIZE/2 <= from) || (to < 0) || (AUDIO_BUF_SIZE/2 <= to)){
		ERROR_MSG();
		std::exit(1);
	}
	
	/********************
	********************/
	if(to <= from) return 0;
	
	/********************
	********************/
	double sum = 0;
	int num = 0;
	
	for(int i = from; i <= to; i++){
		if(!IsInMaskedFreq(i)){
			sum += Gain_Environment[i];
			num++;
		}
	}
	
	if(num == 0)	return 0;
	else			return sum / num;
}

/******************************
******************************/
void THREAD_FFT::get_ParamToDraw(double DispMonitor[], double DispLinein[], double DispEnvironment_L[], double DispEnvironment_H[], int size)
{
	if(size < AUDIO_BUF_SIZE/2) return;
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		DispMonitor[i] = ofMap(Adjusted__Gain_Monitor[i], 0, Gui_Global->gui__DispMax_GainMonitor, 0, ofGetHeight()/NUM_SPLIT_DISP);
		DispLinein[i] = ofMap(Adjusted__Gain_LineIn[i], 0, Gui_Global->gui__DispMax_GainMonitor, 0, ofGetHeight()/NUM_SPLIT_DISP);
		
		DispEnvironment_L[i] = ofMap(Gain_Environment[i], -Gui_Global->gui__DispMax_GainEnv_LowFreq, Gui_Global->gui__DispMax_GainEnv_LowFreq, -ofGetHeight()/NUM_SPLIT_DISP, ofGetHeight()/NUM_SPLIT_DISP);
		DispEnvironment_H[i] = ofMap(Gain_Environment[i], -Gui_Global->gui__DispMax_GainEnv_HighFreq, Gui_Global->gui__DispMax_GainEnv_HighFreq, -ofGetHeight()/NUM_SPLIT_DISP, ofGetHeight()/NUM_SPLIT_DISP);
	}
}

/******************************
******************************/
void THREAD_FFT::update__Gain(const vector<float> &AudioSample_L, const vector<float> &AudioSample_R)
{
	this->lock();
	
	float now = ofGetElapsedTimef();
	
	AudioSample_fft_LPF_saveToArray(AudioSample_L, Gain_LineIn, now - LastInt);
	AudioSample_fft_LPF_saveToArray(AudioSample_R, Gain_Monitor, now - LastInt);
	
	LastInt = now;
	
	this->unlock();
}

/******************************
******************************/
void THREAD_FFT::AudioSample_fft_LPF_saveToArray(const vector<float> &AudioSample, double Gain[], float dt)
{
	/********************
	********************/
	if( AudioSample.size() != N ) { ERROR_MSG(); std::exit(1); }
	
	/********************
	********************/
	double x[N], y[N];
	
	for(int i = 0; i < N; i++){
		x[i] = AudioSample[i] * fft_window[i];
		y[i] = 0;
	}
	
	fft(x, y);

	Gain[0] = 0;
	Gain[N/2] = 0;
	for(int i = 1; i < N/2; i++){
		double GainTemp = sqrt(x[i] * x[i] + y[i] * y[i]);
		
		double Alpha;
		if((Gui_Global->gui__FtfGain_LPF_dt == 0) || (Gui_Global->gui__FtfGain_LPF_dt < dt) )	Alpha = 1;
		else																					Alpha = 1/Gui_Global->gui__FtfGain_LPF_dt * dt;
		
		Gain[i] = GainTemp * Alpha + Gain[i] * (1 - Alpha);
		Gain[N - i] = Gain[i]; // 共役(yの正負反転)だが、Gainは同じ
	}
}

/******************************
******************************/
int THREAD_FFT::fft(double x[], double y[], int IsReverse)
{
	/*****************
		bit反転
	*****************/
	int i, j;
	for(i = 0; i < N; i++){
		j = bitrev[i];
		if(i < j){
			double t;
			t = x[i]; x[i] = x[j]; x[j] = t;
			t = y[i]; y[i] = y[j]; y[j] = t;
		}
	}

	/*****************
		変換
	*****************/
	int n4 = N / 4;
	int k, ik, h, d, k2;
	double s, c, dx, dy;
	for(k = 1; k < N; k = k2){
		h = 0;
		k2 = k + k;
		d = N / k2;

		for(j = 0; j < k; j++){
			c = sintbl[h + n4];
			if(IsReverse)	s = -sintbl[h];
			else			s = sintbl[h];

			for(i = j; i < N; i += k2){
				ik = i + k;
				dx = s * y[ik] + c * x[ik];
				dy = c * y[ik] - s * x[ik];

				x[ik] = x[i] - dx;
				x[i] += dx;

				y[ik] = y[i] - dy;
				y[i] += dy;
			}
			h += d;
		}
	}

	/*****************
	*****************/
	if(!IsReverse){
		for(i = 0; i < N; i++){
			x[i] /= N;
			y[i] /= N;
		}
	}

	return 0;
}

/******************************
******************************/
void THREAD_FFT::make_bitrev(void)
{
	int i, j, k, n2;

	n2 = N / 2;
	i = j = 0;

	for(;;){
		bitrev[i] = j;
		if(++i >= N)	break;
		k = n2;
		while(k <= j)	{j -= k; k /= 2;}
		j += k;
	}
}

/******************************
******************************/
void THREAD_FFT::make_sintbl(void)
{
	for(int i = 0; i < N + N/4; i++){
		sintbl[i] = sin(2 * PI * i / N);
	}
}


