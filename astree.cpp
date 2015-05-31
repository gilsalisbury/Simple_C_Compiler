// dsalisbu
// 1224878


#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symboltable.h"
#include "astree.h"
#include "stringset.h"
#include "lyutils.h"

#define X 0


astree::astree (int symbol, int filenr, int linenr,
                int offset, const char* clexinfo):
        symbol (symbol), filenr (filenr), linenr (linenr),
        offset (offset), lexinfo (intern_stringset (clexinfo)) {
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           (void*) this, filenr, linenr, offset,
           get_yytname (symbol), lexinfo->c_str());
}


astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}


astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}


astree* adoptall (astree* root, astree* child) {
   auto itor = child->children.begin(); 
   for(; itor != child->children.end(); ++itor ){
      adopt1(root, *itor);
   }
   while(!child->children.empty()) child->children.pop_back();
   return root;
}


astree* makeheir (astree* root, astree* child ) {
   root->children.insert(root->children.begin(), child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}


astree* synthtok (int symbol, astree* donor) {
   astree* node = new astree(symbol, donor->filenr, donor->linenr,
                    donor->offset, "");
   adopt1(node, donor);
   return node;
}


static void tok_dump_node (FILE* outfile, astree* node) {
   fprintf (outfile, "%4d%4d.%03d %4d %-16s (%s)",
            int(node->filenr), int(node->linenr), int(node->offset),
            node->symbol, get_yytname(node->symbol), 
            node->lexinfo->c_str());
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
        ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
      fprintf (outfile, "%p", node->children.at(child));
   }
   fprintf (outfile, "\n");
}

static void tok_dump_astree_rec (FILE* outfile, astree* root,
                             int depth) {
   if (root == NULL) return;
   tok_dump_node (outfile, root);
   for (size_t child = 0; child < root->children.size();
        ++child) {
      tok_dump_astree_rec (outfile, root->children[child],
                       depth + 1);
   }
}


void tok_dump_astree (FILE* outfile, astree* root) {
   tok_dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}


static void dump_node (FILE* outfile, astree* node) {
   const char *tname = get_yytname (node->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;
   fprintf (outfile, "%s \"%s\" %ld:%ld.%03ld %6s",
            tname, node->lexinfo->c_str(),
            node->filenr, node->linenr, node->offset,
            strattr(node).c_str());
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
        ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
   }
}


static void error_dump_node (FILE* outfile, astree* node) {
   const char *tname = get_yytname (node->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;
   fprintf (outfile, "%s", node->lexinfo->c_str());
   
}


static void dump_astree_rec (FILE* outfile, astree* root,
                             int depth) {
   if (root == NULL) return;
   for(int i=0; i<depth; i++) {
      fprintf(outfile, "|  ");
   }
   dump_node (outfile, root);
   fprintf (outfile, "\n");
   for (size_t child = 0; child < root->children.size();
        ++child) {
      dump_astree_rec (outfile, root->children[child],
                       depth + 1);
   }
}


static void postorder ( astree* root,
                             int depth) {
   if (root == NULL) return;
   if (root->symbol == TOK_BLOCK) enter_block();
   if (root->symbol == TOK_STRUCT) enter_struct();
   if (root->symbol == TOK_FUNCTION) enter_func(root);
   if (root->symbol == TOK_PROTOTYPE) enter_block();
   
   for (size_t child = 0; child < root->children.size();
        ++child) {
      postorder (root->children[child],
                       depth + 1);
   }

   func foo = getfunc(root->symbol);
   foo(root);
   if (X) {
      for(int i=0; i<depth; i++) {
         fprintf(stdout, "|  ");
      }
      dump_node (stdout, root);
      cout << endl;
   }
   
}



static void preorder (FILE* outfile, astree* root,
                             int depth) {
   if (root == NULL) return;
   for(int i=0; i<depth; i++) {
      fprintf(outfile, "|  ");
   }

   dump_node (outfile, root);
   fprintf (outfile, "\n");
   for (size_t child = 0; child < root->children.size();
        ++child) {
      preorder (outfile, root->children[child],
                       depth + 1);
   }
}



static bool sym_dump_node (FILE* outfile, astree* node) {
   const char *tname = get_yytname (node->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;
   
   fprintf (outfile, "%s (%ld:%ld.%0ld) {%d} %s",
            node->lexinfo->c_str(),
            node->filenr, node->linenr, node->offset,
            node->blocknr, strattr(node).c_str());
   bool need_space = false;
   for (size_t child = 0; child < node->children.size();
        ++child) {
      if (need_space) fprintf (outfile, " ");
      need_space = true;
   }
   fprintf (outfile, "\n");
   return true;
}




static void sym_preorder (FILE* outfile, astree* node, int depth) {
  if (node == NULL) return;
    
  if ((node->symbol == TOK_STRUCT) | (node->symbol == TOK_VARDECL) |
      (node->symbol == TOK_PROTOTYPE)|(node->symbol == TOK_FUNCTION)) {



      //cout << endl;
      if (node->symbol == TOK_STRUCT) {
          for(int i=0; i<depth; i++) {
              fprintf(outfile, "  ");
          }
          sym_dump_node(outfile, node->children[0]);
          for (size_t child = 1; child < node->children.size();
              ++child) {
              fprintf(outfile, "  ");
              sym_dump_node (outfile, 
                node->children[child]->children[0]);
          }
          if (depth == 0)  fprintf(outfile, "\n");
      }else if ((node->symbol == TOK_FUNCTION) | 
                (node->symbol == TOK_PROTOTYPE)) {
          for(int i=0; i<depth; i++) {
              fprintf(outfile, "  ");
          }
          if (node->children[0]->symbol == TOK_ARRAY) 
              sym_dump_node(outfile, node->children[0]->children[1]);
          else sym_dump_node(outfile, node->children[0]->children[0]);
          for (size_t child = 0; child < 
                         node->children[1]->children.size(); ++child) {
              astree* param = node->children[1]->children[child];
              if (param->attr[ATTR_array]) param = param->children[1];
              else param = param->children[0];
              fprintf(outfile, "  ");
              sym_dump_node (outfile, param);
          }
          --depth;
          if (depth == 0)  fprintf(outfile, "\n");
      }else if (node->symbol == TOK_VARDECL) {
          for(int i=0; i<depth; i++) {
              fprintf(outfile, "  ");
          }
          if (depth == 0)  fprintf(outfile, "\n");
          if (node->children[0]->symbol == TOK_ARRAY) 
              sym_dump_node(outfile, node->children[0]->children[1]);
          else sym_dump_node(outfile, node->children[0]->children[0]);
      }

  }
   for (size_t child = 0; child < node->children.size();
        ++child) {
      sym_preorder (outfile, node->children[child], depth+1);
   }
}


void dump_error (FILE* outfile, astree* root,
                             int depth) {
   if (root == NULL) return;
   error_dump_node (stdout, root);
   fprintf (outfile, " ");
   for (size_t child = 0; child < root->children.size();
        ++child) {
      preorder (outfile, root->children[child],
                       depth + 1);
   }
}


void dump_astree (FILE* outfile, astree* root) {
   dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}


void type_ast (FILE* outfile, astree* root) {
   init_symbol_table();
   if (X) {
      cout << "\n\n\n\n";
      preorder(outfile, root, 0);
      cout << "\n\n\n\n";
   }
   postorder (root, 0);
   if (X) {
      cout << "\n\n\n\n";
      preorder(outfile, root, 0);
   }
   sym_preorder (outfile, root, -1);
   fflush (NULL);
}



void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep) {
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      fprintf (outfile, "%s(%d)\n",
               get_yytname (toknum), toknum);
   }
   fflush (NULL);
}


void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%p]-> %d:%d.%d: %s: \"%s\")\n",
           root, root->filenr, root->linenr, root->offset,
           get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}


void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}


RCSC("$Id: astree.cpp,v 1.1 2015-05-04 13:36:08-07 - - $")

