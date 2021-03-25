
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>

static int GenerateSrg() {
    static int srgId = 1;
    return srgId++;
}

/*
 * Walk the Node tree, and dump the AST tree to stdout.
 */
void DumpTree(struct ASTNode* Node, int level) {
    int Lfalse, Lstart, Lend;

    // Handle weirdo loops and conditions first.
    switch(Node->Operation) {
        case OP_IF:
            Lfalse = GenerateSrg();
            for(int i = 0; i < level; i++)
                fprintf(stdout, " ");
            fprintf(stdout, "IF");
            if(Node->Right) { 
                Lend = GenerateSrg();
                fprintf(stdout, ", end label %d", Lend);
            }

            fprintf(stdout, "\n");
            DumpTree(Node->Left, level + 2);
            DumpTree(Node->Middle, level + 2);
            
            if(Node->Right) 
                DumpTree(Node->Right, level + 2);
            
            return;
        case OP_LOOP:
            Lstart = GenerateSrg();
            for(int i = 0; i < level; i++)
                fprintf(stdout, " ");
            fprintf(stdout, "LOOP starts at %d\n", Lstart);
            Lend = GenerateSrg();
            DumpTree(Node->Left, level + 2);
            DumpTree(Node->Right, level + 2);
            return;
    }

    // If current node is a compound, we treat it as if we didn't just enter a loop.
    if(Node->Operation == OP_COMP)
        level = -2;
    
    if(Node->Left)
        DumpTree(Node->Left, level + 2);
    
    if(Node->Right)
        DumpTree(Node->Right, level + 2);
    
    // The meat of this operation!
    for(int i = 0; i < level; i++)
        fprintf(stdout, " ");
    
    switch (Node->Operation){
        case OP_COMP: fprintf(stdout, "\n\n"); return;
        case OP_FUNC: fprintf(stdout, "OP_FUNC %s\n", Node->Symbol->Name); return;
        case OP_ADD:  fprintf(stdout, "OP_ADD\n"); return;
        case OP_SUBTRACT:  fprintf(stdout, "OP_SUBTRACT\n"); return;
        case OP_MULTIPLY:  fprintf(stdout, "OP_MULTIPLY\n"); return;
        case OP_DIVIDE:  fprintf(stdout, "OP_DIVIDE\n"); return;
        case OP_EQUAL:  fprintf(stdout, "OP_EQUAL\n"); return;
        case OP_INEQ:  fprintf(stdout, "OP_INEQ\n"); return;
        case OP_LESS:  fprintf(stdout, "OP_LESS\n"); return;
        case OP_GREAT:  fprintf(stdout, "OP_GREAT\n"); return;
        case OP_LESSE:  fprintf(stdout, "OP_LESSE\n"); return;
        case OP_GREATE:  fprintf(stdout, "OP_GREATE\n"); return;
        case TERM_INTLITERAL:  fprintf(stdout, "TERM_INTLITERAL %d\n", Node->IntValue); return;
        case TERM_STRLITERAL:  fprintf(stdout, "TERM_STRLITERAL rval L%d\n", Node->IntValue); return;
        case REF_IDENT:  
            if(Node->RVal)
             fprintf(stdout, "REF_IDENT rval %s\n", Node->Symbol->Name);
            else 
             fprintf(stdout, "REF_IDENT %s\n", Node->Symbol->Name);
            return;
        case OP_ASSIGN:  fprintf(stdout, "OP_ASSIGN\n"); return;
        case OP_WIDEN:  fprintf(stdout, "OP_WIDEN\n"); return;
        case OP_RET:  fprintf(stdout, "OP_RET\n"); return;
        case OP_CALL:  fprintf(stdout, "OP_CALL %s\n", Node->Symbol->Name); return;
        case OP_ADDRESS:  fprintf(stdout, "OP_ADDRESS %s\n", Node->Symbol->Name); return;
        case OP_DEREF:  
            fprintf(stdout, "OP_DEREF %s\n", Node->RVal ? "rval" : ""); return;
        case OP_SCALE:  fprintf(stdout, "OP_SCALE %s\n", TypeNames(Node->Size)); return;

        case OP_BOOLOR: fprintf(stdout, "OP_BOOLOR\n"); return;
        case OP_BOOLAND: fprintf(stdout, "OP_BOOLAND\n"); return;
        case OP_BITOR: fprintf(stdout, "OP_BITOR\n"); return;
        case OP_BITXOR: fprintf(stdout, "OP_BITXOR\n"); return;
        case OP_BITAND: fprintf(stdout, "OP_BITAND\n"); return;

        case OP_SHIFTL: fprintf(stdout, "OP_SHIFTL\n"); return;
        case OP_SHIFTR: fprintf(stdout, "OP_SHIFTR\n"); return;

        case OP_PREINC: fprintf(stdout, "OP_PREINC\n"); return;
        case OP_PREDEC: fprintf(stdout, "OP_PREDEC\n"); return;
        case OP_POSTINC: fprintf(stdout, "OP_POSTINC\n"); return;
        case OP_POSTDEC: fprintf(stdout, "OP_POSTDEC\n"); return;

        case OP_BITNOT: fprintf(stdout, "OP_BITNOT\n"); return;
        case OP_BOOLNOT: fprintf(stdout, "OP_BOOLNOT\n"); return;
        case OP_NEGATE: fprintf(stdout, "OP_NEGATE\n"); return;

        case OP_BOOLCONV: fprintf(stdout, "OP_BOOLCONV\n"); return;

        default:
            DieDecimal("Unknown Dump Operator", Node->Operation);
    }

}