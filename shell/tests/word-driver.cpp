/* Native word-tree/renderer oracle. Raw bytes enter only the generated frontend. */
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <iterator>
#include <string>
#include "../generated/shell-word-frontend.hpp"
#include "../generated/maia-shell-parser.hpp"
#include "../maia-shell-parse-tree-collector.hpp"
static int textEquals(const char* a, const char* b) { return a && b && strcmp(a,b)==0; }
#include "../../../maiaglob/maia_glob.h"
#include "../shell-pattern.hpp"
#include "../shell-word.hpp"
#include "word-length-probe.hpp"
static void hex(const char* bytes, unsigned int length)
{
    const char* digits="0123456789abcdef";
    if(!length){std::cout<<'-';return;}
    for(unsigned int i=0;i<length;++i){unsigned char ch=static_cast<unsigned char>(bytes[i]);std::cout<<digits[ch>>4]<<digits[ch&15];}
}
static int hexNibble(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}
static int decodeProbeHex(const char* source, char* output, unsigned int maximum)
{
    const unsigned int size = static_cast<unsigned int>(strlen(source));
    if (size % 2 || size / 2 > maximum) return -1;
    for (unsigned int i = 0; i < size / 2; ++i) {
        const int a = hexNibble(source[i * 2]), b = hexNibble(source[i * 2 + 1]);
        if (a < 0 || b < 0) return -1;
        output[i] = static_cast<char>(a * 16 + b);
    }
    output[size / 2] = 0;
    return static_cast<int>(size / 2);
}
int main(int argc,char** argv)
{
    if(argc<3)return 10;
    if (argc > 4 && strcmp(argv[3], "--count-hex") == 0) {
        char value[97];
        const int size = decodeProbeHex(argv[4], value, 96);
        if (size < 0) return 12;
        char before[97]; memcpy(before, value, static_cast<unsigned int>(size) + 1);
        unsigned int count = 999;
        const int result = shellParameterLength(value, &count);
        const int guard = shellLengthDecodeGuard(value, static_cast<unsigned int>(size));
        if (memcmp(before, value, static_cast<unsigned int>(size) + 1)) return 13;
        std::cout << "COUNT\t" << result << '\t' << count << '\t' << guard << '\n';
        return 0;
    }
    int prefixBudget=argc>4 && strcmp(argv[3],"--prefix-budget")==0;
    int budgetMode=argc>4 && (strcmp(argv[3],"--budget")==0 || prefixBudget);
    int seedHex=argc>3 && strcmp(argv[3],"--seed-hex")==0;
    int assignment=argc>3 && strcmp(argv[3],"--assignment")==0;
    int fieldsMode=argc>3 && strcmp(argv[3],"--fields")==0;
    for(int i=budgetMode?5:(assignment || fieldsMode || seedHex)?4:3;i+1<argc;i+=2){
        char value[96];
        const char* seed = argv[i+1];
        if (seedHex) { if (decodeProbeHex(seed, value, 95) < 0) return 12; seed = value; }
        if(!validShellVariableName(argv[i]) || strlen(seed)>95 || !setShellVariable(argv[i],seed))return 11;
    }
    void* patternWorkspace = malloc(maia_glob_compile_workspace_size());
    if (!patternWorkspace) return 14;
    initShellPattern(patternWorkspace, maia_glob_compile_workspace_size());
    if (budgetMode) {
        const char* pattern = shellVariableValue("B", 1);
        unsigned char masks[96]; memset(masks, 0, sizeof(masks));
        unsigned int retained = 999, used = 999;
        int result = shellRemovalValue(shellVariableValue("NAME", 4), pattern,
            static_cast<unsigned int>(strlen(pattern)), masks, 0, prefixBudget,
            static_cast<unsigned int>(strtoul(argv[4], 0, 10)), &retained, &used);
        std::cout << "BUDGET\t" << result << '\t' << retained << '\t' << used << '\n';
        free(patternWorkspace); return 0;
    }
    unsigned int status=static_cast<unsigned int>(strtoul(argv[1],0,10));
    int expand=atoi(argv[2]);
    const std::string source((std::istreambuf_iterator<char>(std::cin)),std::istreambuf_iterator<char>());
    int parsed=parseShellWord(source.data(),static_cast<unsigned int>(source.size()));
    if(parsed!=1){std::cout<<"REJECT\n";free(patternWorkspace);return 0;}
    char output[96]; unsigned char masks[96];
    char name[32];
    const int rendered=assignment ? renderShellWordMode(source.data(),static_cast<unsigned int>(source.size()),status,expand,output,masks,96,1,name) : renderShellWord(source.data(),static_cast<unsigned int>(source.size()),status,expand,output,masks,96);
    if(fieldsMode){
        ShellWordFields fields;
        const int result=rendered==1 ? splitShellWord(output,static_cast<unsigned int>(strlen(output)),gShellWordSplit,gShellWordAnchors,&fields) : rendered;
        std::cout<<"FIELDS\t"<<result;
        if(result==1){
            std::cout<<'\t'<<fields.count;
            for(unsigned int i=0;i<fields.count;++i){
                std::cout<<'\t';hex(output+fields.start[i],fields.length[i]);std::cout<<':';
                hex(reinterpret_cast<const char*>(masks+fields.start[i]),fields.length[i]);
            }
        }
    }else{
        std::cout<<"RESULT\t"<<rendered;
        if(rendered==1){unsigned int n=static_cast<unsigned int>(strlen(output));std::cout<<'\t';hex(output,n);std::cout<<'\t';hex(reinterpret_cast<const char*>(masks),n);}
        if(assignment && rendered==1){std::cout<<'\t';hex(name,static_cast<unsigned int>(strlen(name)));}
    }
    std::cout<<'\n';
    unsigned int count=0;do{++count;}while(count<256&&gShellWordTokens[count-1].type!=kShellWordEof);
    std::cout<<"TOKENS\t"<<count<<'\n';
    for(unsigned int i=0;i<count;++i){
        const MaiaShellWord_MaiaCCParserToken* token=gShellWordTokens+i;
        std::cout<<MaiaShellWord_maiaCCParserTokenName(token->type)<<'\t'<<token->start<<'\t'<<token->end<<'\t';hex(token->value,token->length);std::cout<<'\n';
    }
    for(unsigned int i=0;i<gShellWordTree.nodeCount;++i){
        const MaiaShellParseTreeNode* node=gShellWordNodes+i;unsigned int depth=0;
        for(int parent=node->parent;parent>=0;parent=gShellWordNodes[parent].parent)++depth;
        const int terminal=node->kind==kShellParseTreeTerminal;
        std::cout<<depth<<'\t'<<(terminal?"T":"N")<<'\t'<<(terminal?MaiaShellWord_maiaCCParserTokenName(node->token):node->name)<<'\t';
        if(terminal)hex(node->value,node->length);else std::cout<<'-';std::cout<<'\n';
    }
    free(patternWorkspace);
    return 0;
}
