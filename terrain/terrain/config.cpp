#include "config.h"
#include "utility.h"
#include <string.h>
#include <assert.h>
#define token_buf_size 1024
static const byte seps[]={' ','\t','\r','\n'};
static const byte spec_token_ch[]={'_','-','.','/',};
struct token_set
{
	bool alphabet[256];
	token_set()
	{
		memset(alphabet,0,256*sizeof(bool));
		for(int i=(int)'a';i<=(int)'z';i++)
			alphabet[i]=true;
		for(int i=(int)'A';i<=(int)'Z';i++)
			alphabet[i]=true;
		for(int i=(int)'0';i<=(int)'9';i++)
			alphabet[i]=true;
		alphabet['_']=true;
		alphabet['-']=true;
		alphabet['.']=true;
		alphabet['/']=true;
	}
};
bool trim_space(byte c)
{
	for(int i=0;i<sizeof(seps)/sizeof(byte);i++)
	{
		if(c==seps[i])
		{
			return true;
		}
	}
	return false;
}
bool trim_token(byte c)
{
	static spec_char_verifier ts(spec_token_ch,sizeof(spec_token_ch)/sizeof(byte),true);
	return ts.is_spec(c);
}
bool trim_string(byte c)
{
	return c!='\"'&&c!='\\';
}
static inline bool trim_text(const byte* &buf,int& size,bool (*trim_func)(byte))
{
	if(size<=0)
		return false;
	if(!trim_func(*buf))
		return false;
	buf++,size--;
	for(;size>0;buf++,size--)
	{
		if(!trim_func(*buf))
			return true;
	}
	return true;
}
#define make_token(pbuf,buf,token_buf,token) \
	{if(buf-pbuf>token_buf_size)return ERR_BUFFER_OVERFLOW; \
	memcpy(token_buf,pbuf,buf-pbuf); \
	token_buf[buf-pbuf]=0; \
	token=string((const char*)token_buf);}
static int parse_config_text(const byte* buf,int size,
	map<string,map<string,string>>& configs)
{
	string section(DEFAULT_CONFIG_SECTION);
	configs[section];
	byte token_buf[token_buf_size+1];
	string item,val;
	const byte* pbuf;
	int psize;
	while(size>0)
	{
		trim_text(buf,size,trim_space);
		if(size==0)
			break;
		if(*buf=='[')
		{
			buf++,size--;
			trim_text(buf,size,trim_space);
			if(size==0)
				return ERR_BAD_CONFIG_FORMAT;
			pbuf=buf,psize=size;
			if(!trim_text(buf,size,trim_token))
				return ERR_BAD_CONFIG_FORMAT;
			make_token(pbuf,buf,token_buf,section);
			trim_text(buf,size,trim_space);
			if(size==0||*buf!=']')
				return ERR_BAD_CONFIG_FORMAT;
			buf++,size--;
			trim_text(buf,size,trim_space);
			if(size==0)
				break;
		}
		pbuf=buf,psize=size;
		if(!trim_text(buf,size,trim_token))
			return ERR_BAD_CONFIG_FORMAT;
		make_token(pbuf,buf,token_buf,item);
		trim_text(buf,size,trim_space);
		if(size==0||*buf!='=')
		{
			configs[section][item]="";
			continue;
		}
		buf++,size--;
		trim_text(buf,size,trim_space);
		pbuf=buf,psize=size;
		if(size>0&&*buf=='\"')
		{
			buf++,size--;
			pbuf=buf,psize=size;
			byte* ptoken=token_buf;
			while(size>0)
			{
				trim_text(buf,size,trim_string);
				if(size<=0)
					continue;
				if(*buf=='\"')
				{
					if((ptoken-token_buf)+(buf-pbuf)>token_buf_size)
						return ERR_BUFFER_OVERFLOW;
					memcpy(ptoken,pbuf,buf-pbuf);
					ptoken+=buf-pbuf;
					*ptoken=0;
					break;
				}
				else if(*buf=='\\')
				{
					if((ptoken-token_buf)+(buf-pbuf)>token_buf_size)
						return ERR_BUFFER_OVERFLOW;
					memcpy(ptoken,pbuf,buf-pbuf);
					ptoken+=buf-pbuf;
				}
				else
				{
					//should not reach here.
					assert(false);
				}
				buf++,size--;
				if(size>0)
				{
					if((ptoken-token_buf)+1>token_buf_size)
						return ERR_BUFFER_OVERFLOW;
					*ptoken=*buf;
					ptoken++;
					buf++,size--;
					pbuf=buf,psize=size;
				}
			}
			if(size==0)
				return ERR_BAD_CONFIG_FORMAT;
			//make_token(pbuf,buf,token_buf,val);
			val=(char*)token_buf;
			configs[section][item]=val;
			buf++,size--;
			continue;
		}
		else if(!trim_text(buf,size,trim_token))
			return ERR_BAD_CONFIG_FORMAT;
		make_token(pbuf,buf,token_buf,val);
		configs[section][item]=val;
	}
	return 0;
}
int ConfigProfile::LoadConfigFile(const char* filename)
{
	int ret=0;
	uint filesize=0;
	byte* buf;
	void* hFile=sys_fopen(const_cast<char*>(filename),FILE_OPEN_EXISTING|FILE_READ);
	if(!VALID(hFile))
		return ERR_OPEN_FILE_FAILED;
	if(0!=(ret=sys_get_file_size(hFile,&filesize)))
		goto final;
	buf=new byte[filesize];
	if(0!=(ret=sys_fread(hFile,buf,filesize)))
		goto final1;
	configs.clear();
	if(0!=(ret=parse_config_text(buf,filesize,configs)))
		configs.clear();
final1:
	delete[] buf;
final:
	sys_fclose(hFile);
	return ret;
}
bool ConfigProfile::GetCongfigItem(const string& primary, const string& secondary, string& val)
{
	if(configs.find(primary)==configs.end())
		return false;
	if(configs[primary].find(secondary)==configs[primary].end())
		return false;
	val=configs[primary][secondary];
	return true;
}
bool ConfigProfile::GetCongfigItem(const string& primary, const string& secondary, bool& val)
{
	string s;
	if(!GetCongfigItem(primary,secondary,s))
		return false;
	val=TranslateBool(s);
	return true;
}
bool ConfigProfile::TranslateBool(const string& str)
{
	int i=0;
	if(str==string("true"))
		return true;
	else if(str==string("false"))
		return false;
	else
	{
		sscanf(str.c_str(),"%d",&i);
		return !!i;
	}
}
bool ConfigProfile::GetCongfigItem(const string& primary, const string& secondary, int& val)
{
	string s;
	if(!GetCongfigItem(primary,secondary,s))
		return false;
	val=TranslateInt(s);
	return true;
}
int ConfigProfile::TranslateInt(const string& str)
{
	int i;
	sscanf(str.c_str(),"%d",&i);
	return i;
}
bool ConfigProfile::GetCongfigItem(const string& primary, const string& secondary, uint& val)
{
	string s;
	if(!GetCongfigItem(primary,secondary,s))
		return false;
	val=TranslateUint(s);
	return true;
}
uint ConfigProfile::TranslateUint(const string& str)
{
	int i=0;
	sscanf(str.c_str(),"%d",&i);
	return (uint)i;
}
bool ConfigProfile::GetCongfigItem(const string& primary, const string& secondary, float& val)
{
	string s;
	if(!GetCongfigItem(primary,secondary,s))
		return false;
	val=TranslateFloat(s);
	return true;
}
float ConfigProfile::TranslateFloat(const string& str)
{
	float f=0;
	sscanf(str.c_str(),"%f",&f);
	return f;
}
bool ConfigProfile::GetCongfigItem(const string& primary, const string& secondary, double& val)
{
	string s;
	if(!GetCongfigItem(primary,secondary,s))
		return false;
	val=TranslateDouble(s);
	return true;
}
double ConfigProfile::TranslateDouble(const string& str)
{
	double db=0;
	sscanf(str.c_str(),"%lf",&db);
	return db;
}
static inline char lower_case(char c)
{
	if(c>='A'&&c<='Z')
		return c-'A'+'a';
	return c;
}
static inline string to_lower(const string& str)
{
	string lows;
	char s[2]={0,0};
	for(int i=0;i<(int)str.size();i++)
	{
		s[0]=lower_case(str[i]);
		lows+=s;
	}
	return lows;
}
uint ConfigProfile::TranslateSize(const string& str)
{
	char* s=(char*)str.c_str();
	char buf[40],unit[8];
	char *p,*pbuf,*punit;
	for(p=s,pbuf=buf;*p!=0&&pbuf-buf<39;p++,pbuf++)
	{
		if(*p>='0'&&*p<='9')
		{
			*pbuf=*p;
		}
		else
			break;
	}
	if(pbuf-buf==39)
		return 0;
	*pbuf=0;
	for(punit=unit;*p!=0&&punit-unit<7;p++,punit++)
	{
		if((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z'))
		{
			*punit=*p;
		}
		else
			break;
	}
	*punit=0;
	if(punit-unit==7)
		return 0;
	if(*p!=0)
	{
		if(*p!=' '&&*p!='\r'&&*p!='\n'&&*p!='\t')
			return 0;
	}
	int n=0;
	sscanf(buf,"%d",&n);
	string sunit(unit);
	uint nunit=1;
	string lunit=to_lower(sunit);
	if(lunit=="k"||lunit=="kb"||lunit=="kib")
		nunit=1024;
	else if(lunit=="m"||lunit=="mb"||lunit=="mib")
		nunit=1024*1024;
	else if(sunit=="")
		nunit=1;
	else
		return 0;
	return ((uint)n)*nunit;
}
ConfigProfile::iterator ConfigProfile::BeginIterate(const string& primary)
{
	iterator it;
	if(configs.find(primary)==configs.end())
	{
		it.valid=false;
	}
	else
	{
		it.valid=true;
		it.iter=configs[primary].begin();
		it.end=configs[primary].end();
	}
	return it;
}