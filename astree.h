// dsalisbu
// 1224878

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
#include <bitset>
using namespace std;
#include "auxlib.h"


enum { ATTR_void, ATTR_bool, ATTR_char, ATTR_int, ATTR_null,
       ATTR_string, ATTR_struct, ATTR_array, ATTR_function,
       ATTR_variable, ATTR_field, ATTR_typeid, ATTR_param,
       ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr,
       ATTR_bitset_size,
};


using attr_bitset = bitset<ATTR_bitset_size>;

struct astree {
   int symbol;               // token code
   size_t filenr;            // index into filename stack
   size_t linenr;            // line number from source code
   size_t offset;            // offset of token with current line
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node
   attr_bitset attr;
   astree (int symbol, int filenr, int linenr,
           int offset, const char* clexinfo);
};

// Append one child to the vector of children.
astree* adopt1 (astree* root, astree* child);

// Append two children to the vector of children.
astree* adopt2 (astree* root, astree* left, astree* right);

// Adopt all grandchildren of node child
astree* adoptall (astree* root, astree* child);

// Makes an otherwise younger sibling firstborn
astree* makeheir (astree* root, astree* child);

// Make new node with synthetic Token, adopt donor
astree* synthtok (int symbol, astree* donor);

// Dump an astree to a FILE.
void dump_astree (FILE* outfile, astree* root);

// Dump an astree to a FILE.
void tok_dump_astree (FILE* outfile, astree* root);

// Begin typecheck AST.
void type_ast (FILE* outfile, astree* root);

// Debug print an astree.
void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep);

// Recursively free an astree.
void free_ast (astree* tree);

// Recursively free two astrees.
void free_ast2 (astree* tree1, astree* tree2);

RCSH("$Id: astree.h,v 1.4 2015-04-09 19:31:47-07 - - $")
#endif
