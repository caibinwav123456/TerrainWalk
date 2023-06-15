#include "config_val.h"
config_val_container* config_val_container::get_val_container()
{
	static config_val_container container;
	return &container;
}
int config_val_container::config_value(ConfigProfile* config)
{
	for(ConfigProfile::iterator it=config->BeginIterate(CONFIG_SECTION_VARIABLES);it;++it)
	{
		if(it->first!="")
		{
			if(val_map.find(it->first)!=val_map.end())
			{
				const val_info& info=val_map[it->first];
				switch(info.type)
				{
				case CONFIG_VAL_TYPE_BOOL:
					*(bool*)(info.ptr_val)=ConfigProfile::TranslateBool(it->second);
					break;
				case CONFIG_VAL_TYPE_INT:
					*(int*)(info.ptr_val)=ConfigProfile::TranslateInt(it->second);
					break;
				case CONFIG_VAL_TYPE_UINT:
					*(uint*)(info.ptr_val)=ConfigProfile::TranslateUint(it->second);
					break;
				case CONFIG_VAL_TYPE_FLOAT:
					*(float*)(info.ptr_val)=ConfigProfile::TranslateFloat(it->second);
					break;
				case CONFIG_VAL_TYPE_DOUBLE:
					*(double*)(info.ptr_val)=ConfigProfile::TranslateDouble(it->second);
					break;
				case CONFIG_VAL_TYPE_SIZE:
					*(uint*)(info.ptr_val)=ConfigProfile::TranslateSize(it->second);
					break;
				case CONFIG_VAL_TYPE_STRING:
					*(string*)(info.ptr_val)=it->second;
					break;
				default:
					return ERR_INVALID_TYPE;
				}
			}
		}
	}
	val_map.clear();
	return 0;
}
void config_val_container::add_config_val(const string& str, uint type, void* ptr_val)
{
	val_info info;
	info.type=type;
	info.ptr_val=ptr_val;
	val_map[str]=info;
}
config_val::config_val(char* val_str, uint type, void* ptr_val)
{
	config_val_container::get_val_container()->add_config_val(string(val_str), type, ptr_val);
}
