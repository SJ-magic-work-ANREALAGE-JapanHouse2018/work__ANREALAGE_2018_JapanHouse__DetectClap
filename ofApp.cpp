/************************************************************
************************************************************/
#include "ofApp.h"
#include <time.h>

/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId)
: soundStream_Input_DeviceId(_soundStream_Input_DeviceId)
, soundStream_Output_DeviceId(_soundStream_Output_DeviceId)
, b_DispGui(true)
, fft_thread(THREAD_FFT::getInstance())
, StateClap_L(STATE_CLAP_WAIT)
, StateClap_H(STATE_CLAP_WAIT)
, StateClap_AND(STATE_CLAP_WAIT)
, png_id(0)
, b_PauseGraph(false)
, Osc_video("127.0.0.1", 12345, 12346)
{
	fp = fopen("../../../data/AutoCalib.csv", "w");
	
	/********************
	********************/
	srand((unsigned) time(NULL));
	
	/********************
	********************/
	font[FONT_S].load("RictyDiminished-Regular.ttf", 10, true, true, true);
	font[FONT_M].load("RictyDiminished-Regular.ttf", 20, true, true, true);
	font[FONT_L].load("RictyDiminished-Regular.ttf", 30, true, true, true);
	
	
	/********************
	********************/
	Vboset_Monitor.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Linein.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Env_L.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Env_H.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Clap_L.setup(NUM_TIME_POINTS);
	Vboset_Clap_H.setup(NUM_TIME_POINTS);
	Vboset_Clap_AND.setup(NUM_TIME_POINTS);
	
	/********************
	fft Gainは、都度 全ての値を算出するが、
	clap(state chart)の時間軸表示は、slideしながら表示なので、
	初期化が必要.
	********************/
	for(int i = 0; i < NUM_TIME_POINTS; i++){
		Vboset_Clap_L.VboVerts[i].set(i, 0);
		Vboset_Clap_H.VboVerts[i].set(i, 0);
		Vboset_Clap_AND.VboVerts[i].set(i, 0);
	}
	
	/********************
	********************/
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		DispMonitor[i] = 0;
		DispLinein[i] = 0;
		DispEnvironment_L[i] = 0;
		DispEnvironment_H[i] = 0;
	}
}

/******************************
******************************/
ofApp::~ofApp()
{
	fclose(fp);
}

/******************************
******************************/
void ofApp::exit()
{
	/********************
	ofAppとaudioが別threadなので、ここで止めておくのが安全.
	********************/
	soundStream.stop();
	soundStream.close();
	
	/********************
	********************/
	music.stop();
	music.unloadSound();
	
	/********************
	********************/
	fft_thread->exit();
	try{
		/********************
		stop済みのthreadをさらにstopすると、Errorが出るようだ。
		********************/
		while(fft_thread->isThreadRunning()){
			fft_thread->waitForThread(true);
		}
		
	}catch(...){
		printf("Thread exiting Error\n");
	}
	
	/********************
	********************/
	printf("\n> Good bye\n");
}


//--------------------------------------------------------------
void ofApp::setup(){
	/********************
	********************/
	ofSetWindowTitle("ANREALAGE:Sound");
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	ofSetWindowShape(WIDTH, HEIGHT);
	ofSetEscapeQuitsApp(false);
	
	ofEnableAlphaBlending();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	// ofEnableSmoothing();
	
	/********************
	********************/
	setup_Gui();
	Guis_LoadSetting();
	
	/********************
	********************/
	soundStream.listDevices();
	if( (soundStream_Input_DeviceId == -1) || (soundStream_Output_DeviceId == -1) ){
		ofExit();
		return;
	}
	// soundStream.setDeviceID(soundStream_DeviceId);
	/* set in & out respectively. */
	soundStream.setInDeviceID(soundStream_Input_DeviceId);  
	soundStream.setOutDeviceID(soundStream_Output_DeviceId);
	
	AudioSample.resize(AUDIO_BUF_SIZE);
	
	/********************
	********************/
	if(!music.isLoaded()){
		music.loadSound("music.mp3");
		if(!music.isLoaded()) { ERROR_MSG(); std::exit(1); }
		
		music.setVolume(1.0);
		music.setLoop(true);
		music.setMultiPlay( true );
		// music.setSpeed( 1.0 );
	}
	
	ReStart();
	
	/********************
	********************/
	RefreshVerts();
	Refresh_BarColor();
	
	/********************
	soundStream.setup()の位置に注意:最後
		audioIn()/audioOut()がstartする.
		これらのmethodは、fft_threadにaccessするので、start前にReStart()によって、fft_threadが初期化されていないと、不正accessが発生してしまう.
		
	out/in chs
		今回は、audioOut()から出力する音はないので、検討時に誤ってLoopハウリングを起こすなどの危険性に対する安全も考慮し、out ch = 0.とした.
	********************/
	// soundStream.setup(this, 2/* out */, 2/* in */, AUDIO_SAMPLERATE, AUDIO_BUF_SIZE, AUDIO_BUFFERS);
	soundStream.setup(this, 0/* out */, 2/* in */, AUDIO_SAMPLERATE, AUDIO_BUF_SIZE, AUDIO_BUFFERS);
}

/******************************
******************************/
void ofApp::RefreshVerts()
{
	float BarWidth = Gui_Global->gui__Graph_w;
	float BarSpace = Gui_Global->gui__Graph_space;
	
	/********************
	********************/
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		Vboset_Monitor.VboVerts[i * 4 + 0].set( BarSpace * i           , 0 );
		Vboset_Monitor.VboVerts[i * 4 + 1].set( BarSpace * i           , DispMonitor[i] );
		Vboset_Monitor.VboVerts[i * 4 + 2].set( BarSpace * i + BarWidth, DispMonitor[i] );
		Vboset_Monitor.VboVerts[i * 4 + 3].set( BarSpace * i + BarWidth, 0 );
	}
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		Vboset_Linein.VboVerts[i * 4 + 0].set( BarSpace * i + BarWidth    , 0 );
		Vboset_Linein.VboVerts[i * 4 + 1].set( BarSpace * i + BarWidth    , DispLinein[i] );
		Vboset_Linein.VboVerts[i * 4 + 2].set( BarSpace * i + BarWidth * 2, DispLinein[i] );
		Vboset_Linein.VboVerts[i * 4 + 3].set( BarSpace * i + BarWidth * 2, 0 );
	}
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		Vboset_Env_L.VboVerts[i * 4 + 0].set( BarSpace * i           , 0 );
		Vboset_Env_L.VboVerts[i * 4 + 1].set( BarSpace * i           , DispEnvironment_L[i] );
		Vboset_Env_L.VboVerts[i * 4 + 2].set( BarSpace * i + BarWidth, DispEnvironment_L[i] );
		Vboset_Env_L.VboVerts[i * 4 + 3].set( BarSpace * i + BarWidth, 0 );
	}
	
	for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
		Vboset_Env_H.VboVerts[i * 4 + 0].set( BarSpace * i           , 0 );
		Vboset_Env_H.VboVerts[i * 4 + 1].set( BarSpace * i           , DispEnvironment_H[i] );
		Vboset_Env_H.VboVerts[i * 4 + 2].set( BarSpace * i + BarWidth, DispEnvironment_H[i] );
		Vboset_Env_H.VboVerts[i * 4 + 3].set( BarSpace * i + BarWidth, 0 );
	}
	
	/********************
	********************/
	for(int i = 0; i < NUM_TIME_POINTS - 1; i++){
		Vboset_Clap_L.VboVerts[i].set(i, Vboset_Clap_L.VboVerts[i+1].y);
		Vboset_Clap_H.VboVerts[i].set(i, Vboset_Clap_H.VboVerts[i+1].y);
		Vboset_Clap_AND.VboVerts[i].set(i, Vboset_Clap_AND.VboVerts[i+1].y);
	}
	if(StateClap_L == STATE_CLAP_ECHO)	Vboset_Clap_L.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/4/3/2 );
	else								Vboset_Clap_L.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, 0 );
	
	if(StateClap_H == STATE_CLAP_ECHO)	Vboset_Clap_H.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/4/3/2 );
	else								Vboset_Clap_H.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, 0 );
	
	if(StateClap_AND == STATE_CLAP_ECHO)	Vboset_Clap_AND.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/4/3/2 );
	else									Vboset_Clap_AND.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, 0 );
}

/******************************
******************************/
void ofApp::Refresh_BarColor()
{
	Vboset_Monitor.set_singleColor(ofColor(255, 0, 0, 200));
	Vboset_Linein.set_singleColor(ofColor(255, 255, 255, 200));
	
	Vboset_Env_L.set_singleColor(ofColor(255, 0, 255, 200));
	Vboset_Env_H.set_singleColor(ofColor(255, 255, 255, 200));
	
	Vboset_Clap_L.set_singleColor(ofColor(255, 0, 255, 200));
	Vboset_Clap_H.set_singleColor(ofColor(255, 255, 255, 200));
	Vboset_Clap_AND.set_singleColor(ofColor(255, 0, 0, 200));
}

/******************************
******************************/
void ofApp::ReStart()
{
	fft_thread->setup();
	
	music.play();
}

/******************************
description
	memoryを確保は、app start後にしないと、
	segmentation faultになってしまった。
******************************/
void ofApp::setup_Gui()
{
	/********************
	********************/
	Gui_Global = new GUI_GLOBAL;
	Gui_Global->setup("ANREALAGE", "gui.xml", 1000, 10);
}

/******************************
******************************/
void ofApp::Guis_LoadSetting()
{
	/********************
	********************/
	printf("\n> Gui Load setting\n");
	
	/********************
	********************/
	string FileName;
	
	/********************
	********************/
	FileName = "gui.xml";
	if(checkif_FileExist(FileName.c_str())) Gui_Global->gui.loadFromFile(FileName.c_str());
	
	printf("\n");
}

/******************************
******************************/
void ofApp::remove_xml()
{
	ofFile::removeFile("gui.xml");
}

/******************************
******************************/
bool ofApp::checkif_FileExist(const char* FileName)
{
	if(ofFile::doesFileExist(FileName)){
		printf("loaded file of %s\n", FileName);
		return true;
		
	}else{
		printf("%s not exist\n", FileName);
		return false;
	}
}

/******************************
******************************/
void ofApp::update(){
	/********************
	********************/
	while(Osc_video.OscReceive.hasWaitingMessages()){
		ofxOscMessage m_receive;
		Osc_video.OscReceive.getNextMessage(&m_receive); // 読み捨て 
	}
	
	/********************
	********************/
	ofSoundUpdate();
	
	/********************
	********************/
	fft_thread->update();
	
	Lev_OfEnvironment_L = fft_thread->get_LevOfEnv_L();
	Lev_OfEnvironment_H = fft_thread->get_LevOfEnv_H();
	
	/********************
	********************/
	StateChart_Clap_LH(StateClap_L, Gui_Global->gui__Clap_LowFreq_Thresh_L, Gui_Global->gui__Clap_LowFreq_Thresh_H, Lev_OfEnvironment_L);
	StateChart_Clap_LH(StateClap_H, Gui_Global->gui__Clap_HighFreq_Thresh_L, Gui_Global->gui__Clap_HighFreq_Thresh_H, Lev_OfEnvironment_H);
	StateChart_Clap_AND();
}

/******************************
******************************/
void ofApp::StateChart_Clap_LH(STATE_CLAP& StateClap, float thresh_L, float thresh_H, float Lev_OfEnvironment){
	switch(StateClap){
		case STATE_CLAP_WAIT:
			if(thresh_H < Lev_OfEnvironment){
				StateClap = STATE_CLAP_ECHO;
			}
			break;
			
		case STATE_CLAP_ECHO:
			if(Lev_OfEnvironment < thresh_L){
				StateClap = STATE_CLAP_WAIT;
			}
			break;
	}
}

/******************************
******************************/
void ofApp::StateChart_Clap_AND(){
	switch(StateClap_AND){
		case STATE_CLAP_WAIT:
			if( (StateClap_L == STATE_CLAP_ECHO) && (StateClap_H == STATE_CLAP_ECHO) ){
				StateClap_AND = STATE_CLAP_ECHO;
				
				/********************
				send osc to video.app
				********************/
#ifndef SJ_RELEASE
				printf("> Detect Clap: send OSC\n");
#endif
	
				ofxOscMessage m;
				m.setAddress("/DetectClap");
				m.addIntArg(0); // dummy.
				Osc_video.OscSend.sendMessage(m);
			}
			break;
			
		case STATE_CLAP_ECHO:
			/********************
			Low Freq側は、落ちてくるのが遅い.
			連続でclapした時に、それぞれOSC command飛ぶよう、STATE_CLAP_WAIT への遷移を調整した.
			********************/
			// if( (StateClap_L == STATE_CLAP_WAIT) && (StateClap_H == STATE_CLAP_WAIT) ){
			if( StateClap_H == STATE_CLAP_WAIT ){
				StateClap_AND = STATE_CLAP_WAIT;
			}
			break;
	}
}

/******************************
******************************/
void ofApp::draw(){
	/********************
	********************/
	fft_thread->get_ParamToDraw(DispMonitor, DispLinein, DispEnvironment_L, DispEnvironment_H, AUDIO_BUF_SIZE/2);
	
	/********************
	********************/
	RefreshVerts();
	Refresh_BarColor();
	
	/********************
	以下は、audioOutからの呼び出しだと segmentation fault となってしまった.
	********************/
	Vboset_Monitor.update();
	Vboset_Linein.update();
	Vboset_Env_L.update();
	Vboset_Env_H.update();
	Vboset_Clap_L.update();
	Vboset_Clap_H.update();
	Vboset_Clap_AND.update();
	
	/********************
	********************/
	ofBackground(0);
	
	/********************
	********************/
	draw_Env_H();
	draw_Env_L();
	
	draw_monitor();
	draw_StateChart_Clap();
	
	/********************
	********************/
	draw_time();
	drawGuis();
}

/******************************
******************************/
void ofApp::draw_Env_H(){
	ofPushStyle();
	ofPushMatrix();
	
		/********************
		********************/
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight() * 2 / 4);
		ofScale(1, -1, 1);
		
		/********************
		y目盛り
		********************/
		const int num_lines = 10;
		const double y_step = ofGetHeight()/4/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(80);
			ofSetLineWidth(1);
			ofLine(0, y, ofGetWidth(), y);
			ofLine(0, -y, ofGetWidth(), -y);

			/********************
			********************/
			char buf[BUF_SIZE];
			// sprintf(buf, "%7.4f", Gui_Global->gui__Disp_FftGainMax_Diff/num_lines * i);
			sprintf(buf, "%e", Gui_Global->gui__Disp_FftGainMax_Diff_HighFreq/num_lines * i);
			
			ofSetColor(80);
			// ofDrawBitmapString(buf, 0, y);
			// if(y != 0) ofDrawBitmapString(buf, 0, -y);
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			font[FONT_S].drawStringAsShapes(buf, ofGetWidth() - Gui_Global->gui__Graph_ofs_x - font[FONT_S].stringWidth(buf) - 10, -y); // y posはマイナス
			if(y != 0) font[FONT_S].drawStringAsShapes(buf, ofGetWidth() - Gui_Global->gui__Graph_ofs_x - font[FONT_S].stringWidth(buf) - 10, y);
			ofScale(1, -1, 1); // 戻す.
		}
		
		/********************
		********************/
		draw_ClapThresh(Gui_Global->gui__Clap_HighFreq_Thresh_L, Gui_Global->gui__Clap_HighFreq_Thresh_H, ofGetWidth() - Gui_Global->gui__Graph_ofs_x, Gui_Global->gui__Disp_FftGainMax_Diff_HighFreq);
		draw_LevOfEnv(	Gui_Global->gui__Clap_HighFreq_FftFreq_From * Gui_Global->gui__Graph_space, Gui_Global->gui__Clap_HighFreq_FftFreq_To * Gui_Global->gui__Graph_space + Gui_Global->gui__Graph_w * 2,
						Lev_OfEnvironment_H, ofGetWidth() - Gui_Global->gui__Graph_ofs_x, Gui_Global->gui__Disp_FftGainMax_Diff_HighFreq);
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_Env_H.draw(GL_QUADS);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_Env_L(){
	ofPushStyle();
	ofPushMatrix();
	
		/********************
		********************/
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight() * 2 / 4);
		ofScale(1, -1, 1);
		
		/********************
		back color.
		********************/
		ofPushMatrix();
		ofSetColor(30, 30, 30, 255);
		ofDrawRectangle(0, -ofGetHeight()/4, Gui_Global->gui__w_Graph_EnvL, ofGetHeight()*2/4);
		ofPopMatrix();
		
		/********************
		y目盛り
		********************/
		const int num_lines = 10;
		const double y_step = ofGetHeight()/4/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(80);
			ofSetLineWidth(1);
			ofLine(0, y, Gui_Global->gui__w_Graph_EnvL, y);
			ofLine(0, -y, Gui_Global->gui__w_Graph_EnvL, -y);

			/********************
			********************/
			char buf[BUF_SIZE];
			// sprintf(buf, "%7.4f", Gui_Global->gui__Disp_FftGainMax_Diff/num_lines * i);
			sprintf(buf, "%e", Gui_Global->gui__Disp_FftGainMax_Diff_LowFreq/num_lines * i);
			
			ofSetColor(80);
			// ofDrawBitmapString(buf, 0, y);
			// if(y != 0) ofDrawBitmapString(buf, 0, -y);
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			font[FONT_S].drawStringAsShapes(buf, Gui_Global->gui__w_Graph_EnvL - font[FONT_S].stringWidth(buf) - 10, -y); // y posはマイナス
			if(y != 0) font[FONT_S].drawStringAsShapes(buf, Gui_Global->gui__w_Graph_EnvL - font[FONT_S].stringWidth(buf) - 10, y);
			ofScale(1, -1, 1); // 戻す.
		}
		
		/********************
		********************/
		draw_ClapThresh(Gui_Global->gui__Clap_LowFreq_Thresh_L, Gui_Global->gui__Clap_LowFreq_Thresh_H, Gui_Global->gui__w_Graph_EnvL, Gui_Global->gui__Disp_FftGainMax_Diff_LowFreq);
		draw_LevOfEnv(	Gui_Global->gui__Clap_LowFreq_FftFreq_From * Gui_Global->gui__Graph_space, Gui_Global->gui__Clap_LowFreq_FftFreq_To * Gui_Global->gui__Graph_space + Gui_Global->gui__Graph_w * 2,
						Lev_OfEnvironment_L, Gui_Global->gui__w_Graph_EnvL, Gui_Global->gui__Disp_FftGainMax_Diff_LowFreq);
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		int NumData_To_Draw = 1;
		while(NumData_To_Draw * (Gui_Global->gui__Graph_space) < Gui_Global->gui__w_Graph_EnvL){
			NumData_To_Draw++;
		}
		Vboset_Env_L.draw(GL_QUADS, (NumData_To_Draw - 1) * 4/* points per data */);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_ClapThresh(float y_threshL, float y_threshH, float Disp_x_max, float map_Disp_y_Max)
{
	int y_threshL_mapped = ofMap(y_threshL, 0, map_Disp_y_Max, 0, ofGetHeight()/4);
	int y_threshH_mapped = ofMap(y_threshH, 0, map_Disp_y_Max, 0, ofGetHeight()/4);
	
	if(y_threshL_mapped < y_threshH_mapped)	ofSetColor(0, 0, 255, 60);
	else									ofSetColor(255, 0, 0, 80);
	
	ofDrawRectangle(0, y_threshL_mapped, Disp_x_max, y_threshH_mapped - y_threshL_mapped);
}

/******************************
******************************/
void ofApp::draw_LevOfEnv(int x_from, int x_to, float Lev_OfEnvironment, float Disp_x_max, float map_Disp_y_max)
{
	if(x_from < x_to)	ofSetColor(0, 255, 0, 200);
	else				ofSetColor(255, 0, 0, 200);
	
	if(Disp_x_max < x_from) x_from = Disp_x_max;
	if(Disp_x_max < x_to) x_to = Disp_x_max;
	
	ofSetLineWidth(1);
	ofLine(x_from, 0, x_from, ofGetHeight()/4);
	ofLine(x_to, 0, x_to, ofGetHeight()/4);
	
	int height = ofMap(Lev_OfEnvironment, 0, map_Disp_y_max, 0, ofGetHeight()/4);
	ofSetColor(0, 255, 0, 80);
	ofDrawRectangle(x_from, 0, x_to - x_from, height);
}

/******************************
******************************/
void ofApp::draw_monitor(){
	/********************
	********************/
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	/********************
	back color.
	********************/
	ofSetColor(30, 30, 30, 255);
	ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight()/4);
	ofPopMatrix();
		
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight()/4);
		ofScale(1, -1, 1);
		
		/********************
		y目盛り
		********************/
		const int num_lines = 10;
		const double y_step = ofGetHeight()/4/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(80);
			ofSetLineWidth(1);
			ofLine(0, y, ofGetWidth(), y);

			/********************
			********************/
			char buf[BUF_SIZE];
			sprintf(buf, "%7.4f", Gui_Global->gui__Disp_FftGainMax_Monitor/num_lines * i);
			ofSetColor(80);
			
			// ofDrawBitmapString(buf, 0, y);
			ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
			font[FONT_S].drawString(buf, ofGetWidth() - Gui_Global->gui__Graph_ofs_x - font[FONT_S].stringWidth(buf) - 10, -y); // y posはマイナス
			ofScale(1, -1, 1); // 戻す.
		}
		
		/********************
		x 目盛り(周波数)
		********************/
		for(int i = 0; i < AUDIO_BUF_SIZE/2; i++){
			if(i % 10 == 0){
				char buf[BUF_SIZE];
				sprintf(buf, "%d", i);
				ofSetColor(80);
				
				// ofDrawBitmapString(buf, i * Gui_Global->gui__Graph_space, -10);
				ofScale(1, -1, 1); // 文字が上下逆さまになってしまうので.
				font[FONT_S].drawStringAsShapes(buf, i * Gui_Global->gui__Graph_space, -(-10)); // y posはマイナス
				ofScale(1, -1, 1); // 戻す.
			}
		}
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_Monitor.draw(GL_QUADS);
		Vboset_Linein.draw(GL_QUADS);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_StateChart_Clap(){
	/********************
	********************/
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
	/********************
	back color.
	********************/
	ofSetColor(10);
	ofDrawRectangle(0, ofGetHeight()*3/4, ofGetWidth(), ofGetHeight()/4);
	ofPopMatrix();
		
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight() - 2);
		ofScale(1, -1, 1);
		
		/********************
		t目盛り
		********************/
		ofSetColor(90);
		ofSetLineWidth(1);
		
		const double x_step = 30; // 1sec
		for(int i = 0; i * x_step < NUM_TIME_POINTS; i++){
			int x = int(i * x_step + 0.5);
			ofLine(x, 0, x, ofGetHeight()/4);
		}
		
		/********************
		********************/
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_Clap_AND.draw(GL_LINE_STRIP);
		
		ofPushMatrix();
			ofTranslate(0, ofGetHeight()/4/3);
			Vboset_Clap_H.draw(GL_LINE_STRIP);
			
			ofTranslate(0, ofGetHeight()/4/3);
			Vboset_Clap_L.draw(GL_LINE_STRIP);
		ofPopMatrix();
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_time()
{
	int t_music = music.getPositionMS();

	/********************
	********************/
	char buf[BUF_SIZE];
	
	int min	= t_music / 1000 / 60;
	int sec	= t_music / 1000 - min * 60;
	int ms	= t_music % 1000;
	
	sprintf(buf, "%6d:%6d:%6d\n%6.2f", min, sec, ms, ofGetFrameRate());
	
	/********************
	********************/
	ofSetColor(255, 255, 255);
	ofDrawBitmapString(buf, 250, 20);
}

/******************************
******************************/
void ofApp::drawGuis()
{
	if(!b_DispGui) return;
	
	Gui_Global->gui.draw();
}

/******************************
audioIn/ audioOut
	同じthreadで動いている様子。
	また、audioInとaudioOutは、同時に呼ばれることはない(多分)。
	つまり、ofAppからaccessがない限り、変数にaccessする際にlock/unlock する必要はない。
	ofApp側からaccessする時は、threadを立てて、安全にpassする仕組みが必要
******************************/
void ofApp::audioIn(float *input, int bufferSize, int nChannels)
{
    for (int i = 0; i < bufferSize; i++) {
        AudioSample.Left[i] = input[2*i];
		AudioSample.Right[i] = input[2*i+1];
    }
	
	/********************
	FFT Filtering
	1 process / block.
	********************/
	if(!b_PauseGraph) fft_thread->update__Gain(AudioSample.Left, AudioSample.Right);
}  

/******************************
******************************/
void ofApp::audioOut(float *output, int bufferSize, int nChannels)
{
	/********************
	x	:input -> output
	o	:No output.
	********************/
    for (int i = 0; i < bufferSize; i++) {
		output[2*i  ] = 0; // L
		output[2*i+1] = 0; // R
    }
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch(key){
		case 'c':
		{
			ofxOscMessage m;
			m.setAddress("/DetectClap");
			m.addIntArg(0); // dummy.
			Osc_video.OscSend.sendMessage(m);
		}
			break;
			
		case 'd':
			b_DispGui = !b_DispGui;
			break;
			
		case 'p':
			b_PauseGraph = !b_PauseGraph;
			if(!b_PauseGraph){ // ReStart.
				fft_thread->setup();
			}
			break;
			
		case ' ':
		{
			char buf[BUF_SIZE];
			
			sprintf(buf, "image_%d.png", png_id);
			ofSaveScreen(buf);
			printf("> %s saved\n", buf);
			
			png_id++;
		}
			break;
			
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}