/************************************************************
************************************************************/
#include "ofApp.h"
#include <time.h>

/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _BootMode, int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId)
: soundStream_Input_DeviceId(_soundStream_Input_DeviceId)
, soundStream_Output_DeviceId(_soundStream_Output_DeviceId)
, b_DispGui(true)
, fft_thread(THREAD_FFT::getInstance())
, StateClap_L(STATE_CLAP_WAIT_RISE)
, StateClap_H(STATE_CLAP_WAIT_RISE)
, StateClap_AND(STATE_CLAP_WAIT)
, thresh__t_clap_L(0.12)
, thresh__t_clap_H(0.1)
, t_clap_L(thresh__t_clap_L + 10)
, t_clap_H(thresh__t_clap_H + 10)
, t_clap_ChangeState_L(0)
, t_clap_ChangeState_H(0)
, duration_TryClap(0.07)
, png_id(0)
, b_PauseGraph(false)
// , Osc_video("127.0.0.1", 12345, 12346)
, LastInt(0)
, Lev_OfEnvironment_L(0)
, Lev_OfEnvironment_H(0)
, delta__Lev_OfEnvironment_L(0)
, delta__Lev_OfEnvironment_H(0)
, ofs_x_ReadCursor(0)
{
	/********************
	********************/
	BootMode = BOOTMODE(_BootMode);
	
	fp_Log = fopen("../../../data/Log.csv", "w");
	
	/********************
	********************/
	srand((unsigned) time(NULL));
	
	/********************
	********************/
	Osc[OSC_TARGET__VIDEO].setup("127.0.0.1", 12345, 12346);
	Osc[OSC_TARGET__CLAPON].setup("127.0.0.1", 12349, 12350);
	Osc[OSC_TARGET__STROBE].setup("127.0.0.1", 12347, 12348);
	
	/********************
	********************/
	font[FONT_S].load("RictyDiminished-Regular.ttf", 8, true, true, true);
	font[FONT_M].load("RictyDiminished-Regular.ttf", 15, true, true, true);
	font[FONT_L].load("RictyDiminished-Regular.ttf", 25, true, true, true);
	
	
	/********************
	********************/
	Vboset_Monitor.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Linein.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Env_L.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Env_H.setup(AUDIO_BUF_SIZE/2 * 4); // square
	Vboset_Clap_L.setup(NUM_TIME_POINTS);
	Vboset_Clap_H.setup(NUM_TIME_POINTS);
	Vboset_Clap_AND.setup(NUM_TIME_POINTS);
	Vboset_DeltaDiff_L.setup(NUM_TIME_POINTS);
	Vboset_DeltaDiff_H.setup(NUM_TIME_POINTS);
	
	/********************
	fft Gainは、都度 全ての値を算出するが、
	clap(state chart)の時間軸表示は、slideしながら表示なので、
	初期化が必要.
	********************/
	for(int i = 0; i < NUM_TIME_POINTS; i++){
		Vboset_Clap_L.VboVerts[i].set(i, 0);
		Vboset_Clap_H.VboVerts[i].set(i, 0);
		Vboset_Clap_AND.VboVerts[i].set(i, 0);
		Vboset_DeltaDiff_L.VboVerts[i].set(i, 0);
		Vboset_DeltaDiff_H.VboVerts[i].set(i, 0);
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
	fclose(fp_Log);
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
	ofSetFrameRate(60);
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
		// music.loadSound("music.mp3");
		music.loadSound("PARCO.wav");
		
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
	if(!b_PauseGraph){
		for(int i = 0; i < NUM_TIME_POINTS - 1; i++){
			Vboset_Clap_L.VboVerts[i].set(i, Vboset_Clap_L.VboVerts[i+1].y);
			Vboset_Clap_H.VboVerts[i].set(i, Vboset_Clap_H.VboVerts[i+1].y);
			Vboset_Clap_AND.VboVerts[i].set(i, Vboset_Clap_AND.VboVerts[i+1].y);
			
			Vboset_DeltaDiff_L.VboVerts[i].set(i, Vboset_DeltaDiff_L.VboVerts[i+1].y);
			Vboset_DeltaDiff_H.VboVerts[i].set(i, Vboset_DeltaDiff_H.VboVerts[i+1].y);
		}
		if(StateClap_L == STATE_CLAP_WAIT_FALL)		Vboset_Clap_L.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/NUM_SPLIT_DISP/3/2 );
		else if(StateClap_L == STATE_CLAP_TRY_CLAP)	Vboset_Clap_L.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/NUM_SPLIT_DISP/3/2/2 );
		else										Vboset_Clap_L.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, 0 );
		
		if(StateClap_H == STATE_CLAP_WAIT_FALL)		Vboset_Clap_H.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/NUM_SPLIT_DISP/3/2 );
		else if(StateClap_H == STATE_CLAP_TRY_CLAP)	Vboset_Clap_H.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/NUM_SPLIT_DISP/3/2/2 );
		else										Vboset_Clap_H.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, 0 );
		
		if(StateClap_AND == STATE_CLAP_ECHO)	Vboset_Clap_AND.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofGetHeight()/NUM_SPLIT_DISP/3/2 );
		else									Vboset_Clap_AND.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, 0 );
		
		Vboset_DeltaDiff_L.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofMap(delta__Lev_OfEnvironment_L, -Gui_Global->gui__DispMax_DeltaEnv_L, Gui_Global->gui__DispMax_DeltaEnv_L, -ofGetHeight()/NUM_SPLIT_DISP/2, ofGetHeight()/NUM_SPLIT_DISP/2) );
		Vboset_DeltaDiff_H.VboVerts[NUM_TIME_POINTS - 1].set( NUM_TIME_POINTS - 1, ofMap(delta__Lev_OfEnvironment_H, -Gui_Global->gui__DispMax_DeltaEnv_H, Gui_Global->gui__DispMax_DeltaEnv_H, -ofGetHeight()/NUM_SPLIT_DISP/2, ofGetHeight()/NUM_SPLIT_DISP/2) );
	}
}

/******************************
******************************/
void ofApp::ReverseFromVbo_DeltaDiff_L(int id, char* buf)
{
	if( (id < 0) || (Vboset_DeltaDiff_L.VboVerts.size() <= id) ){
		sprintf(buf, "---");
	}else{
		ofVec3f VboVal = Vboset_DeltaDiff_L.VboVerts[id];
		double val = ofMap(VboVal.y, -ofGetHeight()/NUM_SPLIT_DISP/2, ofGetHeight()/NUM_SPLIT_DISP/2, -Gui_Global->gui__DispMax_DeltaEnv_L, Gui_Global->gui__DispMax_DeltaEnv_L);
		sprintf(buf, "%e", val);
	}
}

/******************************
******************************/
void ofApp::ReverseFromVbo_DeltaDiff_H(int id, char* buf)
{
	if( (id < 0) || (Vboset_DeltaDiff_H.VboVerts.size() <= id) ){
		sprintf(buf, "---");
	}else{
		ofVec3f VboVal = Vboset_DeltaDiff_H.VboVerts[id];
		double val = ofMap(VboVal.y, -ofGetHeight()/NUM_SPLIT_DISP/2, ofGetHeight()/NUM_SPLIT_DISP/2, -Gui_Global->gui__DispMax_DeltaEnv_H, Gui_Global->gui__DispMax_DeltaEnv_H);
		sprintf(buf, "%e", val);
	}
}

/******************************
******************************/
void ofApp::ReverseFromVbo_StateClap(int id, char* buf)
{
	if( (id < 0) || (Vboset_Clap_H.VboVerts.size() <= id) ){
		sprintf(buf, "---");
	}else{
		char buf_L[BUF_SIZE];
		char buf_H[BUF_SIZE];
		char buf_AND[BUF_SIZE];
		
		const int ofs = 2;
		
		ofVec3f VboVal = Vboset_Clap_L.VboVerts[id];
		if(ofGetHeight()/NUM_SPLIT_DISP/3/2 - ofs < VboVal.y)			sprintf(buf_L, "WAIT_FALL");
		else if(ofGetHeight()/NUM_SPLIT_DISP/3/2/2 - ofs < VboVal.y)	sprintf(buf_L, "TRY_CLAP");
		else															sprintf(buf_L, "WAIT_RISE");
		
		VboVal = Vboset_Clap_H.VboVerts[id];
		if(ofGetHeight()/NUM_SPLIT_DISP/3/2 - ofs < VboVal.y)			sprintf(buf_H, "WAIT_FALL");
		else if(ofGetHeight()/NUM_SPLIT_DISP/3/2/2 - ofs < VboVal.y)	sprintf(buf_H, "WAIT_CLAP");
		else															sprintf(buf_H, "WAIT_RISE");
		
		
		VboVal = Vboset_Clap_AND.VboVerts[id];
		if(ofGetHeight()/NUM_SPLIT_DISP/3/2 - ofs < VboVal.y)			sprintf(buf_AND, "ECHO");
		else															sprintf(buf_AND, "WAIT");
		
		sprintf(buf, "L:%-15s  H:%-15s  A:%-15s", buf_L, buf_H, buf_AND);
	}
}

/******************************
******************************/
void ofApp::Refresh_BarColor()
{
	/********************
	********************/
	Vboset_Monitor.set_singleColor(ofColor(255, 0, 0, 200));
	Vboset_Linein.set_singleColor(ofColor(255, 255, 255, 200));
	
	/********************
	********************/
	Vboset_Env_L.set_singleColor(ofColor(255, 0, 255, 200));
	Vboset_Env_H.set_singleColor(ofColor(255, 255, 255, 200));
	
	/********************
	********************/
	Vboset_Clap_L.set_singleColor(ofColor(255, 0, 255, 200));
	Vboset_Clap_H.set_singleColor(ofColor(255, 255, 255, 200));
	Vboset_Clap_AND.set_singleColor(ofColor(255, 0, 0, 200));
	
	/********************
	********************/
	Vboset_DeltaDiff_L.set_singleColor(ofColor(255, 0, 255, 200));
	Vboset_DeltaDiff_H.set_singleColor(ofColor(255, 255, 255, 200));
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
	now = ofGetElapsedTimef();
	
	if(BootMode == BOOTMODE__DEBUG){
		if(now < 30.0) fprintf(fp_Log, "%f,", now);
	}
	
	/********************
	********************/
	for(int i = 0; i < NUM_OSC_TARGET; i++){
		while(Osc[i].OscReceive.hasWaitingMessages()){
			ofxOscMessage m_receive;
			Osc[i].OscReceive.getNextMessage(&m_receive); // 読み捨て 
		}
	}
	
	/********************
	********************/
	ofSoundUpdate();
	
	/********************
	********************/
	fft_thread->update();
	
	double Last_Lev_OfEnvironment_L = Lev_OfEnvironment_L;
	double Last_Lev_OfEnvironment_H = Lev_OfEnvironment_H;
	
	Lev_OfEnvironment_L = fft_thread->get_LevOfEnv_L();
	Lev_OfEnvironment_H = fft_thread->get_LevOfEnv_H();
	
	if((LastInt < now) && (now - LastInt < 0.1)){
		delta__Lev_OfEnvironment_L = (Lev_OfEnvironment_L - Last_Lev_OfEnvironment_L) / (now - LastInt);
		delta__Lev_OfEnvironment_H = (Lev_OfEnvironment_H - Last_Lev_OfEnvironment_H) / (now - LastInt);
	}
	
	/********************
	********************/
	StateChart_Clap_LH(StateClap_L, Gui_Global->gui__DeltaClap_LowFreq_Thresh_H, delta__Lev_OfEnvironment_L, t_clap_ChangeState_L, t_clap_L, thresh__t_clap_L);
	StateChart_Clap_LH(StateClap_H, Gui_Global->gui__DeltaClap_HighFreq_Thresh_H, delta__Lev_OfEnvironment_H, t_clap_ChangeState_H, t_clap_H, thresh__t_clap_H);
	StateChart_Clap_AND();
	
	/********************
	********************/
	LastInt = now;
}

/******************************
******************************/
void ofApp::StateChart_Clap_LH(STATE_CLAP& StateClap, float thresh_H, float Lev_OfEnvironment, float& t_clap_ChangeState, float& t_clap, float thresh__t_clap){
	switch(StateClap){
		case STATE_CLAP_WAIT_RISE:
			if(thresh_H < Lev_OfEnvironment){
				StateClap = STATE_CLAP_WAIT_FALL;
				t_clap_ChangeState = ofGetElapsedTimef();
			}
			break;
			
		case STATE_CLAP_WAIT_FALL:
			if(Lev_OfEnvironment < 0){
				t_clap = ofGetElapsedTimef() - t_clap_ChangeState;
				t_clap_ChangeState = ofGetElapsedTimef();
				
				if(BootMode == BOOTMODE__DEBUG){
					if(thresh_H == Gui_Global->gui__DeltaClap_LowFreq_Thresh_H)			printf("> clap time: %f (LowFreq)\n", t_clap);
					else if(thresh_H == Gui_Global->gui__DeltaClap_HighFreq_Thresh_H)	printf("> clap time: %f (HighFreq)\n", t_clap);
					else																printf("> clap time: %f (---)\n", t_clap);
				}
				
				/********************
				********************/
				if(t_clap < thresh__t_clap)	StateClap = STATE_CLAP_TRY_CLAP;
				else						StateClap = STATE_CLAP_WAIT_RISE; // no chance.
			}
			break;
			
		case STATE_CLAP_TRY_CLAP:
			if(duration_TryClap < ofGetElapsedTimef() - t_clap_ChangeState){
				StateClap = STATE_CLAP_WAIT_RISE;
				t_clap = thresh__t_clap + 10;
			}
			break;
	}
}

/******************************
******************************/
void ofApp::StateChart_Clap_AND(){
	switch(StateClap_AND){
		case STATE_CLAP_WAIT:
			if( (t_clap_L < thresh__t_clap_L) && (t_clap_H < thresh__t_clap_H) ){
				StateClap_AND = STATE_CLAP_ECHO;
				
				/********************
				send osc to video.app
				********************/
				if(BootMode == BOOTMODE__DEBUG){
					printf("> Detect Clap: send OSC\n");
				}
	
				ofxOscMessage m;
				m.setAddress("/DetectClap");
				m.addIntArg(0); // dummy.
				for(int i = 0; i < NUM_OSC_TARGET; i++){
					Osc[i].OscSend.sendMessage(m);
				}
			}
			break;
			
		case STATE_CLAP_ECHO:
			StateClap_AND = STATE_CLAP_WAIT;
			
			t_clap_L = thresh__t_clap_L + 10;
			t_clap_H = thresh__t_clap_H + 10;
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
	Vboset_DeltaDiff_L.update();
	Vboset_DeltaDiff_H.update();
	
	/********************
	********************/
	ofBackground(0);
	
	/********************
	********************/
	draw_Env_H();
	draw_Env_L();
	
	draw_monitor();
	draw_StateChart_Clap();
	
	draw_DeltaEnv_L();
	draw_DeltaEnv_H();
	
	if(b_PauseGraph) draw_CursorAndValue();
	
	/********************
	********************/
	draw_time();
	drawGuis();
	
	/********************
	checked time cost:Top of update - End of draw.
	Result < 3ms
	********************/
	if(BootMode == BOOTMODE__DEBUG){
		if(now < 30.0) fprintf(fp_Log, "%f\n", ofGetElapsedTimef());
	}
}

/******************************
******************************/
void ofApp::draw_CursorAndValue(){
	/********************
	********************/
	int Cursor_x = mouseX + ofs_x_ReadCursor;
	int Vbo_id = Cursor_x - Gui_Global->gui__Graph_ofs_x;

	/********************
	********************/
	if( (Vbo_id < 0) || (NUM_TIME_POINTS <= Vbo_id) ) return;
	
	/********************
	********************/
	ofSetColor(255, 0, 0, 255);
	ofSetLineWidth(1);
	ofLine(Cursor_x, 0, Cursor_x, ofGetHeight());
	
	/********************
	********************/
	ofSetColor(100);
	int ofs_x = 10;
	int ofs_y = 10;
	char buf[BUF_SIZE];
	
	
	ReverseFromVbo_DeltaDiff_L(Vbo_id, buf);
	font[FONT_S].drawString(buf, Cursor_x + ofs_x, ofGetHeight()*4/6 - ofs_y);
	
	ReverseFromVbo_DeltaDiff_H(Vbo_id, buf);
	font[FONT_S].drawString(buf, Cursor_x + ofs_x, ofGetHeight()*5/6 - ofs_y);
	
	ReverseFromVbo_StateClap(Vbo_id, buf);
	font[FONT_S].drawString(buf, Cursor_x + ofs_x, ofGetHeight()*6/6 - ofs_y);
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
		
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight() * 2 / NUM_SPLIT_DISP);
		ofScale(1, -1, 1);
		
		/********************
		y目盛り
		********************/
		const int num_lines = 10;
		const double y_step = ofGetHeight()/NUM_SPLIT_DISP/num_lines;
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
			sprintf(buf, "%e", Gui_Global->gui__DispMax_GainEnv_HighFreq/num_lines * i);
			
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
		// draw_ClapThresh(Gui_Global->gui__Clap_HighFreq_Thresh_L, Gui_Global->gui__Clap_HighFreq_Thresh_H, ofGetWidth() - Gui_Global->gui__Graph_ofs_x, Gui_Global->gui__DispMax_GainEnv_HighFreq);
		draw_LevOfEnv(	Gui_Global->gui__Clap_HighFreq_FftFreq_From * Gui_Global->gui__Graph_space, Gui_Global->gui__Clap_HighFreq_FftFreq_To * Gui_Global->gui__Graph_space + Gui_Global->gui__Graph_w * 2,
						Lev_OfEnvironment_H, ofGetWidth() - Gui_Global->gui__Graph_ofs_x, Gui_Global->gui__DispMax_GainEnv_HighFreq);
						
		draw_FreqMask(ofGetWidth() - Gui_Global->gui__Graph_ofs_x);
		
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
		
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight() * 2 / NUM_SPLIT_DISP);
		ofScale(1, -1, 1);
		
		/********************
		back color.
		********************/
		ofPushMatrix();
		ofSetColor(20, 20, 20, 255);
		ofDrawRectangle(0, -ofGetHeight()/NUM_SPLIT_DISP, Gui_Global->gui__w_Graph_EnvL, ofGetHeight()*2/NUM_SPLIT_DISP);
		ofPopMatrix();
		
		/********************
		y目盛り
		********************/
		const int num_lines = 10;
		const double y_step = ofGetHeight()/NUM_SPLIT_DISP/num_lines;
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
			sprintf(buf, "%e", Gui_Global->gui__DispMax_GainEnv_LowFreq/num_lines * i);
			
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
		// draw_ClapThresh(Gui_Global->gui__Clap_LowFreq_Thresh_L, Gui_Global->gui__Clap_LowFreq_Thresh_H, Gui_Global->gui__w_Graph_EnvL, Gui_Global->gui__DispMax_GainEnv_LowFreq);
		draw_LevOfEnv(	Gui_Global->gui__Clap_LowFreq_FftFreq_From * Gui_Global->gui__Graph_space, Gui_Global->gui__Clap_LowFreq_FftFreq_To * Gui_Global->gui__Graph_space + Gui_Global->gui__Graph_w * 2,
						Lev_OfEnvironment_L, Gui_Global->gui__w_Graph_EnvL, Gui_Global->gui__DispMax_GainEnv_LowFreq);
		
		draw_FreqMask(Gui_Global->gui__w_Graph_EnvL);
		
		/********************
		********************/
		ofSetColor(255);
		glPointSize(1.0);
		glLineWidth(1);
		
		int NumData_To_Draw = 0;
		while(NumData_To_Draw * (Gui_Global->gui__Graph_space) < Gui_Global->gui__w_Graph_EnvL){
			NumData_To_Draw++;
		}
		Vboset_Env_L.draw(GL_QUADS, NumData_To_Draw * 4/* points per data */);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_ClapThresh(float y_threshL, float y_threshH, float Disp_x_max, float map_Disp_y_Max)
{
	float y_threshL_mapped = ofMap(y_threshL, 0, map_Disp_y_Max, 0, ofGetHeight()/NUM_SPLIT_DISP);
	float y_threshH_mapped = ofMap(y_threshH, 0, map_Disp_y_Max, 0, ofGetHeight()/NUM_SPLIT_DISP);
	
	if(y_threshL_mapped < y_threshH_mapped)	ofSetColor(0, 0, 255, 60);
	else									ofSetColor(255, 0, 0, 80);
	
	ofDrawRectangle(0, y_threshL_mapped, Disp_x_max, y_threshH_mapped - y_threshL_mapped);
}

/******************************
******************************/
void ofApp::draw_DeltaClap_Thresh(float y_threshH, float Disp_x_max, float map_Disp_y_Max)
{
	float y_threshL_mapped = 0;//ofMap(y_threshL, 0, map_Disp_y_Max, 0, ofGetHeight()/NUM_SPLIT_DISP/2);
	float y_threshH_mapped = ofMap(y_threshH, 0, map_Disp_y_Max, 0, ofGetHeight()/NUM_SPLIT_DISP/2);
	
	ofSetColor(0, 0, 255, 70);
	
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
	ofLine(x_from, 0, x_from, ofGetHeight()/NUM_SPLIT_DISP);
	ofLine(x_to, 0, x_to, ofGetHeight()/NUM_SPLIT_DISP);
	
	int height = ofMap(Lev_OfEnvironment, 0, map_Disp_y_max, 0, ofGetHeight()/NUM_SPLIT_DISP);
	ofSetColor(0, 255, 0, 80);
	ofDrawRectangle(x_from, 0, x_to - x_from, height);
}

/******************************
******************************/
void ofApp::draw_FreqMask(float Disp_x_max)
{
	ofFill();
	ofSetColor(255, 255, 255, 30);
	
	for(int i = 0; i < NUM__FREQ_MASKS_TO_DETECT_CLAP; i++){
		if(Gui_Global->gui__ClapMask_FreqWidth[i] != 0){
			int x_from = Gui_Global->gui__ClapMask_FreqFrom[i] * Gui_Global->gui__Graph_space;
			int x_to = (Gui_Global->gui__ClapMask_FreqFrom[i] + Gui_Global->gui__ClapMask_FreqWidth[i] - 1) * Gui_Global->gui__Graph_space + Gui_Global->gui__Graph_w * 2;
			
			if(Disp_x_max < x_from) x_from = Disp_x_max;
			if(Disp_x_max < x_to) x_to = Disp_x_max;
			
			ofDrawRectangle(x_from, -ofGetHeight()/NUM_SPLIT_DISP, x_to - x_from, ofGetHeight() * 2/NUM_SPLIT_DISP);
		}
	}
}

/******************************
******************************/
void ofApp::draw_FocusedGain_Area(float Disp_x_max)
{
	ofFill();
	ofSetColor(255, 255, 255, 30);
	
	for(int i = 0; i < NUM__FOCUSED_GAIN_TO_DETECT_CLAP; i++){
		if(Gui_Global->gui__FocusedGain_FreqWidth[i] != 0){
			int x_from = Gui_Global->gui__FocusedGain_FreqFrom[i] * Gui_Global->gui__Graph_space;
			int x_to = (Gui_Global->gui__FocusedGain_FreqFrom[i] + Gui_Global->gui__FocusedGain_FreqWidth[i] - 1) * Gui_Global->gui__Graph_space + Gui_Global->gui__Graph_w * 2;
			
			if(Disp_x_max < x_from) x_from = Disp_x_max;
			if(Disp_x_max < x_to) x_to = Disp_x_max;
			
			ofDrawRectangle(x_from, 0, x_to - x_from, ofGetHeight()/NUM_SPLIT_DISP);
		}
	}
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
	ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight()/NUM_SPLIT_DISP);
	ofPopMatrix();
		
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight()/NUM_SPLIT_DISP);
		ofScale(1, -1, 1);
		
		/********************
		y目盛り
		********************/
		const int num_lines = 10;
		const double y_step = ofGetHeight()/NUM_SPLIT_DISP/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(80);
			ofSetLineWidth(1);
			ofLine(0, y, ofGetWidth(), y);

			/********************
			********************/
			char buf[BUF_SIZE];
			sprintf(buf, "%7.4f", Gui_Global->gui__DispMax_GainMonitor/num_lines * i);
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
		draw_FocusedGain_Area(ofGetWidth() - Gui_Global->gui__Graph_ofs_x);
		
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
	ofDrawRectangle(0, ofGetHeight()*5/NUM_SPLIT_DISP, ofGetWidth(), ofGetHeight()/NUM_SPLIT_DISP);
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
			ofLine(x, 0, x, ofGetHeight()/NUM_SPLIT_DISP);
		}
		
		/********************
		********************/
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_Clap_AND.draw(GL_LINE_STRIP);
		
		ofPushMatrix();
			ofTranslate(0, ofGetHeight()/NUM_SPLIT_DISP/3);
			Vboset_Clap_H.draw(GL_LINE_STRIP);
			
			ofTranslate(0, ofGetHeight()/NUM_SPLIT_DISP/3);
			Vboset_Clap_L.draw(GL_LINE_STRIP);
		ofPopMatrix();
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_DeltaEnv_L(){
	/********************
	********************/
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
	/********************
	back color.
	********************/
	ofSetColor(30, 30, 30, 255);
	ofDrawRectangle(0, ofGetHeight()*3/NUM_SPLIT_DISP, ofGetWidth(), ofGetHeight()/NUM_SPLIT_DISP);
	ofPopMatrix();
		
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight()*7/12); // NUM_SPLIT_DISP
		ofScale(1, -1, 1);
		
		/********************
		t目盛り
		********************/
		ofSetColor(90);
		ofSetLineWidth(1);
		
		const double x_step = 30; // 1sec
		for(int i = 0; i * x_step < NUM_TIME_POINTS; i++){
			int x = int(i * x_step + 0.5);
			ofLine(x, 0, x, ofGetHeight()/NUM_SPLIT_DISP/2);
		}
		
		/********************
		y目盛り
		********************/
		const int num_lines = 5;
		const double y_step = ofGetHeight()/NUM_SPLIT_DISP/2/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(80);
			ofSetLineWidth(1);
			ofLine(0, y, ofGetWidth(), y);
			ofLine(0, -y, ofGetWidth(), -y);

			/********************
			********************/
			char buf[BUF_SIZE];
			sprintf(buf, "%e", Gui_Global->gui__DispMax_DeltaEnv_L/num_lines * i);
			
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
		draw_DeltaClap_Thresh(Gui_Global->gui__DeltaClap_LowFreq_Thresh_H, ofGetWidth() - Gui_Global->gui__Graph_ofs_x, Gui_Global->gui__DispMax_DeltaEnv_L);
		
		/********************
		********************/
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_DeltaDiff_L.draw(GL_LINE_STRIP);
		
	ofPopMatrix();
	ofPopStyle();
}

/******************************
******************************/
void ofApp::draw_DeltaEnv_H(){
	/********************
	********************/
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
	/********************
	back color.
	********************/
	ofSetColor(0);
	ofDrawRectangle(0, ofGetHeight()*4/NUM_SPLIT_DISP, ofGetWidth(), ofGetHeight()/NUM_SPLIT_DISP);
	ofPopMatrix();
		
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		/********************
		********************/
		ofTranslate(Gui_Global->gui__Graph_ofs_x, ofGetHeight()*9/12); // NUM_SPLIT_DISP
		ofScale(1, -1, 1);
		
		/********************
		t目盛り
		********************/
		ofSetColor(90);
		ofSetLineWidth(1);
		
		const double x_step = 30; // 1sec
		for(int i = 0; i * x_step < NUM_TIME_POINTS; i++){
			int x = int(i * x_step + 0.5);
			ofLine(x, 0, x, ofGetHeight()/NUM_SPLIT_DISP/2);
		}
		
		/********************
		y目盛り
		********************/
		const int num_lines = 5;
		const double y_step = ofGetHeight()/NUM_SPLIT_DISP/2/num_lines;
		for(int i = 0; i < num_lines; i++){
			int y = int(i * y_step + 0.5);
			
			ofSetColor(80);
			ofSetLineWidth(1);
			ofLine(0, y, ofGetWidth(), y);
			ofLine(0, -y, ofGetWidth(), -y);

			/********************
			********************/
			char buf[BUF_SIZE];
			sprintf(buf, "%e", Gui_Global->gui__DispMax_DeltaEnv_H/num_lines * i);
			
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
		draw_DeltaClap_Thresh(Gui_Global->gui__DeltaClap_HighFreq_Thresh_H, ofGetWidth() - Gui_Global->gui__Graph_ofs_x, Gui_Global->gui__DispMax_DeltaEnv_H);
		
		/********************
		********************/
		glPointSize(1.0);
		glLineWidth(1);
		
		Vboset_DeltaDiff_H.draw(GL_LINE_STRIP);
		
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
	fft_thread->update__Gain(AudioSample.Left, AudioSample.Right);
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
			for(int i = 0; i < NUM_OSC_TARGET; i++){
				Osc[i].OscSend.sendMessage(m);
			}
		}
			break;
			
		case 'd':
			b_DispGui = !b_DispGui;
			break;
			
		case 'j':
			music.setPositionMS(music.getPositionMS() + 5 * 60e3);
			break;
			
		case 'k':
		{
			int h = 0;
			int m = 0;
			int s = 0;
			
			int ms = ((h * 60 + m) * 60 + s) * 1000;
			music.setPositionMS(ms);
		}
			break;
			
		case 'l':
			music.setPositionMS(music.getPositionMS() + 30 * 60e3);
			break;
			
		case 'p':
			b_PauseGraph = !b_PauseGraph;
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
			
		case OF_KEY_RIGHT:
			ofs_x_ReadCursor++;
			break;
			
		case OF_KEY_LEFT:
			ofs_x_ReadCursor--;
			break;
			
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	ofs_x_ReadCursor = 0;
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
