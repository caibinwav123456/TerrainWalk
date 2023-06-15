#include "app.h"
#include <math.h>
#include <stdio.h>
#include "config.h"
#include "config_val.h"
#include "config_val_extern.h"
#include "drawtext.h"
//#define REFINED_MODEL
/*#ifdef REFINED_MODEL
#define GRID_PITCH 3.0
#define CORE_SPAN 12.0
#define GRID_SPAN 3.0
#define PHYS_SCALE 0.025
#else
#define GRID_PITCH 7.0
#define CORE_SPAN 12.0
#define GRID_SPAN 3.0
#define PHYS_SCALE 0.1
#endif*/
DEFINE_FLOAT_VAL(GRID_PITCH,7.0);
DEFINE_FLOAT_VAL(CORE_SPAN,12.0);
DEFINE_FLOAT_VAL(GRID_SPAN,3.0);
DEFINE_FLOAT_VAL(PHYS_SCALE,0.1);
DEFINE_BOOL_VAL(FlipX,false);
#define MOUSE_DRAG 0.025
#define PROG_START_VB 10
#define PROG_VB_1 15
#define PROG_END_VB 95
#define safe_release(ptr) \
	if((ptr)!=NULL) \
	{ \
		(ptr)->Release(); \
		(ptr)=NULL; \
	}
int DIMW=100,DIMH=100;
//#define DIM 256
//#define HDIM 128
//#define DIM1 257
//////////////////////physical constant
DEFINE_FLOAT_VAL(MAX_SPEED,20);//1
DEFINE_FLOAT_VAL(MIN_SPEED,0);//5
DEFINE_FLOAT_VAL(ENGINE_ACC,5);//4;//0.2
DEFINE_FLOAT_VAL(GRAVITY_ACC,4);//3;//
DEFINE_FLOAT_VAL(FRICTION_ACC,3);//2;//0.1
DEFINE_FLOAT_VAL(JUMP,4);//1.5;//20
DEFINE_FLOAT_VAL(JUMP_INTERVAL,0.2);
DEFINE_FLOAT_VAL(JUMP_POWERUP_TIME,0.3);
DEFINE_FLOAT_VAL(JUMP_FADE_TIME,0.3);
DEFINE_FLOAT_VAL(ONLAND_FADE_TIME,0.3);
//////////////////////
int W=0,H=0;
float tcon=0,fcon=0;
char txt[50]="Frame rate:0fps",txt1[20]="LightTrack:ON",
	txt2[50]="",txt3[20]="PhysMode:OFF",txt4[20]="Jump:0%%";
float wscale=1.0;
//////////////////current grid coordinates
POINT CurUV;
//////////////////for detection
bool onland=false,lately_onland=false;
//////////////////for jump state
bool jump_swt=false,jump_pre=false;
float jump_delay=0,jump_force=0,jump_fade=0,onland_fade=0;
float accumulate=0.;
//////////////////device objects
IDirect3DDevice9* device=0;
IDirect3DVertexBuffer9* vb=0;
IDirect3DIndexBuffer9* ib=0;
IDirect3DTexture9* terrainmap=0;
D3DXMATRIX view,proj,Rx,Ry,trans,world;
D3DXVECTOR3 xt(1,0,0),yt(0,1,0),zt(0,0,1);
D3DVIEWPORT9 vp={0,0,800,600,0,1};//1024/8,800/8
D3DXVECTOR3 Pos(0.0f,1.0f,-2.0f);
D3DXVECTOR3 Velocity(0.0f,0.0f,0.0f);
D3DXVECTOR3 CurUVNorm(0.0f,1.0f,0.0f);
D3DLIGHT9 light;
CD3DFont* font=NULL;
TextRender* TxtRdr=NULL;
DEFINE_STRING_VAL(map_path,"");
DEFINE_STRING_VAL(help_file,"help.txt");
DEFINE_FLOAT_VAL(MAP_HEIGHT_SCALE,0.3f);
int keystate=(1<<20);
float hangle=0,vangle=0;//X=0,Y=1,Z=-2,
D3DXVECTOR2 anglea(0,0);
////////////////////////////////////////
float *h_map=NULL;
int walk=0;
////////////////////////////////////////
struct vertex
{
	float _x,_y,_z;
	float _nx,_ny,_nz;
	float _u,_v;
	static const DWORD FVF;
	vertex(){}
	vertex(float x,float y,float z,float nx,float ny,float nz,float u,float v)
	{_x=x;_y=y;_z=z;_nx=nx;_ny=ny;_nz=nz;_u=u;_v=v;}
};
const DWORD vertex::FVF=D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1;
bool mode=false;
#define Progress(tag1,tag2,prog) \
	tag1:if(SetProgress(prog)) \
	{ \
		ret=false; \
		goto tag2; \
	}
bool SetProgress(int prog)
{
	if(prog<0)
		prog=0;
	else if(prog>100)
		prog=100;
	bool out;
	EnterCriticalSection(&csProg);
	out=AbortFlg;
	if(!out)
		InitProg=prog;
	LeaveCriticalSection(&csProg);
	return out;
}
bool InitParams()
{
	D3DXIMAGE_INFO texinfo;
	if(FAILED(D3DXGetImageInfoFromFile(map_path.c_str(),&texinfo)))
		return false;
	float _W=(float)texinfo.Width/GRID_PITCH,
	_H=(float)texinfo.Height/GRID_PITCH;
	float mean=max(_W,_H);
	float ratio=(min(mean,16384.))/mean;
	_W/=ratio,_H/=ratio;
	DIMW=(int)floorf(_W),DIMH=(int)floorf(_H);
	if(DIMW&0x1)DIMW++;
	if(DIMH&0x1)DIMH++;
	wscale=((float)texinfo.Width/texinfo.Height)*(FlipX?-1:1);

	//adjust physical constants
	float physscale=(float)DIMH*PHYS_SCALE;// /10./3;
	MAX_SPEED/=physscale;
	MIN_SPEED/=physscale;
	ENGINE_ACC/=physscale;
	GRAVITY_ACC/=physscale;
	FRICTION_ACC/=physscale;
	JUMP/=physscale;
	vp.Width=HRes,vp.Height=VRes;

	return true;
}
bool PreInit(char* cfgfile)
{
	ConfigProfile profile;
	if(profile.LoadConfigFile(cfgfile==NULL?CFG_FILE_PATH:cfgfile)!=0)
		return false;
	config_val_container::get_val_container()->config_value(&profile);
	return InitParams();
}
bool PrepVbIb(int HDIMW,int HDIMH,int DIMW1,int DIMH1)
{
	bool ret=true;
	HRESULT hr;
	hr=device->CreateVertexBuffer(
		DIMW1*DIMH1*sizeof(vertex),
		D3DUSAGE_WRITEONLY,
		vertex::FVF,
		D3DPOOL_MANAGED,
		&vb,
		0);
	if(FAILED(hr))
	{
		MessageBox(0,"vb failed",0,0);
		return false;
	}
	hr=device->CreateIndexBuffer(
		DIMW*DIMH*6*sizeof(DWORD),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX32,
		D3DPOOL_MANAGED,
		&ib,
		0);
	if(FAILED(hr))
	{
		MessageBox(0,"ib failed",0,0);
		return false;
	}

	D3DSURFACE_DESC desc;
	terrainmap->GetLevelDesc(0,&desc);
	if(desc.Format!=D3DFMT_X8R8G8B8)
	{
		MessageBox(0,"fmt incompatible",0,0);
		return false;
	}
	D3DLOCKED_RECT rect;
	terrainmap->LockRect(0,&rect,0,0);
	DWORD* imgdata=(DWORD*)rect.pBits;

	vertex* vertices;
	vb->Lock(0,0,(void**)&vertices,0);

	float corew=(float)desc.Width*CORE_SPAN/GRID_PITCH/DIMW,coreh=(float)desc.Height*CORE_SPAN/GRID_PITCH/DIMH;
	int gridw=corew*GRID_SPAN,gridh=coreh*GRID_SPAN;
	double *wsample=new double[gridw+1],*hsample=new double[gridh+1];
	{
		double wsum=0,hsum=0;
		for(int i=0;i<=gridw;i++)
		{
			wsample[i]=exp(-pow((double)i/corew,2));
			if(i==0)
				wsum+=wsample[i];
			else
				wsum+=2*wsample[i];
		}
		for(int i=0;i<=gridh;i++)
		{
			hsample[i]=exp(-pow((double)i/coreh,2));
			if(i==0)
				hsum+=hsample[i];
			else
				hsum+=2*hsample[i];
		}
		for(int i=0;i<=gridw;i++)
			wsample[i]/=wsum;
		for(int i=0;i<=gridh;i++)
			hsample[i]/=hsum;
	}
	Progress(end0,end,PROG_VB_1);
	
	for(int i=0;i<=DIMH;i++)
	for(int j=0;j<=DIMW;j++)
	{
		Progress(end1,end,(PROG_END_VB-PROG_VB_1)*(i*DIMW1+j)/(DIMW1*DIMH1)+PROG_VB_1);
		int row=(i*desc.Height/DIMH);
		int col=(j*desc.Width/DIMW);
		double c=0;
		for(int k=row-gridh;k<=row+gridh;k++)
		for(int l=col-gridw;l<=col+gridw;l++)
		{
			int nk=(k<0?0:(k>=(int)desc.Height?(int)desc.Height-1:k));
			int nl=(l<0?0:(l>=(int)desc.Width?(int)desc.Width-1:l));
			int index=rect.Pitch/4*nk+nl;
			float hm=(float)(((imgdata[index]>>16)&255)+((imgdata[index]>>8)&255)+((imgdata[index])&255))/3;
			c+=hm*(wsample[abs(l-col)]*hsample[abs(k-row)]);
		}
		float a=((float)(j-HDIMW))/HDIMW,b=((float)(i-HDIMH))/HDIMH;
		vertices[i*DIMW1+j]._x=a;
		vertices[i*DIMW1+j]._z=b;
		vertices[i*DIMW1+j]._y=h_map[i*DIMW1+j]=c/256*MAP_HEIGHT_SCALE;
		vertices[i*DIMW1+j]._ny=2.0f/DIMH;//0.05f;
		vertices[i*DIMW1+j]._u=(a+1)/2;
		vertices[i*DIMW1+j]._v=(b+1)/2;
	}
	delete[] wsample;
	delete[] hsample;

	for(int i=0;i<=DIMH;i++)
	for(int j=0;j<=DIMW;j++)
	{
		if(j==DIMW)
			vertices[i*DIMW1+j]._nx=vertices[i*DIMW1+j-1]._y-vertices[i*DIMW1+j]._y;
		else if(j==0)
			vertices[i*DIMW1+j]._nx=vertices[i*DIMW1+j]._y-vertices[i*DIMW1+j+1]._y;
		else
			vertices[i*DIMW1+j]._nx=(vertices[i*DIMW1+j-1]._y-vertices[i*DIMW1+j+1]._y)/2;

		vertices[i*DIMW1+j]._nx*=fabs(wscale);

		if(i==DIMH)
			vertices[i*DIMW1+j]._nz=vertices[i*DIMW1+j-DIMW1]._y-vertices[i*DIMW1+j]._y;
		else if(i==0)
			vertices[i*DIMW1+j]._nz=vertices[i*DIMW1+j]._y-vertices[i*DIMW1+j+DIMW1]._y;
		else
			vertices[i*DIMW1+j]._nz=(vertices[i*DIMW1+j-DIMW1]._y-vertices[i*DIMW1+j+DIMW1]._y)/2;
	}
	vb->Unlock();
	terrainmap->UnlockRect(0);
	device->SetTexture(0,terrainmap);
	DWORD* indices;
	ib->Lock(0,0,(void**)&indices,0);
	for(int i=0;i<DIMH;i++)
	for(int j=0;j<DIMW;j++)
	{
		indices[(i*DIMW+j)*6]=i*DIMW1+j;
		indices[(i*DIMW+j)*6+1]=i*DIMW1+j+1;
		indices[(i*DIMW+j)*6+2]=i*DIMW1+j+DIMW1+1;
		indices[(i*DIMW+j)*6+3]=i*DIMW1+j+DIMW1+1;
		indices[(i*DIMW+j)*6+4]=i*DIMW1+j+DIMW1;
		indices[(i*DIMW+j)*6+5]=i*DIMW1+j;
	}
	ib->Unlock();
	Progress(end2,end,PROG_END_VB);
end:
	return ret;
}
bool Setup()
{
	bool ret=true;
	D3DXCOLOR color(1.0f,1.0f,1.0f,1.0f);
	D3DMATERIAL9 mt;
	int HDIMW=DIMW/2,HDIMH=DIMH/2,
		DIMW1=DIMW+1,DIMH1=DIMH+1;
	Progress(end0,end,5);
	D3DXCreateTextureFromFile(device,map_path.c_str(),&terrainmap);
	////////////////////////////////////
	h_map=new float[DIMW1*DIMH1];

	Progress(ends,end,7);
	font=new CD3DFont("Times New Roman",16,0);
	font->InitDeviceObjects(device);
	font->RestoreDeviceObjects();
	TxtRdr=new TextRender(font,30);
	if(TxtRdr->Load((char*)help_file.c_str())!=0&&Windowed)
		return false;

	Progress(end1,end,PROG_START_VB);
	if(!PrepVbIb(HDIMW,HDIMH,DIMW1,DIMH1))
		return false;
	////////////////////////////////////
	device->SetRenderState(D3DRS_NORMALIZENORMALS,true);
	device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);//CCW
	device->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);
	device->SetRenderState(D3DRS_LIGHTING,true);
	device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);

	ZeroMemory(&light,sizeof(light));
	light.Type=D3DLIGHT_POINT;
	light.Diffuse=color;
	light.Ambient=color*0.1f;//0.1
	light.Position=Pos;//D3DXVECTOR3(0.0f,1.0f,-2.0f);
	light.Range=1000;
	light.Attenuation0=0.01f;
	light.Attenuation1=1.0f;
	light.Attenuation2=0.0f;
	device->SetLight(0,&light);
	device->LightEnable(0,true);
	ZeroMemory(&mt,sizeof(mt));
	mt.Diffuse=D3DXCOLOR(1.0,1.0,1.0,1.0);mt.Ambient=D3DXCOLOR(1.0,1.0,1.0,1.0);
	device->SetMaterial(&mt);

	device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
	device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
	device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);
	//normalization cannot be ignored

	D3DXMatrixScaling(&world,wscale,1.,1.);
	D3DXMatrixTranslation(&trans,-Pos.x,-Pos.y,-Pos.z);
	D3DXMatrixRotationX(&Rx,vangle);
	D3DXMatrixRotationY(&Ry,hangle);
	view=trans*Ry*Rx;
	D3DXMatrixPerspectiveFovLH(
		&proj,
		D3DX_PI/3,
		Windowed?(float)HResF/VResF:(float)HRes/VRes,
		0.01f,
		1000.0f);
	device->SetTransform(D3DTS_WORLD,&world);
	device->SetTransform(D3DTS_VIEW,&view);
	device->SetTransform(D3DTS_PROJECTION,&proj);

	device->SetViewport(&vp);
	Progress(end2,end,100);
end:
	return ret;
}
void Cleanup()
{
	safe_release(terrainmap);
	safe_release(vb);
	safe_release(ib);
	if(font!=NULL)
	{
		font->DeleteDeviceObjects();
		delete font;
		font=NULL;
	}
	safe_release(device);
	if(h_map!=NULL)
	{
		delete[] h_map;
		h_map=NULL;
	}
	if(TxtRdr!=NULL)
	{
		delete TxtRdr;
		TxtRdr=NULL;
	}
}
inline void ShowText(int& y,char* text)
{
	font->DrawText(0,y,0xffffff00,text);
	y+=30;
}
#define _ST(y,t) ShowText(y,t)
void ShowStat(int& y)
{
	_ST(y,txt);
	_ST(y,txt1);
	_ST(y,txt2);
	_ST(y,txt3);
	_ST(y,txt4);
}
void ShowHelp(int& y)
{
	int x=0;
	TxtRdr->Render(x,y,0xffffff00);
}
bool Display(float tdelta)
{
	if(!device)
		return false;
	device->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
		0x00000000,100.0f,0);
	device->SetTransform(D3DTS_WORLD,&world);
	device->SetTransform(D3DTS_VIEW,&view);
	device->SetTransform(D3DTS_PROJECTION,&proj);
	device->SetRenderState(D3DRS_FILLMODE,(keystate&(1<<10))?D3DFILL_WIREFRAME:D3DFILL_SOLID);
	device->SetLight(0,&light);
	device->SetViewport(&vp);
	device->BeginScene();
	device->SetStreamSource(0,vb,0,sizeof(vertex));
	device->SetIndices(ib);
	device->SetFVF(vertex::FVF);
	device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,(DIMW+1)*(DIMH+1),0,DIMW*DIMH*2);
	int ypos=0;
	if(keystate&(1<<13))
		ShowStat(ypos);
	if(keystate&(1<<9))
		ShowHelp(ypos);
	device->EndScene();
	return true;
}
bool Present()
{
	device->Present(0,0,0,0);//MessageBox(0,"vb failed",0,0);
	return true;
}
void ProcessMouseMove(float tdelta);
void PhysProc(float tdelta,D3DXVECTOR3 Engine);
bool FrameMove(float tdelta,float rtc)
{
	if(!device)
		return false;
	int pulsed=0;
	D3DXMatrixTranslation(&trans,-Pos.x,-Pos.y,-Pos.z);
	ProcessMouseMove(tdelta);
	view=trans*Ry*Rx;
	D3DXMATRIX rot=Ry*Rx;
	D3DXMatrixTranspose(&rot,&rot);
	D3DXVECTOR3 Xt(1,0,0),Yt(0,1,0),Zt(0,0,1);
	D3DXVec3TransformCoord(&xt,&Xt,&rot);
	D3DXVec3TransformCoord(&yt,&Yt,&rot);
	D3DXVec3TransformCoord(&zt,&Zt,&rot);

	if(walk!=2)keystate&=~(1<<22);
	if(!(keystate&(1<<22)))onland=lately_onland=false;
	D3DXVECTOR3 Engine(0,0,0);
	if(!(keystate&(1<<22)))
	{
		if(keystate&(1<<1))Pos+=-xt*0.2*tdelta;
		if(keystate&(1<<2))Pos+=xt*0.2*tdelta;
		if(keystate&(1<<3))Pos+=zt*0.2*tdelta;
		if(keystate&(1<<4))Pos+=-zt*0.2*tdelta;
		if(keystate&(1<<5))Pos+=yt*0.2*tdelta;
		if(keystate&(1<<6))Pos+=-yt*0.2*tdelta;
	}
	else
	{
		if(keystate&(1<<1)){Engine+=-xt*ENGINE_ACC;pulsed=1;}
		if(keystate&(1<<2)){Engine+=xt*ENGINE_ACC;pulsed=1;}
		if(keystate&(1<<3)){Engine+=zt*ENGINE_ACC;pulsed=1;}
		if(keystate&(1<<4)){Engine+=-zt*ENGINE_ACC;pulsed=1;}
		if(keystate&(1<<5)){Engine+=yt*ENGINE_ACC;pulsed=1;}
		if(keystate&(1<<6)){Engine+=-yt*ENGINE_ACC;pulsed=1;}
	}
	if(keystate&(1<<7)){light.Attenuation0+=light.Attenuation0*tdelta;light.Attenuation1+=light.Attenuation1*tdelta;}
	if(keystate&(1<<8)){light.Attenuation0-=light.Attenuation0*tdelta;light.Attenuation1-=light.Attenuation1*tdelta;}

	if(walk==2)
	{
		if(keystate&(1<<22))
		{
			PhysProc(tdelta,Engine);
		}
		else
		{
			Pos.x=(Pos.x>fabs(wscale)?fabs(wscale):(Pos.x<-fabs(wscale)?-fabs(wscale):Pos.x));
			Pos.z=(Pos.z>1?1:(Pos.z<-1?-1:Pos.z));
			float u=(Pos.x/wscale+1)/2*DIMW;
			float v=(Pos.z       +1)/2*DIMH;
			int U=(int)floorf(u);
			int V=(int)floorf(v);
			float uerp=u-U,verp=v-V;
			if(U<0)U=0;
			if(V<0)V=0;
			if(U>DIMW-1)U=DIMW-1;
			if(V>DIMH-1)V=DIMH-1;CurUV.x=U;CurUV.y=V;
			int DIMW1=DIMW+1;
			Pos.y=h_map[DIMW1*V+U]+(h_map[DIMW1*V+U+1]-h_map[DIMW1*V+U])*uerp+(h_map[DIMW1*(V+1)+U]-h_map[DIMW1*V+U])*verp;
			Pos.y+=0.1;
		}
	}
	if(walk==1)
	{
		if(Pos.x<fabs(wscale)&&Pos.x>-fabs(wscale)&&Pos.y>0&&Pos.y<0.5&&Pos.z>-1&&Pos.z<1)
			walk=2;
	}
	if(keystate&(1<<20))
		light.Position=Pos;
	if(((keystate&(1<<21))>>21)==(walk==0))
		walk=!walk;
	fcon++;
	tcon+=rtc;
	if(tcon>=1.)
	{
		char txtwnd[30]="";
		if(Windowed)
			sprintf(txtwnd,"(Wnd:%d*%d)",HResF,VResF);
		sprintf(txt,"%d*%d%s,%dfps",HRes,VRes,txtwnd,(int)fcon);//
		tcon=0.,fcon=0.;
	}
	if(keystate&(1<<20))sprintf(txt1,"LightTrack:ON");
	else sprintf(txt1,"LightTrack:OFF");
	if(keystate&(1<<22))sprintf(txt3,"PhysMode:ON %d,%d",(int)onland,pulsed);
	else sprintf(txt3,"PhysMode:OFF");
	if(walk==0)sprintf(txt2,"WalkingMode:OFF");
	if(walk==1)sprintf(txt2,"WalkingMode:Landing...");
	if(walk==2)
	{
		sprintf(txt2,"WalkingMode:ON U:%d V:%d",CurUV.x,CurUV.y);
		sprintf(txt4,"Jump:%d%%",(int)(jump_force*100));
	}
	else
		sprintf(txt4,"Jump:0%%");
	//vp.X+=800/8;if(vp.X==800){vp.X=0;vp.Y+=600/8;if(vp.Y==600/8*8)vp.Y=0;}
	//device->SetViewport(&vp);
	return true;
}
void ProcessMouseMove(float tdelta)
{
	if(!mode)
		return;
	const float inv_dim_time=1.0/MOUSE_DRAG;
	float acct=1-exp(-tdelta*inv_dim_time);
	if(acct>1)acct=1;
	D3DXVECTOR2 acc=anglea*acct;
	hangle-=acc.x,vangle-=acc.y;
	anglea-=acc;
	if(hangle<-D3DX_PI)hangle+=D3DX_PI*2;
	if(hangle>D3DX_PI)hangle-=D3DX_PI*2;
	if(vangle<-D3DX_PI/2)vangle=-D3DX_PI/2;
	if(vangle>D3DX_PI/2)vangle=D3DX_PI/2;
	D3DXMatrixRotationX(&Rx,vangle);
	D3DXMatrixRotationY(&Ry,hangle);
}
void ComputeNormal()
{
	CurUVNorm.y=2.0f/DIMH;
	int DIMW1=DIMW+1;
	CurUVNorm.x=(h_map[DIMW1*CurUV.y+CurUV.x]-h_map[DIMW1*CurUV.y+CurUV.x+1])*(wscale<0?-1:1);
	CurUVNorm.z=(h_map[DIMW1*CurUV.y+CurUV.x]-h_map[DIMW1*(CurUV.y+1)+CurUV.x]);
	D3DXVec3Normalize(&CurUVNorm,&CurUVNorm);
}
bool AdjustHeight(float tdelta)
{
	Pos.x=(Pos.x>fabs(wscale)?fabs(wscale):(Pos.x<-fabs(wscale)?-fabs(wscale):Pos.x));
	Pos.z=(Pos.z>1?1:(Pos.z<-1?-1:Pos.z));
	float u=(Pos.x/wscale+1)/2*DIMW;
	float v=(Pos.z       +1)/2*DIMH;
	int U=(int)floor(u);
	int V=(int)floor(v);
	float uerp=u-U,verp=v-V;
	if(U<0)U=0;
	if(V<0)V=0;
	if(U>DIMW-1)U=DIMW-1;
	if(V>DIMH-1)V=DIMH-1;
	CurUV.x=U;CurUV.y=V;
	int DIMW1=DIMW+1;
	float height=h_map[DIMW1*V+U]+(h_map[DIMW1*V+U+1]-h_map[DIMW1*V+U])*uerp+(h_map[DIMW1*(V+1)+U]-h_map[DIMW1*V+U])*verp;
	if(Pos.y<height+0.1)
	{
		Pos.y=height+0.1;
		ComputeNormal();
		Velocity-=D3DXVec3Dot(&Velocity,&CurUVNorm)*CurUVNorm;
		if(accumulate>=0.1)sndPlaySound("landing.wav",SND_ASYNC);
		accumulate=0;
		return true;
	}
	accumulate+=tdelta*(Pos.y-height);
	return false;
}
void JumpProc(float tdelta);
void PhysProc(float tdelta,D3DXVECTOR3 Engine)
{
	if(Pos.x>fabs(wscale)||Pos.x<-fabs(wscale))
	{
		Velocity.x*=-1;
		if(Pos.x>fabs(wscale))Pos.x=fabs(wscale);
		if(Pos.x<-fabs(wscale))Pos.x=-fabs(wscale);
	}
	if(Pos.z>1||Pos.z<-1)
	{
		Velocity.z*=-1;
		if(Pos.z>1)Pos.z=1;
		if(Pos.z<-1)Pos.z=-1;
	}
	if(onland)
	{
		float vlen=D3DXVec3Length(&Velocity);
		if(vlen<FRICTION_ACC*tdelta)
			Velocity=D3DXVECTOR3(0,0,0);
		else
		{
			float newlen=vlen-FRICTION_ACC*tdelta;
			Velocity*=(newlen/vlen);
		}

		Velocity+=(Engine+D3DXVECTOR3(0,-GRAVITY_ACC,0))*tdelta;
		Velocity-=D3DXVec3Dot(&Velocity,&CurUVNorm)*CurUVNorm;
		float u=(Pos.x/wscale+1)/2*DIMW;
		float v=(Pos.z       +1)/2*DIMH;
		int U=(int)floor(u);
		int V=(int)floor(v);
		if(CurUV.x!=U||CurUV.y!=V)
			onland=false;
	}
	else
	{
		Velocity+=D3DXVECTOR3(0,-GRAVITY_ACC,0)*tdelta;
		onland=AdjustHeight(tdelta);
	}
	////////////////////////////////
	JumpProc(tdelta);
	{
		float length=D3DXVec3Length(&Velocity);
		if(length<MIN_SPEED)
			Velocity.x=Velocity.y=Velocity.z=0;
		else if(length>MAX_SPEED)
			Velocity*=(MAX_SPEED/length);
	}
	Pos+=Velocity*tdelta;
}
void JumpProc(float tdelta)
{
	if(onland)
	{
		lately_onland=true;
		onland_fade=0;
	}
	else if(lately_onland)
	{
		onland_fade+=tdelta/ONLAND_FADE_TIME;
		if(onland_fade>=1.0)
		{
			lately_onland=false;
			onland_fade=0;
		}
	}
	////////////////////////////////
	if(jump_swt)
	{
		sndPlaySound("up.wav",SND_ASYNC);
		D3DXMATRIX tmp;
		D3DXVECTOR3 xt(0,0,0);
		D3DXMatrixTranspose(&tmp,&Ry);
		Velocity.y+=JUMP*jump_force;
		if(keystate&(1<<1))xt.x-=0.5*JUMP*jump_force;
		if(keystate&(1<<2))xt.x+=0.5*JUMP*jump_force;
		if(keystate&(1<<3))xt.z+=0.5*JUMP*jump_force;
		if(keystate&(1<<4))xt.z-=0.5*JUMP*jump_force;
		D3DXVec3TransformCoord(&xt,&xt,&tmp);
		Velocity+=xt;
		onland=false;
		lately_onland=false;
		jump_swt=false;
		jump_pre=false;
		jump_force=0.0;
		jump_fade=0.0;
		onland_fade=0.0;
	}
	if(jump_pre)
	{
		jump_force+=tdelta/JUMP_POWERUP_TIME;
		if(!onland)
		{
			jump_fade+=tdelta/JUMP_FADE_TIME;
			if(jump_fade>1.0)
			{
				jump_pre=false;
				jump_force=0.0;
				jump_fade=0;
			}
		}
		if(jump_force>=1.0)
		{
			jump_force=1.0;
			jump_swt=true;
		}
	}
	jump_delay-=tdelta;
	if(jump_delay<0)
		jump_delay=0;
}
void onmdown(HWND hwnd)
{
	mode=(Windowed?(!mode):true);
	ShowCursor(!mode);
}
void onmove(HWND hwnd)
{
	static bool b=true;
	POINT pt={0,0};
	const int limit=50;
	if(!mode)
		return;
	if(b)
	{
		RECT rcClient;
		GetClientRect(hwnd,&rcClient);
		GetCursorPos(&pt);
		ScreenToClient(hwnd,&pt);
		pt.x-=rcClient.right/2;pt.y-=rcClient.bottom/2;
		pt.x=(pt.x>limit?limit:(pt.x<-limit?-limit:pt.x));
		pt.y=(pt.y>limit?limit:(pt.y<-limit?-limit:pt.y));
		anglea.x+=(float)pt.x/200;
		anglea.y+=(float)pt.y/200;
		pt.x=rcClient.right/2;pt.y=rcClient.bottom/2;
		ClientToScreen(hwnd,&pt);
		SetCursorPos(pt.x,pt.y);
	}
	b=!b;
}
void Reset()
{
	Pos.x=Pos.z=0;Pos.y=1;
	Velocity.x=Velocity.y=Velocity.z=0;
	walk=2;
	keystate|=(1<<22);
	keystate|=(1<<21);
	onland=false;
	lately_onland=false;
}
void Jump(bool down)
{
	if(lately_onland&&jump_delay==0&&down)
	{
		jump_pre=true;
		jump_delay=JUMP_INTERVAL;
	}
	if(jump_pre&&!down)
	{
		jump_swt=true;
	}
}
void Action(int id,bool down)
{
	switch(id)
	{
	case 11:
		if(keystate&(1<<22))
			Jump(down);
		break;
	case 12:
		if(down)
			Reset();
		break;
	}
}
void onkey(int key,bool down,E_ONKEY_TYPE type)
{
	static int hitstate=0;
	bool bdown;
	switch(type)
	{
	case eOnKeyTypePressed:
		if(down)
			keystate|=(1<<key);
		else
			keystate&=~(1<<key);
		break;
	case eOnKeyTypeHit:
		bdown=!!(hitstate&(1<<key));
		if(down)
			hitstate|=(1<<key);
		else
			hitstate&=~(1<<key);
		if(bdown&&!down)
			keystate^=(1<<key);
		break;
	case eOnKeyTypeAct:
		bdown=!!(keystate&(1<<key));
		if(down)
			keystate|=(1<<key);
		else
			keystate&=~(1<<key);
		if((!bdown)&&down)
			Action(key,true);
		else if(bdown&&!down)
			Action(key,false);
		break;
	}
}
void OnSize(int width,int height)
{
	HResF=width,VResF=height;
	D3DXMatrixPerspectiveFovLH(
		&proj,
		D3DX_PI/3,
		(float)HResF/VResF,
		0.01f,
		1000.0f);
}
