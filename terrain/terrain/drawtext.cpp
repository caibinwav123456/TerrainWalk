#include "StdAfx.h"
#include "drawtext.h"
int TextRender::Load(char* file)
{
	int ret=0;
	void* hFile=sys_fopen(file,FILE_READ|FILE_OPEN_EXISTING);
	if(!VALID(hFile))
		return ERR_OPEN_FILE_FAILED;
	uint size=0;
	if((ret=sys_get_file_size(hFile,&size))!=0)
		goto end;
	char* buf=new char[size+1];
	if((ret=sys_fread(hFile,buf,size))!=0)
		goto end1;
	buf[size]=0;
	seg_lines(buf);
end1:
	delete[] buf;
end:
	sys_fclose(hFile);
	return ret;
}
void TextRender::seg_lines(char* buf)
{
	texts.clear();
	for(char *ps=buf;*ps;)
	{
		char *pe,*next=NULL;
		for(pe=ps;*pe&&*pe!='\r'&&*pe!='\n';pe++);
		switch(*pe)
		{
		case 0:
			next=pe;
			break;
		case '\r':
			next=pe+1;
			if(*next=='\n')
				next++;
			break;
		case '\n':
			next=pe+1;
			break;
		}
		*pe=0;
		texts.push_back(ps);
		ps=next;
	}
}
void TextRender::Render(int& x,int& y,DWORD color)
{
	for(int i=0;i<(int)texts.size();i++)
	{
		if(!texts[i].empty())
			m_pFont->DrawText(x,y,color,texts[i].c_str());
		y+=m_iLHeight;
	}
}