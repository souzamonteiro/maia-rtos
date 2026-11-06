#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test-predicate-probe.hpp"
static void hex(const char* bytes, unsigned int length) {
    if (!length) { printf("-"); return; }
    for (unsigned int i=0;i<length;++i) printf("%02x",static_cast<unsigned char>(bytes[i]));
}
int main(int argc, char** argv) {
    unsigned int length=static_cast<unsigned int>(fread(gPredicateProbe,1,578,stdin));
    if (argc == 2) { printf("%d\n",predicate_slots(static_cast<unsigned int>(atoi(argv[1])))); return 0; }
    printf("%d\n",predicate_run(length));
    if (!predicate_accepted(length)) return 0;
    if (predicate_tree_guard(length) != 1) return 11;
    for(unsigned int i=0;i<predicate_tokens();++i){
        printf("%s\t%u\t%u\t",predicate_token_name(i),predicate_token_start(i),predicate_token_end(i));
        hex(predicate_token_value(i),predicate_token_length(i)); printf("\n");
    }
    for(unsigned int i=0;i<predicate_nodes();++i){
        unsigned int depth=0;for(int p=predicate_parent(i);p>=0;p=predicate_parent(p))++depth;
        printf("%u\t%s\t%s\t",depth,predicate_kind(i)==2?"T":"N",predicate_name(i));
        if(predicate_kind(i)==2)hex(predicate_value(i),predicate_length(i));else printf("-");
        printf("\n");
    }
    return 0;
}
