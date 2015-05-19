
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
vector<string> included_filenames;
bool struct_block = false;

symbol::symbol ( size_t filenr, size_t linenr, size_t offset,
    	size_t blocknr ) :
		filenr (filenr), linenr (linenr), offset (offset),
		blocknr (blocknr) {
	DEBUGF ('f', "symbol %p->{%d:%d.%d: }\n",
        (void*) this, filenr, linenr, offset);
}

void init_symbol_table () {
	(*stack).push_back (new symbol_table ());
}

symbol* lookup (const string* s) {

	auto itor = (*stack).begin ();
	for (size_t i=0; i<next_block; ++i) {
		if((*itor) == NULL) continue;
		const auto& e = (*itor)->find (s);
		if (e != (*itor++)->end()) {
			return e->second;
		}
	}
	return NULL;
}

bool attr_or (attr_bitset bits, ... ) {
	va_list attrs;
	va_start (attrs, bits);
	bool has_attrs = false;
	size_t flag;
	while (flag = va_arg (attrs, size_t)) {
		has_attrs = bits[flag];
		if (has_attrs) break;
	}
	va_end (attrs);
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

string strattr (attr_bitset bits) {
	string s = "";
	for (int i=0; i<ATTR_bitset_size; ++i) {
		if (bits[i]) s += attr_name[i] + " ";
	}
	return s;
}

// make if elses before turn in
string strtype (attr_bitset bits) {
	string s = "";
	if(bits[ATTR_typeid]) {
		s = "typeid";
	}else{
	    if(bits[ATTR_int]) s = "int";
	    if(bits[ATTR_char]) s = "char";
	    if(bits[ATTR_string]) s = "string";
	    if(bits[ATTR_bool]) s = "bool";
	}
	return s;

}

size_t idtype (attr_bitset bits) {
	size_t type = 0;
	if(bits[ATTR_typeid]) {
		type = ATTR_typeid;
	}else{
	    if(bits[ATTR_int]) type = ATTR_int;
	    if(bits[ATTR_char]) type = ATTR_char;
	    if(bits[ATTR_string]) type = ATTR_string;
	    if(bits[ATTR_bool]) type = ATTR_bool;
	}
	return type;
}

bool insert (const string* str, symbol* sym) {
	if  (!((*stack).front()->insert(make_pair(str, sym))).second) {
		return false;
	}
	return true;
}

static attr_bitset get_attr(astree* root) {
	if (root == NULL) {
		errprintf("error in get_attr\n");
		exit(1);
	}
	if (lookup(root->lexinfo)) {
		symbol* sym = lookup(root->lexinfo);
		root->attr = root->attr | sym->attr;
		sym->attr = root->attr | sym->attr;
		return sym->attr;
	}else if (!(root->attr).to_ulong()){
	    errprintf ("%:%s: %d: error: unknown identifier : '%s'\n",
              included_filenames.back().c_str(),
              root->linenr, root->lexinfo->c_str());
		exit(1);
    }
		return root->attr;
}

// need to figure out how to handle blocks
void block (astree* root) {
	if  (root == NULL) return;
	if  (struct_block) return;
	next_block += 1;
	(*stack).push_back(NULL);
	(*stack).push_back(new symbol_table());
}

// write print subtree function for error printing
void equal_op (astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	astree* right = root->children.back();
	attr_bitset rbits = get_attr(right);
	if  (!compat(lbits, rbits)) {
		errprintf ("%:%s: %d: error: incompatible assignment: type '%s' assigned to type '%s'\n",
              included_filenames.back().c_str(),
              root->linenr, strtype(right->attr).c_str(), 
              strtype(left->attr).c_str());
		exit(1);
	}
	else{
		right->attr.set(ATTR_vreg, true);
	}
		return;
}

void plus_op (astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	astree* right = root->children.back();
	attr_bitset rbits = get_attr(right);
	if  ((attr_and(lbits, ATTR_int)) && 
		(attr_and(rbits, ATTR_int))) {
		root->attr.set(ATTR_int, true);
		root->attr.set(ATTR_vreg, true);
	}else{
		errprintf ("%:%s: %d: error: incompatible operands: '%s + %s'\n",
              included_filenames.back().c_str(),
              root->linenr, left->lexinfo->c_str(), 
              right->lexinfo->c_str());
		exit(1);
	}
}

void minus_op (astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	astree* right = root->children.back();
	attr_bitset rbits = get_attr(right);
	if  ((attr_and(lbits, ATTR_int)) && 
		(attr_and(rbits, ATTR_int))) {
		root->attr.set(ATTR_int, true);
		root->attr.set(ATTR_vreg, true);
	}else{
		errprintf ("%:%s: %d: error: incompatible operands: '%s - %s'\n",
              included_filenames.back().c_str(),
              root->linenr, left->lexinfo->c_str(), 
              right->lexinfo->c_str());
		exit(1);
	}
}

void mult_op (astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	astree* right = root->children.back();
	attr_bitset rbits = get_attr(right);
	if  ((attr_and(lbits, ATTR_int)) && 
		(attr_and(rbits, ATTR_int))) {
		root->attr.set(ATTR_int, true);
		root->attr.set(ATTR_vreg, true);
	}else{
		errprintf ("%:%s: %d: error: incompatible operands: '%s * %s'\n",
              included_filenames.back().c_str(),
              root->linenr, left->lexinfo->c_str(), 
              right->lexinfo->c_str());
		exit(1);
	}
}

void div_op (astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	astree* right = root->children.back();
	attr_bitset rbits = get_attr(right);
	if  ((attr_and(lbits, ATTR_int)) && 
		(attr_and(rbits, ATTR_int))) {
		root->attr.set(ATTR_int, true);
		root->attr.set(ATTR_vreg, true);
	}else{
		errprintf ("%:%s: %d: error: incompatible operands : '%s / %s'\n",
              included_filenames.back().c_str(),
              root->linenr, left->lexinfo->c_str(), 
              right->lexinfo->c_str());
		exit(1);
	}
}

void declid (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_DECLID) | 
		(root->symbol == TOK_FIELD) | 
		(root->symbol == TOK_TYPEID)) {
    	symbol *sym = new symbol(root->filenr, 
    	    root->linenr, root->offset, next_block-1);
    	sym->attr.set(ATTR_lval, true);
    	if(insert(root->lexinfo, sym)) {
    		root->attr.set(ATTR_lval, true);
    		return;
    	}else {
    		errprintf ("%:%s: %d: error: redeclaration of identifier : '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->lexinfo->c_str());
		    exit(1);
    	}
    }
}

void vardecl (astree* root) {
    if  (root == NULL) return;
    if  (compat(root->children.front()->attr, 
    	root->children.back()->attr)) {
    	//printf ("setting %s attr to variable", 
    	//	(root->children.front())->children.front()->lexinfo->c_str());
    	size_t type = idtype(root->children.back()->attr);
    	astree* grandchild = root->children.front()->children.front();
  		grandchild->attr.set(ATTR_variable, true);
  		symbol * sym = lookup(grandchild->lexinfo);
  		sym->attr.set(ATTR_variable, true);
  		sym->attr.set(type, true);
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
		symbol* sym = lookup(root->children.front()->lexinfo);
		sym->attr.set(ATTR_int, true);
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
		case ('='):
			foo = equal_op;
			break;
		case ('+'):
			foo = plus_op;
			break;
		case ('-'):
			foo = minus_op;
			break;
		case ('*'):
			foo = mult_op;
			break;
		case ('/'):
			foo = div_op;
			break;
		default:
			foo = donothing;
			break;
	}
	return foo;
}

