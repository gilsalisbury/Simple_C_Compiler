#ifndef __EMIT_H__
#define __EMIT_H__

#include "astree.h"

void emit_ta_code (FILE* outfile, astree* tree);
void prefix_struct (astree* node);
void prefix_strcon (astree* node);
void prefix_vars (astree* node);
void prefix_func (astree* root, astree* name);

RCSH("$Id: emit.h,v 1.1 2013-09-19 16:38:25-07 - - $")
#endif
