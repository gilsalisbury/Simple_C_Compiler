
#include "symboltable.h"
#include "auxlib.h"
#include "lyutils.h"

vector<string> attr_name 
	{  "void", "bool", "char", "int", "null",
       "string", "struct", "array", "function",
       "variable", "field", "typeid", "param",
       "lval", "const", "vreg", "vaddr",
       "bitset_size",
	};

size_t next_block = 1;
symbol_stack* stack = new symbol_stack();


symbol::symbol ( size_t filenr, size_t linenr, size_t offset,
    	size_t blocknr ) :
		filenr (filenr), linenr (linenr), offset (offset),
		blocknr (blocknr) {
	DEBUGF ('f', "symbol %p->{%d:%d.%d: }\n",
        (void*) this, filenr, linenr, offset);
}

// last thing I did tonight
symbol* lookup (string* s) {
	const auto& e = (*stack).front()->find (s);
	if (e == (*stack).front()->end()) {
		return NULL;
	}
	return e->second;
}


bool attr_or(attr_bitset bits, ... ) {
	va_list attrs;
	va_start(attrs, bits);
	bool has_attrs = false;
	size_t flag;
	while(flag = va_arg(attrs, size_t)) {
		has_attrs = bits[flag];
		if (has_attrs) break;
	}
	va_end(attrs);
	return has_attrs;
}

bool attr_and(attr_bitset bits, ... ) {
	va_list attrs;
	va_start(attrs, bits);
	bool has_attrs = false;
	bool flag = 0;
	for(;;) {
		flag = bits[va_arg(attrs, size_t)];
		if (flag == 0) break;
		has_attrs = flag;
	}
	va_end(attrs);
	return has_attrs;
}

static bool compat (attr_bitset Abits, attr_bitset Bbits) {
	attr_bitset bits = Abits & Bbits;
	if (attr_or (bits, ATTR_int, ATTR_char, ATTR_string, 
		ATTR_bool, ATTR_typeid)) {
		return true;
	}
	return false;
}

string getattr (attr_bitset bits) {
	string s = "";
	for (int i=0; i<ATTR_bitset_size; ++i) {
		if (bits[i]) s += attr_name[i] + " ";
	}
	return s;
}

void init_symbol_table () {
	(*stack).push_back(new symbol_table());
}

bool insert (const string* str, symbol* sym) {
	if  (!((*stack).front()->insert(make_pair(str, sym))).second) {
		return false;
	}
	return true;
}

void plus_op (astree* root) {
	if  (root == NULL) return;
	if  (attr_and(root->children.front()->attr, ATTR_int) &&
		 attr_and(root->children.back()->attr, ATTR_int)) {
		root->attr.set(ATTR_int, true);
		root->attr.set(ATTR_vreg, true);
	}else{
		printf("INCORRECT TYPE! --> %s\n", root->lexinfo->c_str());
		exit(1);
	}
}

void declid (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_DECLID) | (root->symbol == TOK_FIELD) | (root->symbol == TOK_TYPEID)) {
    	symbol *sym = new symbol(root->filenr, root->linenr, root->offset, next_block-1);
    	if(insert(root->lexinfo, sym)) {
    		root->attr.set(ATTR_lval, true);
    		return;
    	}else {
    		printf("redeclaration! --> %s\n", root->lexinfo->c_str());
    		exit(1);
    	}
    }
}

void vardecl (astree* root) {
    if  (root == NULL) return;
    if  (compat(root->children.front()->attr, 
    	root->children.back()->attr)) {
    	printf ("setting %s attr to variable", 
    		(root->children.front())->children.front()->lexinfo->c_str());
  		root->children.front()->children.front()->attr.set(ATTR_variable, true);
    }else{
    	printf ("ERROR NON COMPAT TYPES\n");
    	exit(1);
    }
}

void intdec (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_INT)) {
		root->attr.set(ATTR_int, true);
		root->children.front()->attr.set(ATTR_int, true);
    }
}

void chardec (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_CHAR)) {
		root->attr.set(ATTR_char, true);
		root->children.front()->attr.set(ATTR_char, true);
    }
}

void stringdec (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_STRING)) {
		root->attr.set(ATTR_string, true);
		root->children.front()->attr.set(ATTR_string, true);
    }
}

void booldec (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_FALSE) | (root->symbol == TOK_TRUE)) {
		root->attr.set(ATTR_bool, true);
		root->children.front()->attr.set(ATTR_bool, true);
    }
}

void intcon (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_INTCON)) {
		root->attr.set(ATTR_int, true);
		root->attr.set(ATTR_const, true);
    }
    return;
}

void charcon (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_CHARCON)) {
		root->attr.set(ATTR_char, true);
		root->attr.set(ATTR_const, true);
    }
    return;
}


void stringcon (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_STRINGCON)) {
		root->attr.set(ATTR_string, true);
		root->attr.set(ATTR_const, true);
    }
    return;
}

void boolcon (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_FALSE) |
		  (root->symbol == TOK_TRUE)) {
		root->attr.set(ATTR_bool, true);
		root->attr.set(ATTR_const, true);
    }
    return;
}

void donothing (astree* root) {
	for (size_t child = 0; child < root->children.size();
        ++child) {
    }
	return;
}


func getfunc (int TOK_TYPE) {
	func foo;
	switch(TOK_TYPE) {
		case (TOK_VARDECL):
			foo = vardecl;
			break;
		case (TOK_DECLID):
			foo = declid;
			break;
		case (TOK_INT):
			foo = intdec;
			break;
		case (TOK_CHAR):
			foo = chardec;
			break;
		case (TOK_STRING):
			foo = stringdec;
			break;
		case (TOK_BOOL):
			foo = booldec;
			break;
		case (TOK_INTCON):
			foo = intcon;
			break;
		case (TOK_CHARCON):
			foo = charcon;
			break;
		case (TOK_STRINGCON):
			foo = stringcon;
			break;
		case (TOK_TRUE):
			foo = boolcon;
			break;
		case (TOK_FALSE):
			foo = boolcon;
			break;
		case ('+'):
			foo = plus_op;
			break;
		default:
			foo = donothing;
			break;
	}
	return foo;
}

