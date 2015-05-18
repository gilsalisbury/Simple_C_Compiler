#ifndef __SYMBOLTABLE_H__
#define __SYMBOLTABLE_H__



#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>
#include "astree.h"

using namespace std;

struct symbol;
using symbol_table = unordered_map<const string*,symbol*>;
using symbol_entry = symbol_table::value_type;
using symbol_stack = vector<symbol_table*>;



struct symbol {
    attr_bitset attr;
    symbol_table* fields;
    size_t filenr, linenr, offset;
    size_t blocknr;
    vector<symbol*>* parameters;
    symbol( size_t filenr, size_t linenr, size_t offset,
    	size_t blocknr );
};


void init_symbol_table();
bool attr_and(attr_bitset, ... );
bool attr_or(attr_bitset, ... );
bool insert (const string* str, symbol* sym);
string strattr (attr_bitset bits);


typedef void (*func) (astree* root);
void vardecl_type(astree* root);
func getfunc(int TOK_TYPE);


#endif