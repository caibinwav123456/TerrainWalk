#include "ASTError.h"
#include "utility.h"
#undef DEF_ERROR
#define DEF_ERROR(code,str) str
static char* err_descs[]=
{
#include "error.h"
};
DLLAPI(char*) get_error_desc(int errcode)
{
	if(_r(errcode)<0||_r(errcode)>=sizeof(err_descs)/sizeof(char*))
		errcode=ERR_GENERIC;
	return err_descs[_r(errcode)];
}
