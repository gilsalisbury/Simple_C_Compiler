// dsalisbu
// 1224878

#ifndef __MANGLE_H__
#define __MANGLE_H__

#include <string>
#include <iostream>
#include <fstream>

#include "astree.h"

using namespace std;


string mangle_global(astree* root);
string mangle_local (astree* root);
string mangle_typenm(astree* root);
string mangle_func  (astree* root);




#endif
