#ifndef _CONFIG_VAL_H_
#define _CONFIG_VAL_H_
#include "common.h"
#include "config.h"
#include "config_val_extern.h"
#include <map>
#include <string>
using namespace std;

struct val_info
{
	uint type;
	void* ptr_val;
};
class config_val_container
{
public:
	int config_value(ConfigProfile* config);
	void add_config_val(const string& str, uint type, void* ptr_val);
	static config_val_container* get_val_container();
private:
	map<string,val_info> val_map;
};

#endif
