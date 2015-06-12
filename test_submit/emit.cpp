
#include <stdio.h>
#include <assert.h>

#include "emit.h"
#include "lyutils.h"
#include "auxlib.h"
#include "mangle.h"
#include "symboltable.h"

using string_stack = vector<astree*>;

int address = 1;

string_stack* strings =  new string_stack();
string* struct_prefix = new string();
string* strcon_prefix = new string();
string* var_prefix = new string();
string* func_prefix = new string();
string* main_string = new string();
string* current_string;

void emit (astree*);



void prefix_var (astree* node) {
   if (node == NULL) return;
   *var_prefix += mangle_global(node) + "\n";
}



string oc_oil (astree* root) {
   int id = idtype(root->attr);
   string s;
   if (id == ATTR_int) {
      s = "int";
   }else if (id == ATTR_struct) {
      s += "struct " + mangle_typenm(root);
      s += "*";
   }else{
      if (root->attr[ATTR_string]) {
         s += "char*";
      }else{
         s += "char";
      }
   }
   if (root->attr[ATTR_array]) {
      s += "*";
   }
   if (root->attr[ATTR_field]) {
      s += "*";
   }

   //if (root->blocknr) s += mangle_local (root);
   s += " " + mangle_global (root);
   return s;
}


string alloc_reg (astree* root) {
   int id = idtype(root->attr);
   string s;
   if (id == ATTR_int) {
      s = "i" + to_string(address);
   }else if (id == ATTR_struct) {
      s = "p" + to_string(address);
   }else {  // had if (id == ATTR_string)
      s = "c" + to_string(address);
   }
   if (root->attr[ATTR_field]) {
      s = "a" + to_string(address);
   }
   ++address;
   return s;
}



string base (astree* root) {
   int id = idtype(root->attr);
   string s;
   if (id == ATTR_int) {
      s = "int";
   }else if (id == ATTR_struct) {
      s = "struct " + mangle_typenm(root);
   }else {  // had if (id == ATTR_string)
      s = "char" ;
      if (id == ATTR_string) s+= "*";
   }
  
   ++address;
   return s;
}



string type_reg (astree* root) {
   int id = idtype(root->attr);
   string s;
   if (id == ATTR_int) {
      s = "int";
   }else if (id == ATTR_struct) {
      s = "struct " + mangle_typenm(root) + "*";
   }else {  // had if (id == ATTR_string)
      s = "char" ;
      if (id == ATTR_string) s+= "*";
   }
   
   if (root->attr[ATTR_array]) {
      s+= "*";
   }
   s+= " ";
   ++address;
   return s;
}



void prefix_vars (astree* root) {
   *var_prefix += oc_oil (root) + ";\n";
}



void prefix_struct (astree* root) {
   *struct_prefix += "struct ";
   *struct_prefix += mangle_global(root->children[0]);
   *struct_prefix += " {\n";
   for (size_t i = 1; i < root->children.size(); ++i) {
      *struct_prefix += "        ";
      if (root->children.at(i)->attr[ATTR_array]) {
         *struct_prefix += oc_oil (root->children.at(i)->children[1]);
      }else {
         *struct_prefix += oc_oil (root->children.at(i)->children[0]);
      }
      *struct_prefix += ";\n";
   }
    *struct_prefix += "};\n";
}



void prefix_strcon (astree* root) {
   root->reg = alloc_reg(root);
   *strcon_prefix += "char* " + root->reg;
   *strcon_prefix += " = " + *root->lexinfo + ";\n";
}



void postorder (astree* root) {
   assert (root != NULL);
  
   for (size_t child = 0; child < root->children.size(); ++child) {
      
      if ((root->children.at(child)->symbol == TOK_WHILE) |
          (root->children.at(child)->symbol == TOK_IFELSE)|
          (root->children.at(child)->symbol == TOK_CALL)) {
         emit (root->children.at(child));
      }else
      postorder (root->children.at(child));
   }
   emit (root);
}




void postorder_stmts (astree* root) {
   assert (root != NULL);
   current_string = main_string;
   if ((root->symbol == TOK_FUNCTION) |
          (root->symbol == TOK_STRUCT)){
      return;
   }
   if (root->symbol == TOK_IFELSE) {
      emit (root);
      return;
   }
   for (size_t child = 0; child < root->children.size(); ++child) {
      if ((root->children.at(child)->symbol == TOK_WHILE) |
          (root->children.at(child)->symbol == TOK_IFELSE)|
          (root->children.at(child)->symbol == TOK_CALL)
          ) {
          emit (root->children.at(child));
      }else
      postorder_stmts (root->children.at(child));
   }
   if (root->symbol != TOK_ELSE) emit (root);
}


void preorder (astree* root) {
   current_string = main_string;
   if (root == NULL) return;
   if (root->symbol == TOK_FUNCTION) {
      if (root->children[0]->symbol == TOK_ARRAY) { 
         prefix_func (root, root->children[0]->children[1]);
      }else {
         prefix_func (root, root->children[0]->children[0]);
      }
   }else if (root->symbol == TOK_STRUCT) {
      prefix_struct (root);
   }
   for (size_t child = 0; child < root->children.size(); ++child) {
      preorder (root->children.at(child));
   }
}



void prefix_func (astree* root, astree* name) {
   current_string = func_prefix;
   *current_string += oc_oil (name) + " (";
   vector<astree*> paramlist = root->children[1]->children;
   for (size_t i = 0; i < paramlist.size(); ++i) {
      *current_string += "\n        ";
      if (paramlist[i]->attr[ATTR_array]) {
         *current_string += oc_oil (paramlist[i]->children[1]);
      }else{
         *current_string += oc_oil (paramlist[i]->children[0]);
      }
      if (i != paramlist.size() -1) *current_string += ",\n";
   }
  
   *current_string += ")\n{\n";
   postorder(root->children[2]);
   *current_string += "};\n";
   current_string = main_string;

   
}



void emit_field_select (astree* root) {
   root->reg = alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg (root) + " ";
   *current_string += root->reg + " = &";
   //oot->reg = "*" + root->reg;
   if (root->children[0]->reg != "") {
      *current_string += root->children[0]->reg;
   }else{
      *current_string += mangle_global(root->children[0]);
   }
   *current_string += "->";
   if (root->children[1]->reg != "") {
      *current_string += root->children[1]->reg;
   }else{
      *current_string += mangle_global(root->children[1]);
   }
   *current_string += ";\n";
}

void emit_return (astree* root) {
   if (!root->children.size()) {
      *current_string += "\nreturn;\n";
   }else{
      *current_string += "return ";
      if (root->children[0]->reg != "") {
         *current_string += root->children[0]->reg;
      }else if (root->children[0]->attr[ATTR_const]) {
         *current_string += *root->children[0]->lexinfo;
      }else{
         *current_string += mangle_global(root->children[0]);
      }
      *current_string += ";\n";
   }
}


void emit_unop (string operand, astree* root) {
   root->reg = alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg (root) + " ";
   *current_string += root->reg + " = " + operand;
   if (root->children[0]->reg != "") {
      *current_string += root->children[0]->reg + ";\n";
   }else{
      *current_string += *root->children[0]->lexinfo + ";\n";
   }
}


void emit_index_select (astree* root) {
   root->reg = alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg (root) + " ";
   *current_string += root->reg + " = &";
   //root->reg = "*" + root->reg;
   if (root->children[0]->reg != "") {
      *current_string += root->children[0]->reg;
   }else{
      *current_string += mangle_global(root->children[0]);
   }
   *current_string += "[";
   if (root->children[1]->reg != "") {
      *current_string += root->children[1]->reg;
   }else if (root->children[1]->attr[ATTR_const]) {
      *current_string += *root->children[1]->lexinfo;
   }else{
      *current_string += mangle_global(root->children[1]);
   }
   *current_string += "];\n";
}

void emit_binop (string opcode, astree* root) {

   root->reg = alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg (root) + " ";
   *current_string += root->reg + " = ";
   
   if (root->children[0]->reg != "") {
      *current_string += root->children[0]->reg;
      
   }else if (root->children[0]->attr[ATTR_null]) {
      *current_string += "0";
   }else if (!root->children[0]->attr[ATTR_const]) {
      *current_string += mangle_global(root->children[0]);
      
   }else{
      
      *current_string += *root->children[0]->lexinfo;
   }
   *current_string += " " + opcode + " ";
   if (root->children[1]->reg != "") {
      *current_string += root->children[1]->reg;
   }else if (root->children[1]->attr[ATTR_null]) {
      *current_string += "0";
   }else if (!root->children[1]->attr[ATTR_const]) {
      *current_string += mangle_global(root->children[1]);
   }else{
      *current_string += *root->children[1]->lexinfo;
   }
   *current_string += ";\n";
}


void emit_call (astree* root) {
   astree* name;
   for (size_t i = 1; i < root->children.size(); ++ i) {
      emit (root->children.at(i));
   }
   name = root->children[0];
   root->reg = alloc_reg (name);
   *current_string += "        ";
   if (!root->attr[ATTR_void]) {

      *current_string += type_reg  (name) + " ";
      *current_string += root->reg + " = ";
   }
   *current_string += mangle_func (name) + "(";
   for (size_t i = 1; i < root->children.size(); ++ i) {
      if (root->children[i]->reg != "") {
         *current_string += root->children[i]->reg;
      }else if (!root->children[i]->attr[ATTR_const]) {
         *current_string += mangle_global(root->children[i]);
      }else{
         *current_string += *root->children[i]->lexinfo;
      }
      if (i != root->children.size() - 1) *current_string += ", ";
   }
    *current_string += ");\n";
}

void emit_alloc_struct (astree* root) {
   root->reg =  alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg  (root) + " ";
   *current_string += root->reg + " = ";
   *current_string += "xcalloc (1, sizeof(struct " + 
            mangle_typenm(root) + "));\n";
}

void emit_alloc_string (astree* root) {
   root->reg =  alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg  (root) + " ";
   *current_string += root->reg + " = ";
   *current_string += "xcalloc (";
   if (root->children[0]->reg != "") {
      *current_string += root->children[0]->reg;
   }else if (root->children[0]->attr[ATTR_variable]) {
      *current_string += mangle_global (root->children[0]);
   }else{
      *current_string += *root->children[0]->lexinfo;
   }
   *current_string += ", sizeof(char));\n";
}



void emit_alloc_array (astree* root) {
   //postorder (root->children[0]);
   root->reg =  alloc_reg (root);
   *current_string += "        ";
   *current_string += type_reg  (root) + " ";
   *current_string += root->reg + " = ";
   *current_string += "xcalloc (";
   root->reg = root->reg;
   if (root->children[1]->reg != "") {
      *current_string += root->children[1]->reg;
   }else if (root->children[1]->attr[ATTR_variable]) {
    
      *current_string += mangle_global (root->children[0]);
   }else{
      *current_string += *root->children[1]->lexinfo;
   }
   if (root->attr[ATTR_struct]) 
      *current_string += ", sizeof(struct "+type_reg  (root) + "));\n";
   else *current_string += ", sizeof(" + base  (root) + "));\n";
}



void emit_vardecl (astree* root) {
   if (root==NULL) return;
   astree* name;
   if (root->children[0]->attr[ATTR_array]) 
      name = root->children[0]->children[1];
   else name = root->children[0]->children[0];
   if (root->children[1]->attr[ATTR_struct] &&
      root->children[1]->children.size()) {
      if (root->children[1]->children[0]->symbol == TOK_NEW) {
         emit_alloc_struct(root->children[1]);
      }
   }
   *current_string+= "        ";
   *current_string += oc_oil (name) + " = ";
   if (root->children[1]->reg != "") {
      *current_string += root->children[1]->reg;
   }else{
      *current_string += *root->children[1]->lexinfo;
   }
   *current_string += ";\n";
}


void emit_assign (astree* root) {
   astree* name;
   *current_string+= "        ";
   if (root->children[0]->attr[ATTR_array]) 
      name = root->children[0]->children[1];
   else if (root->children[0]->symbol == '.') {
      name = root->children[0]->children[0];
   } else name = root->children[0];
   *current_string += " " + mangle_global (name) + " = ";
   if (root->children[1]->reg != "") {
      *current_string += root->children[1]->reg;
   }else{
      *current_string += *root->children[1]->lexinfo;
   }
   *current_string += ";\n";
}


void emit_while (astree* root) {

   *current_string += mangle_global (root) + ":;\n";
   postorder (root->children[0]);
   *current_string += "        ";
   if (root->children[0]->attr[ATTR_const]) {
      root->children[0]->reg = alloc_reg (root->children[0]);
      *current_string += type_reg(root->children[0]) + " " +
                           root->children[0]->reg + " = ";
      *current_string += *root->children[0]->lexinfo;
      *current_string += ";\n        ";
   }
   // do not print if operand 
   *current_string += "if(!" + root->children[0]->reg + ")";
   *current_string += " goto break_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ";\n";
   postorder (root->children[1]);
   *current_string += "        goto while_" + to_string(root->filenr)+
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ";\n";
   *current_string += "break_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ":;\n";

}


void emit_if (astree* root) {
   postorder (root->children[0]);
   //*current_string += mangle_global (root) + ":;\n";
   if (root->children[0]->attr[ATTR_const]) {
      root->children[0]->reg = alloc_reg (root->children[0]);
      *current_string += type_reg(root->children[0]) + " " +
                           root->children[0]->reg + " = ";
      *current_string += *root->children[0]->lexinfo;
      *current_string += ";\n        ";
   }
   // do not print if operand 
   *current_string += "        ";
   *current_string += "if(!" + root->children[0]->reg + ")";
   *current_string += " goto fi_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ";\n";
   postorder (root->children[1]);
   *current_string += "fi_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ":;\n";

}


void emit_ifelse (astree* root) {
   
   if (root->children.size() == 2) {
      emit_if (root);
      return;
   }
   postorder (root->children[0]);
   if (root->children[0]->attr[ATTR_const]) {
      root->children[0]->reg = alloc_reg (root->children[0]);
      *current_string += type_reg(root->children[0]) + " " +
                           root->children[0]->reg + " = ";
      *current_string += *root->children[0]->lexinfo;
      *current_string += ";\n        ";
   }
   // do not print if operand 
   *current_string += "if(!" + root->children[0]->reg + ")";
   *current_string += " goto else_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ";\n";
   postorder (root->children[1]);
   *current_string += "        goto fi_" + to_string(root->filenr)+ 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ";\n";
   *current_string += "else_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ":;\n";
   postorder (root->children[2]);
   *current_string += "fi_" + to_string(root->filenr) + 
                     "_" + to_string(root->linenr) + "_" + 
                     to_string(root->offset) + ":;\n";
}


void emit_prefix (FILE* outfile) {
   fprintf(outfile, struct_prefix->c_str());
   fprintf(outfile, strcon_prefix->c_str());
   fprintf(outfile, var_prefix->c_str());
   fprintf(outfile, func_prefix->c_str());
}



void emit_main (astree* root, FILE* outfile) {
   *current_string += "void __ocmain (void) {\n";
   postorder_stmts(root);
   *current_string += "return;\n};\n";
   fprintf(outfile, main_string->c_str());
   
}


void emit (astree* root) {

   switch (root->symbol) {

      case '='   :         emit_assign (root);                 break;
      case TOK_VARDECL :   emit_vardecl (root);                break;
      case TOK_NEWSTRING:  emit_alloc_string (root);           break;
      case TOK_NEWARRAY:   emit_alloc_array (root);            break;
      case '+':            emit_binop ("+", root);             break;
      case '-':            emit_binop ("-", root);             break;
      case '*':            emit_binop ("*", root);             break;
      case '/':            emit_binop ("/", root);             break;
      case '.':            emit_field_select (root);           break;
      case '!':            emit_unop ("!", root);              break;
      case TOK_LT:         emit_binop ("<", root);             break;
      case TOK_GT:         emit_binop (">", root);             break;
      case TOK_LE:         emit_binop ("<=", root);            break;
      case TOK_GE:         emit_binop (">=", root);            break;
      case TOK_EQ:         emit_binop ("==", root);            break;
      case TOK_NE:         emit_binop ("!=", root);            break;
      case TOK_NEG:        emit_unop ("-", root);              break;
      case TOK_POS:        emit_unop ("+", root);              break;
      case TOK_INDEX:      emit_index_select (root);           break;
      case TOK_RETURN:      emit_return (root);                break;
      case TOK_CALL:       emit_call (root);                   break;
      case TOK_WHILE:      emit_while (root);                  break;
      case TOK_IFELSE:     emit_ifelse (root);                 break;
      default:                                                 break;
   }
}


void emit_ta_code (FILE* outfile, astree* root) {
   if (root == NULL) return;
   preorder(root);
   emit_prefix(outfile);
   emit_main(root, outfile);
}

RCSC("$Id: emit.cc,v 1.3 2013-09-20 17:52:13-07 - - $")
