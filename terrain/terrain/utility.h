#include "common.h"
#include <string.h>
class DLL spec_char_verifier
{
public:
	spec_char_verifier(const byte* ch,uint nch,bool balphanum=false,bool _etc_spec=false);
	bool is_spec(char c) const;
private:
	bool alphabet[128];
	bool etc_spec;
};
