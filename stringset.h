// $Id: stringset.h,v 1.2 2015-04-13 19:15:04-07 - - $
// dsalisbu
// 1224878

#ifndef __STRINGSET__
#define __STRINGSET__

#include <iostream>
#include <string>

const std::string* intern_stringset (const char*);

void dump_stringset (std::ostream&);

#endif
