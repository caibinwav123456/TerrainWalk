#include "StdAfx.h"
#include<process.h>
#include<math.h>
#include<stdio.h>
#include "app.h"
#include "resource.h"
#include "config_val_extern.h"
#define ID_TIMER_LOADING_PROGRESS 50
enum E_PROGRAM_STATE
{
	PROG_STATE_START=0,
	PROG_STATE_LOADING,
	PROG_STATE_READY,
	PROG_STATE_INITED,
};
static int ProgramState=PROG_STATE_START;
static HANDLE hThread=0;
static bool InitResult=false;
static bool InitComplete=false;
static int s_InitProg=0;
int InitProg=0;
int HRes=0,VRes=0;
int HResF=0;
int VResF=0;
int RefreshRate=0;
bool AbortFlg=false;
DEFINE_BOOL_VAL(Windowed,false);
DEFINE_FLOAT_VAL(DTClamp_high,1.1f);
DEFINE_FLOAT_VAL(DTClamp_low,0.9f);
CRITICAL_SECTION csProg;
HDC Mdc=NULL;
HBITMAP MBitmap=NULL;
void Load(HWND hwnd);
void AbortSetup(HWND hwnd);
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
static inline double get_performance_greq()
{
	LARGE_INTEGER bigint;
	QueryPerformanceFrequency(&bigint);
	return (double)bigint.QuadPart;
}
static inline double get_performance_cnt()
{
	LARGE_INTEGER bigint;
	QueryPerformanceCounter(&bigint);
	return (double)bigint.QuadPart;
}
static inline void clamp_tdelta(float* tdelta,float inv_rfrshrt)
{
	if(*tdelta>DTClamp_high*inv_rfrshrt)
		*tdelta=DTClamp_high*inv_rfrshrt;
	if(*tdelta<DTClamp_low*inv_rfrshrt)
		*tdelta=DTClamp_low*inv_rfrshrt;
}
static bool parse_cmdline(char** cfg,char* buf,uint buflen)
{
	LPSTR cmdline=GetCommandLine();
	char *op_tag="--cfgfile=";
	char* cfgstart=strstr(cmdline,op_tag);
	if(cfgstart!=NULL)
	{
		cfgstart+=strlen(op_tag);
		char* cfgend=strchr(cfgstart,' ');
		if(cfgend!=NULL)
		{
			uint len=cfgend-cfgstart;
			if(len>=buflen)
				return false;
			memcpy(buf,cfgstart,len);
			buf[len]=0;
			*cfg=buf;
		}
		else
			*cfg=cfgstart;
	}
	else
		*cfg=NULL;
	return true;
}
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE PreInstance,
				   LPSTR lpCmdLine,
				   int nCmdShow)
{
	const uint buflen=50;
	char buf[buflen],*cfg;
	if(!parse_cmdline(&cfg,buf,buflen))
	{
		MessageBox(0,"invalid command line","error",MB_OK);
		return -1;
	}

	HWND hwnd;
	MSG msg;
	char lpszClassName[]="Terrain";//
	WNDCLASS wc;
	wc.style=0;//CS_HREDRAW|CS_VREDRAW;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON_TW));
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=lpszClassName;

	RegisterClass(&wc);

	HResF=GetSystemMetrics(SM_CXFULLSCREEN);
	VResF=GetSystemMetrics(SM_CYFULLSCREEN);
	HRes=GetSystemMetrics(SM_CXSCREEN);
	VRes=GetSystemMetrics(SM_CYSCREEN);

	hwnd=CreateWindow(lpszClassName,
		              "TerrainWalk",
					  WS_OVERLAPPEDWINDOW,
					  0,0,HResF,VResF,
					  NULL,
					  NULL,
					  hInstance,
					  NULL);

	ShowWindow(hwnd,nCmdShow);
	UpdateWindow(hwnd);

	if(!PreInit(cfg))
	{
		MessageBox(0,"pre-init failed",0,0);
		return -1;
	}
	if(!InitD3D(HRes,VRes,RefreshRate,hwnd,Windowed))
	{
		MessageBox(0,"init failed",0,0);
		return -1;
	}
	if(!Windowed)
		onmdown(hwnd);
	Load(hwnd);
	ZeroMemory(&msg,sizeof(MSG));
	double freq=get_performance_greq();
	double inv_freq=1.0f/freq;
	float inv_rfrshrate=1.0f/RefreshRate;
	while(msg.message!=WM_QUIT)
	{
		if(ProgramState!=PROG_STATE_INITED)
		{
			if(!GetMessage(&msg,0,0,0))
			{
				AbortSetup(hwnd);
				msg.wParam=-1;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if(PeekMessage(&msg,0,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			static float atdelta=0;
			double cTime=get_performance_cnt();
			static double lTime=cTime;
			float tdelta=(cTime-lTime)*inv_freq;
			atdelta=tdelta*0.01f+atdelta*0.99f;
			clamp_tdelta(&atdelta,inv_rfrshrate);
			Display(atdelta);
			FrameMove(atdelta,tdelta);
			Present();
			lTime=cTime;
		}
	}
	Cleanup();
	return msg.wParam;
}

unsigned int WINAPI ThreadProc(void* param)
{
	InitResult=Setup();
	InitComplete=true;
	return 0;
}
void DrawProg(HWND hwnd,HDC hdc,LPRECT rcClt)
{
	RECT& rc=*rcClt;
	RECT rcprog;
	POINT point;
	const int progh=20,progw=256;
	rcprog.left=rc.left+(rc.right-rc.left-progw)/2;
	rcprog.right=rc.left+(rc.right-rc.left-progw)/2+progw*s_InitProg/100;
	rcprog.top=rc.top+(rc.bottom-rc.top-progh)/2;
	rcprog.bottom=rc.top+(rc.bottom-rc.top+progh)/2;
	point.x=rc.left+(rc.right-rc.left+progw)/2+5;
	point.y=rcprog.top;
	LOGBRUSH lgbsprog;
	lgbsprog.lbStyle=BS_SOLID;
	lgbsprog.lbColor=RGB(128,128,255);
	lgbsprog.lbHatch=0;
	LOGPEN lgpenprog;
	lgpenprog.lopnStyle=PS_SOLID;
	lgpenprog.lopnColor=RGB(128,128,255);
	lgpenprog.lopnWidth.x=1;
	lgpenprog.lopnWidth.y=0;
	LOGFONT lgfont;
	ZeroMemory(&lgfont,sizeof(lgfont));
	lgfont.lfHeight=progh;
	lgfont.lfCharSet=ANSI_CHARSET;
	strcpy(lgfont.lfFaceName,"Times New Roman");
	HFONT font=CreateFontIndirect(&lgfont);
	HBRUSH brush=CreateBrushIndirect(&lgbsprog);
	HPEN pen=CreatePenIndirect(&lgpenprog);
	FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
	HGDIOBJ oldbrush=SelectObject(hdc,brush);
	HGDIOBJ oldpen=SelectObject(hdc,pen);
	RoundRect(hdc,rcprog.left,rcprog.top,rcprog.right,rcprog.bottom,10,10);
	HGDIOBJ oldfont=SelectObject(hdc,(HGDIOBJ)font);
	char text[50];
	sprintf(text,"Loading...%d%%",s_InitProg);
	SetTextColor(hdc,RGB(255,255,0));
	SetBkMode(hdc,TRANSPARENT);
	TextOut(hdc,point.x,point.y,text,(int)strlen(text));
	SelectObject(hdc,oldbrush);
	SelectObject(hdc,oldpen);
	SelectObject(hdc,oldfont);
	DeleteObject(brush);
	DeleteObject(pen);
	DeleteObject((HGDIOBJ)font);
}
void DrawProgBuffered(HWND hwnd,HDC hdc)
{
	HGDIOBJ oldbmp=SelectObject(Mdc,(HGDIOBJ)MBitmap);
	RECT rc;
	GetClientRect(hwnd,&rc);
	DrawProg(hwnd,Mdc,&rc);
	BitBlt(hdc,0,0,rc.right,rc.bottom,Mdc,0,0,SRCCOPY);
	SelectObject(Mdc,oldbmp);
}
void Load(HWND hwnd)
{
	InitializeCriticalSection(&csProg);
	HDC hdc=GetDC(hwnd);
	Mdc=CreateCompatibleDC(hdc);
	MBitmap=CreateCompatibleBitmap(hdc,HRes,VRes);
	ProgramState=PROG_STATE_LOADING;
	DrawProgBuffered(hwnd,hdc);
	ReleaseDC(hwnd,hdc);
	SetTimer(hwnd,ID_TIMER_LOADING_PROGRESS,20,NULL);
	hThread=(HANDLE)_beginthreadex(NULL,NULL,ThreadProc,NULL,0,NULL);
	if(hThread==NULL)
	{
		KillTimer(hwnd,ID_TIMER_LOADING_PROGRESS);
		MessageBox(0,"setup thread create failed",0,0);
		PostQuitMessage(-1);
	}
}
void AbortSetup(HWND hwnd)
{
	EnterCriticalSection(&csProg);
	AbortFlg=true;
	LeaveCriticalSection(&csProg);
	WaitForSingleObject(hThread,INFINITE);
	CloseHandle(hThread);
	KillTimer(hwnd,ID_TIMER_LOADING_PROGRESS);
	DeleteCriticalSection(&csProg);
}
LRESULT CALLBACK WndProc(HWND hwnd,
						 UINT message,
						 WPARAM wParam,
						 LPARAM lParam)
{
	int k;
	switch(message)
	{
	case WM_TIMER:
		if(wParam==ID_TIMER_LOADING_PROGRESS)
		{
			if(ProgramState==PROG_STATE_READY)
			{
				KillTimer(hwnd,ID_TIMER_LOADING_PROGRESS);
				WaitForSingleObject(hThread,INFINITE);
				CloseHandle(hThread);
				DeleteCriticalSection(&csProg);
				if(!InitResult)
				{
					MessageBox(0,"setup failed",0,0);
					PostQuitMessage(-1);
					break;
				}
				ProgramState=PROG_STATE_INITED;
				DeleteDC(Mdc);
				DeleteObject(MBitmap);
				Mdc=NULL;
				MBitmap=NULL;
				break;
			}
			EnterCriticalSection(&csProg);
			s_InitProg=InitProg;
			if(InitComplete)
			{
				ProgramState=PROG_STATE_READY;
				s_InitProg=100;
			}
			LeaveCriticalSection(&csProg);
			HDC hdc=GetDC(hwnd);
			DrawProgBuffered(hwnd,hdc);
			ReleaseDC(hwnd,hdc);
		}
		break;
	case WM_PAINT:
		if(ProgramState==PROG_STATE_LOADING
			||ProgramState==PROG_STATE_READY)
		{
			PAINTSTRUCT ps;
			HDC hdc=BeginPaint(hwnd,&ps);
			DrawProgBuffered(hwnd,hdc);
			EndPaint(hwnd,&ps);
		}
		else
			return DefWindowProc(hwnd,message,wParam,lParam);
		break;
	case WM_LBUTTONDOWN:
		if(ProgramState==PROG_STATE_INITED)
			onmdown(hwnd);
		else
			return DefWindowProc(hwnd,message,wParam,lParam);
		break;
	case WM_MOUSEMOVE:
		if(ProgramState==PROG_STATE_INITED)
			onmove(hwnd);
		else
			return DefWindowProc(hwnd,message,wParam,lParam);
		break;
	case WM_KEYDOWN: //if(wParam==VK_ESCAPE)::DestroyWindow(hwnd);
		if(ProgramState!=PROG_STATE_INITED)
			return DefWindowProc(hwnd,message,wParam,lParam);
		switch(wParam)
		{
		case  VK_LEFT:;
		case      'A':k=1;onkey(k,true,eOnKeyTypePressed);break;
		case VK_RIGHT:;
		case      'D':k=2;onkey(k,true,eOnKeyTypePressed);break;
		case    VK_UP:;
		case      'W':k=3;onkey(k,true,eOnKeyTypePressed);break;
		case  VK_DOWN:;
		case      'S':k=4;onkey(k,true,eOnKeyTypePressed);break;
		case      'Q':k=5;onkey(k,true,eOnKeyTypePressed);break;
		case      'E':k=6;onkey(k,true,eOnKeyTypePressed);break;
		case      'Z':k=7;onkey(k,true,eOnKeyTypePressed);break;
		case      'X':k=8;onkey(k,true,eOnKeyTypePressed);break;
		case      'N':k=21;onkey(k,true,eOnKeyTypeHit);break;
		case      'H':k=22;onkey(k,true,eOnKeyTypeHit);break;
		case      'M':k=20;onkey(k,true,eOnKeyTypeHit);break;
		case      'J':k=10;onkey(k,true,eOnKeyTypeHit);break;
		case    VK_F1:k=13;onkey(k,true,eOnKeyTypeHit);break;
		case    VK_F2:k=9;onkey(k,true,eOnKeyTypeHit);break;
		case VK_SPACE:k=11;onkey(k,true,eOnKeyTypeAct);break;
		case VK_RETURN:k=12;onkey(k,true,eOnKeyTypeAct);break;
		default      :k=0;break;  
		}
		break;
	case WM_KEYUP:
		if(wParam==VK_ESCAPE)
		{
			DestroyWindow(hwnd);
			break;
		}
		if(ProgramState!=PROG_STATE_INITED)
			return DefWindowProc(hwnd,message,wParam,lParam);
		switch(wParam)
		{
		case  VK_LEFT:;
		case      'A':k=1;onkey(k,false,eOnKeyTypePressed);break;
		case VK_RIGHT:;
		case      'D':k=2;onkey(k,false,eOnKeyTypePressed);break;
		case    VK_UP:;
		case      'W':k=3;onkey(k,false,eOnKeyTypePressed);break;
		case  VK_DOWN:;
		case      'S':k=4;onkey(k,false,eOnKeyTypePressed);break;
		case      'Q':k=5;onkey(k,false,eOnKeyTypePressed);break;
		case      'E':k=6;onkey(k,false,eOnKeyTypePressed);break;
		case      'Z':k=7;onkey(k,false,eOnKeyTypePressed);break;
		case      'X':k=8;onkey(k,false,eOnKeyTypePressed);break;
		case      'N':k=21;onkey(k,false,eOnKeyTypeHit);break;
		case      'H':k=22;onkey(k,false,eOnKeyTypeHit);break;
		case      'M':k=20;onkey(k,false,eOnKeyTypeHit);break;
		case      'J':k=10;onkey(k,false,eOnKeyTypeHit);break;
		case    VK_F1:k=13;onkey(k,false,eOnKeyTypeHit);break;
		case    VK_F2:k=9;onkey(k,false,eOnKeyTypeHit);break;
		case VK_SPACE:k=11;onkey(k,false,eOnKeyTypeAct);break;
		case VK_RETURN:k=12;onkey(k,false,eOnKeyTypeAct);break;
		default      :k=0;break;  
		}
		break;
	case WM_SIZE:
		if(ProgramState!=PROG_STATE_INITED)
			return DefWindowProc(hwnd,message,wParam,lParam);
		{
			RECT rc;
			GetClientRect(hwnd, &rc);
			OnSize(rc.right-rc.left,rc.bottom-rc.top);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd,message,wParam,lParam);
	}
	return 0;
}
