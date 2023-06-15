#ifndef _CONFIG_VAL_EXTERN_H_
#define _CONFIG_VAL_EXTERN_H_
#include "common.h"

#define CONFIG_VAL_TYPE_BOOL 0
#define CONFIG_VAL_TYPE_INT  1
#define CONFIG_VAL_TYPE_UINT 2
#define CONFIG_VAL_TYPE_FLOAT 3
#define CONFIG_VAL_TYPE_DOUBLE 4
#define CONFIG_VAL_TYPE_STRING 5
#define CONFIG_VAL_TYPE_SIZE 6

#define DECLARE_BOOL_VAL(name) extern bool name
#define DECLARE_INT_VAL(name) extern int name
#define DECLARE_UINT_VAL(name) extern uint name
#define DECLARE_FLOAT_VAL(name) extern float name
#define DECLARE_DOUBLE_VAL(name) extern double name
#define DECLARE_STRING_VAL(name) extern string name
#define DECLARE_SIZE_VAL(name) extern uint name

#define DEFINE_BOOL_VAL(name,def_val) bool name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_BOOL,&name)
#define DEFINE_INT_VAL(name,def_val) int name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_INT,&name)
#define DEFINE_UINT_VAL(name,def_val) uint name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_UINT,&name)
#define DEFINE_FLOAT_VAL(name,def_val) float name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_FLOAT,&name)
#define DEFINE_DOUBLE_VAL(name,def_val) double name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_DOUBLE,&name)
#define DEFINE_STRING_VAL(name,def_val) string name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_STRING,&name)
#define DEFINE_SIZE_VAL(name,def_val) uint name(def_val);config_val __config_##name(#name,CONFIG_VAL_TYPE_SIZE,&name)

class DLL config_val
{
public:
	config_val(char* val_str, uint type, void* ptr_val);
};

#endif