#include "utility.h"
spec_char_verifier::spec_char_verifier(const byte* ch,uint nch,bool balphanum,bool _etc_spec):etc_spec(_etc_spec)
{
	memset(alphabet,0,128*sizeof(bool));
	if(balphanum)
	{
		for(int i=(int)'a';i<=(int)'z';i++)
			alphabet[i]=true;
		for(int i=(int)'A';i<=(int)'Z';i++)
			alphabet[i]=true;
		for(int i=(int)'0';i<=(int)'9';i++)
			alphabet[i]=true;
	}
	for(int i=0;i<(int)nch;i++)
		alphabet[(uint)(uchar)ch[i]]=true;
}
bool spec_char_verifier::is_spec(char c) const
{
	int i=(int)c;
	if(i<0||i>=128)
		return etc_spec;
	else
		return alphabet[i];
}
