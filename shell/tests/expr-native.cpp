#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expr-probe.hpp"
static void hex(const char* bytes, unsigned int length) {
    if (!length) { printf("-"); return; }
    for (unsigned int i=0;i<length;++i) printf("%02x",static_cast<unsigned char>(bytes[i]));
}
int main(int argc, char** argv) {
    const unsigned int length = static_cast<unsigned int>(fread(gExprProbeInput,1,1201,stdin));
    int status;
    if (argc > 2 && strcmp(argv[1],"slots")==0) status=expr_slots(static_cast<unsigned int>(atoi(argv[2])));
    else if (argc > 2 && strcmp(argv[1],"budget")==0) status=expr_budget(length,static_cast<unsigned int>(atoi(argv[2])));
    else if (argc > 1 && strcmp(argv[1],"guard")==0) { printf("%d\n",expr_tree_guard(length));return 0; }
    else status=expr_run(length);
    printf("%d\t",status);hex(expr_result(),static_cast<unsigned int>(strlen(expr_result())));printf("\n");
    if (argc < 2 || strcmp(argv[1],"trace")!=0 || !expr_accepted()) return 0;
    for(unsigned int i=0;i<expr_token_count();++i){printf("%s\t%u\t%u\t",expr_token_name(i),expr_token_start(i),expr_token_end(i));hex(expr_token_value(i),expr_token_length(i));printf("\n");}
    for(unsigned int i=0;i<expr_node_count();++i){unsigned int depth=0;for(int p=expr_node_parent(i);p>=0;p=expr_node_parent(static_cast<unsigned int>(p)))++depth;
        printf("%u\t%s\t%s\t",depth,expr_node_kind(i)==2?"T":"N",expr_node_name(i));
        if(expr_node_kind(i)==2)hex(expr_node_value(i),expr_node_length(i));else printf("-");printf("\n");}
    return 0;
}
