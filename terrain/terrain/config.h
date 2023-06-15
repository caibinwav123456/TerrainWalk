#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "common.h"
#include <map>
#include <string>
using namespace std;
class ConfigProfile
{
public:
	class iterator
	{
		friend class ConfigProfile;
		bool valid;
		map<string,string>::iterator iter;
		map<string,string>::iterator end;
		iterator(){}
	public:
		operator bool()
		{
			if(!valid)
				return false;
			return valid=(iter!=end);
		}
		iterator operator++(int)
		{
			iterator it=*this;
			if(valid)
				iter++;
			return it;
		}
		iterator& operator++()
		{
			if(valid)
				++iter;
			return *this;
		}
		pair<const string,string>& operator*()
		{
			return *iter;
		}
		pair<const string,string>* operator->()
		{
			return &*iter;
		}
	};
	int LoadConfigFile(const char* filename);
	bool GetCongfigItem(const string& primary, const string& secondary, string& val);
	bool GetCongfigItem(const string& primary, const string& secondary, bool& val);
	bool GetCongfigItem(const string& primary, const string& secondary, int& val);
	bool GetCongfigItem(const string& primary, const string& secondary, uint& val);
	bool GetCongfigItem(const string& primary, const string& secondary, float& val);
	bool GetCongfigItem(const string& primary, const string& secondary, double& val);
	static bool TranslateBool(const string& str);
	static int TranslateInt(const string& str);
	static uint TranslateUint(const string& str);
	static float TranslateFloat(const string& str);
	static double TranslateDouble(const string& str);
	static uint TranslateSize(const string& str);
	iterator BeginIterate(const string& primary);
private:
	map<string,map<string,string>> configs;
};
#endif