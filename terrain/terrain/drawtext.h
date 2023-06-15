#ifndef _DRAW_TEXT_H_
#define _DRAW_TEXT_H_
#include "common.h"
#include "d3dfont.h"
#include <string>
#include <vector>
using namespace std;
class TextRender
{
public:
	TextRender(CD3DFont* pFont,int height):m_pFont(pFont),m_iLHeight(height){};
	int Load(char* file);
	void Render(int& x,int& y,DWORD color);
private:
	void seg_lines(char* buf);
	vector<string> texts;
	CD3DFont* m_pFont;
	int m_iLHeight;
};
#endif