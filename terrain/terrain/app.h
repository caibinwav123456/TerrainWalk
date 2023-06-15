#ifndef _app_h
#define _app_h
#include "StdAfx.h"
#include "d3dfont.h"
#include "config_val_extern.h"

extern IDirect3DDevice9* device;
extern CRITICAL_SECTION csProg;
extern int InitProg;
extern int HRes;
extern int VRes;
extern int HResF;
extern int VResF;
extern bool AbortFlg;
DECLARE_BOOL_VAL(Windowed);

enum E_ONKEY_TYPE
{
	eOnKeyTypePressed=0,
	eOnKeyTypeHit,
	eOnKeyTypeAct,
};

bool PreInit(char* cfgfile);
bool InitD3D(int w,int h,int& rfrhrate,HWND hwnd,bool windowed);
bool Setup();
void Cleanup();
bool Display(float tdelta);
bool FrameMove(float tdelta,float rtc);
bool Present();
void onmdown(HWND hwnd);
void onmove(HWND hwnd);
void onkey(int key,bool down,E_ONKEY_TYPE type);
void OnSize(int width,int height);
#endif
