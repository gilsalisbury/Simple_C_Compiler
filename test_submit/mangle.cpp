

#include "mangle.h"
#include "symboltable.h"
#include "lyutils.h"

using namespace std;


size_t reg = 1;

string mangle_while (astree* root) {
    string s = "while_" + to_string(root->filenr) + 
                "_" + to_string(root->linenr) + "_" + 
                to_string(root->offset);
    return s;
}

string mangle_if (astree* root) {
    string s = "if_" + to_string(root->filenr) + 
                "_" + to_string(root->linenr) + "_" + 
                to_string(root->offset);
    return s;
}

string mangle_func (astree* root) {
    return "__" + *root->lexinfo;
}


string mangle_typenm (astree* root) {
    string s;
    s = "s_" + *root->typenm;
    return s;
}

string mangle_local (astree* root) {
    string s;
    s = "_" + to_string(root->blocknr) + "_" + *root->lexinfo;
    return s;
}


string mangle_stmt (astree* root) {
    string s;
    s = *root->lexinfo + "_" + to_string(root->blocknr) + "_" +
        to_string(root->linenr) + "_" + to_string(root->offset);
    return s;
}


string mangle_global (astree* root) {
    //cout << "in mangle global" << endl;
    string s;
    if (root->symbol == TOK_WHILE) {
        s = mangle_while (root);
        return s;
    }
    if (root->symbol == TOK_IFELSE) {
        s = mangle_if (root);
        return s;
    }if (root->blocknr != 0) {
        //cout << "in mangle global calling mangle local" << endl;
        fflush(NULL);
        s = mangle_local (root);
        //cout << "exit mangle global by local" << endl;
        return s;
    }
    if (root->attr[ATTR_field]) {
        //cout << "have field of: " << (root->oftype == NULL) << endl;
        fflush(NULL);
        s = "f_" + *root->oftype + "_" + *root->lexinfo;
    }else if (root->attr[ATTR_struct]) {
        s = "s_" + *root->lexinfo;
    }else{
        s = "__" + *root->lexinfo;
    }
    //cout << "exit mangle global" << endl;
    return s;
}



string mangle_name (astree* root) {  
    string s = "";
    mangle_name(root);
    /*switch (root->blocknr) {
        case (0) :
            s = mangle_global (root);
            break;
        default: 
            break;
    }*/
    return s;
}
