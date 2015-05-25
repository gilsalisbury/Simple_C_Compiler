
#include "symboltable.h"
#include "auxlib.h"
#include "lyutils.h"

vector<string> attr_name 
	{  "void", "bool", "char", "int", "null",
       "string", "struct", "array", "function",
       "variable", "field", "typeid", "param",
       "lval", "const", "vreg", "vaddr", "prototype",
       "bitset_size",
	};

int blocknr = 0;
int next_block = 1;

vector<int>* block_count = new vector<int>;
symbol_stack* gc = new symbol_stack();
symbol_stack* func_stack = new symbol_stack();
symbol_stack* stack = new symbol_stack();
symbol_stack* struct_stack = new symbol_stack();
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
	block_count->push_back(blocknr);
	(*stack).push_back (new symbol_table ());
	(*struct_stack).push_back (new symbol_table ());
}

symbol* lookup (const string* s) {
	symbol* sym;
	auto itor = (*stack).rbegin ();
	while(itor != (*stack).rend()) {
		if((*itor) == NULL) continue;
		const auto& e = (*itor)->find (s);
		if (e != (*(itor++))->end()) {
			sym = e->second;
			break;
		}else sym = NULL;
	}
	return sym;
}


symbol* lookup_struct (const string* s) {
	symbol* sym;
	auto itor = (*struct_stack).rbegin ();
	while(itor != (*struct_stack).rend()) {
		if((*itor) == NULL) continue;
		const auto& e = (*itor)->find (s);
		if (e != (*(itor++))->end()) {
			sym = e->second;
			break;
		}else sym = NULL;
	}
	return sym;
}


symbol* lookup_func (const string* s) {
	symbol* sym;
	cout << "in look up func" << endl;
	cout << "looking for: " << s->c_str() << endl;
	auto b = stack->front()->begin();
	while (b != stack->front()->end()) {
		cout << "have members: " << (b++)->first->c_str() << endl;
	}

	const auto& e = stack->at(0)->find (s);
	if (e != stack->at(0)->end()) {
		sym = e->second;
	}else sym = NULL;
	cout << "exit lookup func" << endl;
	return sym;
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



string strattr (astree* root) {
	attr_bitset bits = root->attr;
	string s = "";
	for (int i=0; i<ATTR_bitset_size; ++i) {
		if (bits[i]) {
			s += attr_name[i] + " ";
			if (bits[i] && i==ATTR_struct) {
				if (root->typenm)s += "\""+*root->typenm+"\" ";
			}if (bits[i] && i==ATTR_field) {
				if (root->oftype) s += "{"+*root->oftype+"} ";
			}
		}
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


int idtype (attr_bitset bits) {
	int type = 0;
	if(bits[ATTR_struct]) {
		type = ATTR_struct;
	}else{
	    if(bits[ATTR_int]) type = ATTR_int;
	    if(bits[ATTR_char]) type = ATTR_char;
	    if(bits[ATTR_string]) type = ATTR_string;
	    if(bits[ATTR_bool]) type = ATTR_bool;
	    if(bits[ATTR_void]) type = ATTR_void;
	    if(bits[ATTR_null]) type = ATTR_null;
	}
	return type;
}


bool insert (const string* str, symbol* sym) {
	if  ( (*stack).back() == NULL ) {
		errprintf("error stack is null\n");
		exit(1);
	}
	if  (!((*stack).back()->insert(make_pair(str, sym))).second) {
		return false;
	}
	return true;
}


bool insert_struct (const string* str, symbol* sym) {
	if  ( (*struct_stack).back() == NULL ) {
		errprintf("error stack is null\n");
		exit(1);
	}
	if  (!((*struct_stack).back()->insert(make_pair(str, sym))).second) {
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


void enter_block () {
	printf("\nPushing symbol table %d\n", next_block);
	next_block += 1;
	blocknr += 1;
	block_count->push_back(blocknr);
	(*stack).push_back(new symbol_table());
}


void block (astree* root) {
	if  (root == NULL) return;
	if  (struct_block) return;

	symbol_table* table = stack->back();
    gc->push_back(table);
	stack->pop_back();
	block_count->pop_back();
	next_block -= 1;
	printf("\nExiting block %d\n", next_block);
}


void ifelse_op(astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	if (lbits[ATTR_bool]) {
		printf("attrs are %s\n", left->lexinfo->c_str());
		return;
	}else{
		errprintf ("%:%s: %d: error: required type bool, found type '%s'\n",
              included_filenames.back().c_str(),
              root->linenr, strtype(left->attr).c_str());
		exit(1);
	}
}


void while_op(astree* root) {
	if  (root == NULL) return;
	astree* left = root->children.front();
	attr_bitset lbits = get_attr(left);
	if (lbits[ATTR_bool]) {
		printf("attrs are %s\n", left->lexinfo->c_str());
		return;
	}else{
		errprintf ("%:%s: %d: error: required type bool, found type '%s'\n",
              included_filenames.back().c_str(),
              root->linenr, strtype(left->attr).c_str());
		exit(1);
	}
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
	cout << "in declid" << endl;
	if  (root == NULL) return;
	if  ((root->symbol == TOK_DECLID) | 
		(root->symbol == TOK_FIELD) | 
		(root->symbol == TOK_TYPEID)) {
    	symbol *sym = new symbol(root->filenr, 
    	    root->linenr, root->offset, block_count->back());
    	sym->attr.set(ATTR_lval, true);
    	if(insert(root->lexinfo, sym)) {
    		cout << "inserted: " << root->lexinfo->c_str() << endl;
    		root->attr.set(ATTR_lval, true);
    		cout << "exit declid" << endl << endl;
    		return;
    	}else {
    		errprintf ("%:%s: %d: error: redeclaration of identifier : '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->lexinfo->c_str());
		    exit(1);
    	}
    }
}


// need dump_node funct
void vardecl (astree* root) {
	printf("entered vardecl\n");
    if  (root == NULL) return;
    if  (compat(root->children.front()->attr, 
    	root->children.back()->attr)) {
    	int type = idtype(root->children.back()->attr);
    	astree* grandchild = root->children.front()->children.front();
  		grandchild->attr.set(ATTR_variable, true);
  		symbol * sym = lookup(grandchild->lexinfo);
  		sym->attr.set(ATTR_variable, true);
  		sym->attr.set(type, true);
    }else{
    	errprintf ("%:%s: %d: error: invalid declaration : '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->children.front()->lexinfo->c_str());
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


void call_op(astree* root) {
	if (root == NULL) return;
	symbol* func = lookup_func(root->children[0]->lexinfo); 
	if (!func | !func->attr[ATTR_function]) {
		errprintf ("%:%s: %d: error: calling undefinied function '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->children[0]->lexinfo->c_str());
		    exit(1);
	}else if (func->parameters->size() != root->children.size()-1) {
		errprintf ("%:%s: %d: error: call does not match function"
			" definition '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->children[0]->lexinfo->c_str());
		    exit(1);
	}
	cout << "made it here" << endl;
	for (size_t i = 0; i< root->children.size()-1; ++i) {
		astree* param = root->children[i+1];
		// come back to struct case
		if (param->attr[ATTR_struct]) {
			if (lookup(param->lexinfo)->fields == NULL) {
				errprintf ("%:%s: %d: error: in function call '%s'"
				": incomplete type definition for parameter '%s'\n",
            	included_filenames.back().c_str(),
           		param->linenr, 
           		root->children[0]->lexinfo->c_str(),
           		param->lexinfo->c_str());
		    	exit(1);
			}
		}
		cout << "made it here" << endl;
		symbol* sym = lookup(param->lexinfo);
		// COMPAT DOES NOT WORK!!!!!!!!!!!!!!!!!
		//if (idtype(sym->attr) != idtype(func->parameters->at(i)->attr)) {
		if (!compat(sym->attr, func->parameters->at(i)->attr)) {
			errprintf ("%:%s: %d: error: invalid parameters in function: '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->children[i]->lexinfo->c_str());
		    exit(1);
		}
	}
	int ftype = idtype(func->attr);
	if (ftype == ATTR_struct) {

	}else{
		root->attr.set(ftype, true);
	}
}


void proto_op (astree* root) {
	if (root == NULL) return;
	printf("\n\nHERE\n\n");
	astree* grandchild = root->children[0]->children[0];
	symbol* func = lookup_func(grandchild->lexinfo); 
	if  (next_block != 2) {
		errprintf ("%:%s: %d: error: nested function definitions not allowed '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, grandchild->lexinfo->c_str());
		    exit(1);
	}
	if (func) {
		errprintf ("%:%s: %d: error: prototype redefines existing identifier '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, grandchild->lexinfo->c_str());
		    exit(1);
	}
	// Initalize new function symbol 	
	func = new symbol (root->filenr, 
    	    root->linenr, root->offset, 0);
	func->parameters = new vector<symbol*>;
    func->attr.set(ATTR_prototype, true);
    func->lexinfo = root->children[0]->lexinfo;
    grandchild->attr |= func->attr;
    func->attr |= grandchild->attr;
    astree* params = root->children[1];

    // push params into function symbol's paramlist
	for (size_t i = 0; i < params->children.size(); ++i) {
        symbol* sym = lookup(params->children[i]->children[0]->lexinfo);
        func->parameters->push_back(sym);
    }
    // pop function symbol table off of variable stack
    // and throw it in the garbage stack for storage
    symbol_table* table = stack->back();
    gc->push_back(table);
    stack->pop_back();
    block_count->pop_back();
    --next_block;
    // insert the function symbol into stack error if already there.
	if(insert(grandchild->lexinfo, func)) {
		return;
	}else{
		errprintf ("%:%s: %d: error: prototype redefines existing function '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, grandchild->lexinfo->c_str());
		    exit(1);
	}

}


void func_op (astree* root) {
	if (root == NULL) return;
	astree* grandchild = root->children[0]->children[0];
	symbol* func = lookup_func(grandchild->lexinfo); 
	cout << func << endl;
	if (next_block != 2) {
		errprintf ("%:%s: %d: error: nested "
			"function definitions not allowed '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, grandchild->lexinfo->c_str());
		    exit(1);
	}
	if (func) {
		if (!func->attr[ATTR_prototype]) {
			cout << strattr(grandchild) << endl << endl;
			errprintf ("%:%s: %d: error: function "
				"re-defines existing identifier: '%s'\n",
        		included_filenames.back().c_str(),
        		root->linenr, root->lexinfo->c_str());
			exit(1);
		}
		// then we type check 
		if (func->attr[ATTR_prototype]) {
			astree* params = root->children.at(1);
			int ftype = idtype(func->attr);
			if (ftype == ATTR_struct) {
				if (lookup_struct(grandchild->typenm)) {
					symbol* sym = lookup_struct(grandchild->typenm);
					if (!sym->fields) {
						errprintf ("%:%s: %d: error: function returns "
							"a type that is undefined '%s'\n",
            				included_filenames.back().c_str(),
            				root->linenr, grandchild->lexinfo->c_str());
		    			exit(1);
		    		}
		    		//cout << "HERERERERERERERERERERERERERERERERERERE" << endl;
		    		cout << grandchild->typenm->c_str() << "     " << func->lexinfo->c_str() << endl;
		    		if (grandchild->typenm != func->lexinfo) {
		    			errprintf ("%:%s: %d: error: function '%s' returns "
							"a differnet type from its prototype\n",
            				included_filenames.back().c_str(),
            				root->linenr, grandchild->lexinfo->c_str());
		    			exit(1);
		    		}
				}
			}if((params->children.size() != func->parameters->size()) |
				(!root->children[0]->attr[ftype])) {
				errprintf ("%:%s: %d: error: function does not "
							"match prototype definition '%s'\n",
            				included_filenames.back().c_str(),
            				root->linenr, grandchild->lexinfo->c_str());
		    	exit(1);
			} 
			for (size_t i = 0; i < params->children.size(); ++i) {
				astree* param = params->children[i]->children[0];
				symbol * sym = lookup 
					(param->lexinfo);
				attr_bitset bits = sym->attr | 
								func->parameters->at(i)->attr;
        		if ((bits != sym->attr) | (sym->lexinfo != 
        						func->parameters->at(i)->lexinfo)) {
        			errprintf ("%:%s: %d: error: function '%s' param"
						"meters do not match prototype definition\n",
            			included_filenames.back().c_str(),
            			root->linenr, grandchild->lexinfo->c_str());
		    		exit(1);
        		}
    		}
    		func->attr.set(ATTR_prototype, false);
    		func->attr.set(ATTR_function, true);
    		grandchild->attr.set(ATTR_function, true);
    		symbol_table* table = stack->back();
    		gc->push_back(table);
    		stack->pop_back();
    		block_count->pop_back();
    		--next_block;
		}
	}else{

		func = new symbol (root->filenr, 
    	    root->linenr, root->offset, 0);
		if (grandchild->attr[ATTR_struct]) {
			//getname for array case
			symbol* strct = lookup_struct(grandchild->typenm);
			if (!strct | !strct->fields) {
				errprintf ("%:%s: %d: error: funciton '%s' returns undefined type\n",
        		included_filenames.back().c_str(),
        		root->linenr, root->lexinfo->c_str());
				exit(1);
			}
		}
		func->parameters = new vector<symbol*>;
		grandchild->attr |= root->children[0]->attr;
		grandchild->attr.set(ATTR_function, true);
		func->attr |= grandchild->attr;
		astree* paramlst = root->children.at(1);
	    for (size_t i = 0; i < paramlst->children.size(); ++i) {   
       	    symbol* sym = lookup(paramlst->
       	    	            children[i]->children[0]->lexinfo);
            func->parameters->push_back(sym);
        }
        symbol_table* table = stack->back();
    	gc->push_back(table);
    	stack->pop_back();
    	block_count->pop_back();
    	--next_block;
    	if (!insert(grandchild->lexinfo, func)) {
    		errprintf ("%:%s: %d: error: funciton redefinition '%s'\n",
        		included_filenames.back().c_str(),
        		root->linenr, root->lexinfo->c_str());
			exit(1);
    	}
	}
}

// add type checking
void paramlist (astree* root) {
	cout << "in paramlist" << endl;
	if  (root == NULL) return;
	for (size_t i = 0; i< root->children.size(); ++i) {
		if (root->children[i]->attr[ATTR_struct]) {
			if (lookup_struct(root->children[i]->
					lexinfo)->fields == NULL) {
				errprintf ("%:%s: %d: error: undefined type '%s'\n",
            	included_filenames.back().c_str(),
           		root->children[i]->linenr, 
           		root->children[i]->lexinfo->c_str());
		    	exit(1);
			}
		}
		astree* grandchild = root->children[i]->children[0];
		cout << "looking up " << grandchild->lexinfo->c_str() << endl;
		symbol* sym = lookup(grandchild->lexinfo);
		sym->lexinfo = grandchild->lexinfo;
		sym->attr |= root->children[i]->attr;
		sym->attr.set(ATTR_param, true);
		grandchild->attr |= sym->attr;
	}
	cout << "exit paramlist" << endl;
}


// print entire subtre
void struct_op (astree* root) {
	if  (root == NULL) return;
	if  (next_block != 2) {
		errprintf ("%:%s: %d: error: nested struct definitions not allowed '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->lexinfo->c_str());
		    exit(1);
	}
    
    for (size_t i = 0; i < root->children.size(); ++i) {
    	astree* child = root->children[i];
        if(child->children.size() == 1) {
        	child->children[0]->oftype = root->children[0]->typenm;
        }
    }
    astree* child = root->children[0];
    symbol_table* table = stack->back();
    symbol* sym = lookup_struct(root->children[0]->lexinfo);
    sym->lexinfo = child->lexinfo;
    sym->fields = table;
    stack->pop_back();
    block_count->pop_back();
    --next_block;
	return;
}


void type_id (astree* root) {
	
	if  (root == NULL) return;
	root->typenm = root->lexinfo;
	root->oftype = root->lexinfo;
    if(root->children.size() == 0) {
    	symbol* sym = new symbol (root->filenr, 
    	    root->linenr, root->offset, block_count->back());
    	sym->attr.set(ATTR_struct, true);
    	sym->lexinfo = root->lexinfo;
    	bool insert = insert_struct(root->lexinfo, sym);
    	sym = lookup_struct(root->lexinfo);
		if(insert) {
    		root->attr.set(ATTR_struct, true);
    	}else if (!sym->fields) {
    		root->attr.set(ATTR_struct, true);
    	}else{
    		errprintf ("%:%s: %d: error: redefinition of type: '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->lexinfo->c_str());
		    exit(1);
    	}
    }else if (root->children.size() == 1) {
        if (lookup_struct(root->lexinfo)){
        	
        	symbol* sym = lookup_struct(root->lexinfo);
        	root->attr.set(ATTR_struct, true);
        	root->children[0]->attr.set(ATTR_struct, true);
        	root->children[0]->typenm = root->lexinfo;
        	root->children[0]->oftype = root->lexinfo;
        	sym->attr.set(ATTR_struct, true);
        	sym = lookup(root->children[0]->lexinfo);
        	sym->attr.set(ATTR_struct, true);
        	sym->lexinfo = root->lexinfo;

        	
        }else{
        	
        	symbol *sym = new symbol (root->filenr, 
    	    root->linenr, root->offset, block_count->back());
        	sym->attr.set(ATTR_struct, true);
        	root->attr.set(ATTR_struct, true);
        	root->children[0]->attr.set(ATTR_struct, true);
        	root->children[0]->typenm = root->lexinfo;
        	root->children[0]->oftype = root->lexinfo;
        	insert_struct(root->lexinfo, sym);
        	sym = lookup(root->children[0]->lexinfo);
        	sym->attr.set(ATTR_struct, true);
        	sym->lexinfo = root->lexinfo;
        }
    }
    
    return;
}


void field (astree* root) {
	
	if  (root == NULL) return;
	if  (root->symbol != TOK_FIELD) return;
	root->attr.set(ATTR_field, true);
	root->typenm = root->lexinfo;
	symbol* sym = new symbol (root->filenr, 
    	root->linenr, root->offset, block_count->back());
    if (!insert(root->lexinfo, sym)) {
    	errprintf ("%:%s: %d: error: redeclartion of field: '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->lexinfo->c_str());
		    exit(1);
    }
    
    return;
}

void ident (astree* root) {
	if  (root == NULL) return;
	symbol* sym = lookup(root->lexinfo);
	if (!sym) {
		errprintf ("%:%s: %d: error: undefined identifier: '%s'\n",
            included_filenames.back().c_str(),
            root->linenr, root->lexinfo->c_str());
		    exit(1);
	}if (sym->attr[ATTR_struct]) {
		root->typenm = sym->lexinfo;
		root->attr |= sym->attr;
	}else{
		root->attr |= sym->attr;
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

void voidcon (astree* root) {
	if  (root == NULL) return;
	if  ((root->symbol == TOK_VOID)) {
		root->attr.set(ATTR_void, true);
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
		case (TOK_BLOCK):
			foo = block;
			break;
		case (TOK_STRUCT):
			foo = struct_op;
			break;
		case (TOK_PROTOTYPE):
			foo = proto_op;
			break;
		case (TOK_CALL):
			foo = call_op;
			break;
		case (TOK_FUNCTION):
			foo = func_op;
			break;
		case (TOK_PARAMLIST):
			foo = paramlist;
			break;
		case (TOK_VARDECL):
			foo = vardecl;
			break;
		case (TOK_DECLID):
			foo = declid;
			break;
		case (TOK_IDENT):
			foo = ident;
			break;
		case (TOK_IFELSE):
			foo = ifelse_op;
			break;
		case (TOK_WHILE):
			foo = while_op;
			break;
		case (TOK_TYPEID):
			foo = type_id;
			break;
		case (TOK_FIELD):
			foo = field;
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
		case (TOK_VOID):
			foo = voidcon;
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

