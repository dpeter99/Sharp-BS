
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>
#include <stdbool.h>

/*
 * Handles reading in a comma-separated list of declarations.
 * Erythro treats structs, enums and function parameters the same in this regard -
 *  comma separated.
 * 
 * C and C++ tend to treat enums and structs differently - the former separated by commas,
 *  the latter separated by semicolons.
 * 
 * Note that since functions are read in through parentheses, and structs/enums are read in
 *  through brackets, the end character is configurable.
 * 
 * @param FunctionSymbol: The Symbol Table Entry of the current function, if applicable.
 * @param Storage: The Storage Scope of this declaration list.
 * @param End: The end token, in terms of TokenTypes enum values.
 * @return the amount of declarations read in.
 * 
 */

static int ReadDeclarationList(struct SymbolTableEntry* FunctionSymbol, int Storage, int End) {
    int TokenType, ParamCount = 0;
    struct SymbolTableEntry* PrototypePointer = NULL, *Composite;

    if(FunctionSymbol != NULL)
        PrototypePointer = FunctionSymbol->Start;

    while(CurrentToken.type != End) {
        TokenType = ParseOptionalPointer(FunctionSymbol);
        VerifyToken(TY_IDENTIFIER, "identifier");

        printf("\tReading a new element: %s of type %d\n", CurrentIdentifier, TokenType);

        if(PrototypePointer != NULL) {
            if(TokenType != PrototypePointer->Type)
                DieDecimal("Function parameter of invalid type at index", ParamCount + 1);
            PrototypePointer=PrototypePointer->NextSymbol;
        } else {
            BeginVariableDeclaration(TokenType, Composite, Storage);
        }
        ParamCount++;

        if((CurrentToken.type != LI_COM) && (CurrentToken.type != End)) 
            DieDecimal("Unexpected token in parameter", CurrentToken.type);
        
        if(CurrentToken.type == LI_COM)
            Tokenise();
    }

    if((FunctionSymbol != NULL) && (ParamCount != FunctionSymbol->Length))
        DieMessage("Invalid number of parameters in prototyped function", FunctionSymbol->Name);

    return ParamCount;
}

/*
 * Handles the declaration of a new struct.
 *  struct thisStct { int x, int y, int z };
 * 
 * Verifies that the current identifier is not used,
 *  verifies that this is not a redefinition (excluding
 *   the case where there is a declaration but no definition)
 *  and then saves it into the Structs symbol table.
 * 
 * @return the Symbol Table entry of this new struct.
 */

struct SymbolTableEntry* BeginStructDeclaration() {
    struct SymbolTableEntry* Composite = NULL, *Member;
    int Offset;

    Tokenise();

    if(CurrentToken.type == TY_IDENTIFIER) {
        Composite = FindStruct(CurrentIdentifier);
        Tokenise();
    }

    if(CurrentToken.type != LI_LBRAC) {
        if(Composite == NULL) 
            DieMessage("Unknown Struct", CurrentIdentifier);
        return Composite;
    }

    if(Composite)
        DieMessage("Redefinition of struct", CurrentIdentifier);

    Composite = AddSymbol(CurrentIdentifier, DAT_STRUCT, 0, SC_STRUCT, 0, 0, NULL);
    Tokenise();
    printf("Reading a struct declaration..\n");
    ReadDeclarationList(NULL, SC_MEMBER, LI_RBRAC);
    VerifyToken(LI_RBRAC, "}");

    Composite->Start = StructMembers;
    StructMembers = StructMembersEnd = NULL;

    Member = Composite->Start;
    Member->SinkOffset = 0;
    Offset = TypeSize(Member->Type, Member->CompositeType);

    for(Member = Member->NextSymbol; Member != NULL; Member = Member->NextSymbol) {
        Member->SinkOffset = AsAlignMemory(Member->Type, Offset, 1);

        Offset += TypeSize(Member->Type, Member->CompositeType);
    }

    Composite->Length = Offset;
    return Composite;
}

/*
 * Handles the declaration of a type of a variable.
 *  int newVar;
 * 
 * It verifies that we have a type keyword followed by a
 *  unique, non-keyword identifier.
 * 
 * It then stores this variable into the appropriate symbol table,
 *  and returns the new item.
 * 
 * @return the Symbol Table entry of this new variable.
 */
struct SymbolTableEntry* BeginVariableDeclaration(int Type, struct SymbolTableEntry* Composite, int Scope) {
    struct SymbolTableEntry* Symbol = NULL;

    switch(Scope) {
        case SC_GLOBAL:
            if(FindGlobal(CurrentIdentifier) != NULL)
                DieMessage("Invalid redeclaration of global variable", CurrentIdentifier);
        case SC_LOCAL:
        case SC_PARAM:
            if(FindLocal(CurrentIdentifier) != NULL)
                DieMessage("Invalid redeclaration of local variable", CurrentIdentifier);
        case SC_MEMBER:
            if(FindMember(CurrentIdentifier) != NULL)
                DieMessage("Invalid redeclaration of Enum/Struct member", CurrentIdentifier);
    }

    if(CurrentToken.type == LI_LBRAS) {
        Tokenise();

        if(CurrentToken.type == LI_INT) {
            switch(Scope) {
                case SC_GLOBAL:
                    Symbol = AddSymbol(CurrentIdentifier, PointerTo(Type), ST_ARR, Scope, 1, 0, NULL);
                    break;
                case SC_LOCAL:
                case SC_PARAM:
                case SC_MEMBER:
                    Die("Local arrays are unimplemented");
            }
        }

        Tokenise();
        VerifyToken(LI_RBRAS, "]");
    } else {
        Symbol = AddSymbol(CurrentIdentifier, Type, ST_VAR, Scope, 1, 0, Composite);
    }

    return Symbol;

}

/*
 * Handles the declaration of a new function.
 *  Verifies that the identifier is not taken (excluding the case
 *   where there is a declaration but no definition)
 *  Parses the list of parameters if present
 *  Saves the function prototype if there is no body
 *  Generates and saves the break-out point label
 * 
 * @param Type: The return type of the function
 * @return the AST for this function
 * 
 */

struct ASTNode* ParseFunction(int Type) {
    struct ASTNode* Tree;
    struct ASTNode* FinalStatement;
    struct SymbolTableEntry* OldFunction, *NewFunction = NULL;
    int SymbolSlot, BreakLabel, ParamCount, ID;

    if((OldFunction = FindSymbol(CurrentIdentifier)) != NULL)
        if(OldFunction->Storage != ST_FUNC)
            OldFunction = NULL;
    if(OldFunction == NULL) {
        BreakLabel = NewLabel();
        NewFunction = AddSymbol(CurrentIdentifier, Type, ST_FUNC, SC_GLOBAL, BreakLabel, 0, NULL);
    }

    VerifyToken(LI_LPARE, "(");
    ParamCount = ReadDeclarationList(OldFunction, SC_GLOBAL, LI_RPARE);
    VerifyToken(LI_RPARE, ")");

    printf("\nIdentified%sfunction %s of return type %s, end label %d\n", 
        (OldFunction == NULL) ? " new " : " overloaded ", 
        (OldFunction == NULL) ? NewFunction->Name : OldFunction->Name, 
        TypeNames(Type), BreakLabel);

    if(NewFunction) {
        NewFunction->Elements = ParamCount;
        NewFunction->Start = Params;
        OldFunction = NewFunction;
    }

    Params = ParamsEnd = NULL;

    if(CurrentToken.type == LI_SEMIC) {
        Tokenise();
        return NULL;
    }

    FunctionEntry = OldFunction;

    Tree = ParseCompound();

    if(Type != RET_VOID) {
        // Functions with one statement have no composite node, so we have to check
        FinalStatement = (Tree->Operation == OP_COMP) ? Tree->Right : Tree;

        if(FinalStatement == NULL || FinalStatement->Operation != OP_RET) {
            Die("Function with non-void type does not return");
        }

    }

    return ConstructASTBranch(OP_FUNC, Tree->ExprType, Tree, OldFunction, BreakLabel);
}

/*
 *  Handles the logic for return.
 *  //TODO: No brackets
 *  //TODO: Type inference
 * 
 */

struct ASTNode* ReturnStatement() {
    struct ASTNode* Tree;
    int ReturnType;


    if(FunctionEntry->Type == RET_VOID)
        Die("Attempt to return from void function");
    
    VerifyToken(KW_RETURN, "return");

    VerifyToken(LI_LPARE, "("); // TODO: Make optional! Reject?

    Tree = ParsePrecedenceASTNode(0);

    Tree = MutateType(Tree, FunctionEntry->Type, 0);
    if(Tree == NULL)
        Die("Returning a value of incorrect type for function");
    

    Tree = ConstructASTBranch(OP_RET, RET_NONE, Tree, FunctionEntry, 0);

    printf("\t\tReturning from function %s\n", FunctionEntry->Name);

    VerifyToken(LI_RPARE, ")"); // TODO: OPTIONALISE!

    return Tree;
}



/*
 * Handles the surrounding logic for If statements.
 * 
 * If statements have the basic form:
 *  * if (condition) body
 *  * if (condition)
 *        body
 *  * if (condition) {
 *        body
 *    }
 * 
 * Conditions may be any truthy statement (such as a pointer,
 *  object, integer), as conditions not recognized are auto-
 *   matically converted to booleans.
 * 
 * This meaning, any object that can be resolved to 0 or NULL
 *  can be placed as the condition and used as a check.
 * 
 * For example:
 *  struct ASTNode* Node = NULL;
 *  if(Node) {
 *      // This will not run, as Node is ((void*)0)
 *  }
 * 
 */
struct ASTNode* IfStatement() {
    struct ASTNode* Condition, *True, *False = NULL;

    VerifyToken(KW_IF, "if");
    VerifyToken(LI_LPARE, "(");

    Condition = ParsePrecedenceASTNode(0);

    // Limit if(x) to =? != < > <= => 
    // No null checking, no arithmetic, no functions.
    // TODO: this
    if(Condition->Operation < OP_EQUAL || Condition->Operation > OP_GREATE)
        Condition = ConstructASTBranch(OP_BOOLCONV, Condition->ExprType, Condition, NULL, 0);
    
    VerifyToken(LI_RPARE, ")");

    True = ParseCompound();

    if(CurrentToken.type == KW_ELSE) {
        Tokenise();
        False = ParseCompound();
    }

    return ConstructASTNode(OP_IF, RET_NONE, Condition, True, False, NULL, 0);
}

/*
 * Handles the surrounding logic for While loops.
 * 
 * While loops have the basic form:
 *  while ( condition ) { body }
 * 
 * When reaching the condition (which alike an If statement,
 *  can be any truthy value), if it resolves to true:
 * The body is executed, and immediately the condition is checked
 *  again.
 * This repeats until the condition resolves false, at which point
 *  the loop executes no more.
 * 
 * This can be prototyped as the following pseudo-assembler:
 * 
 * cond:
 *      check <condition>
 *      jne exit
 *      <body>
 *      jump cond
 * exit:  
 *      <more code>
 * 
 * @return the AST of this statement
 * 
 */
struct ASTNode* WhileStatement() {
    struct ASTNode* Condition, *Body;

    VerifyToken(KW_WHILE, "while");
    VerifyToken(LI_LPARE, "(");

    Condition = ParsePrecedenceASTNode(0);


    if(Condition->Operation < OP_EQUAL || Condition->Operation > OP_GREATE)
        Condition = ConstructASTBranch(OP_BOOLCONV, Condition->ExprType, Condition, NULL, 0);
    
    VerifyToken(LI_RPARE, ")");

    Body = ParseCompound();

    return ConstructASTNode(OP_LOOP, RET_NONE, Condition, NULL, Body, NULL, 0);
}

/*
 * Handles the surrounding logic for For loops.
 * 
 * They have the basic form of:
 *  for ( init ; condition; iterator) { body }
 * 
 * The initialiser is run only once upon reaching the for loop.
 * Then the condition is checked, and if true, the body is executed.
 * After execution of the body, the iterator is run and the condition
 *  checked again.
 * 
 * It can be prototyped as the following pseudo-assembler code:
 * 
 * for: 
 *      <init>
 * cond:
 *      check <condition>
 *      jne exit
 *      <body>
 *      <iterator>
 *      jump cond
 * exit:
 *      <loop exit>
 * 
 * In the case of the implementation, "init" is the preoperator,
 *  "iterator" is the postoperator.
 * 
 * @return the AST of this statement
 */
struct ASTNode* ForStatement() {
    struct ASTNode* Condition, *Body;
    struct ASTNode* Preop, *Postop;

    struct ASTNode* Tree;

    VerifyToken(KW_FOR, "for");
    VerifyToken(LI_LPARE, "(");

    Preop = ParseStatement();
    VerifyToken(LI_SEMIC, ";");

    Condition = ParsePrecedenceASTNode(0);

    if(Condition->Operation < OP_EQUAL || Condition->Operation > OP_GREATE)
        Condition = ConstructASTBranch(OP_BOOLCONV, Condition->ExprType, Condition, NULL, 0);

    VerifyToken(LI_SEMIC, ";");

    Postop = ParseStatement();
    VerifyToken(LI_RPARE, ")");

    Body = ParseCompound();

    // We need to be able to skip over the body and the postop, so we group them together.
    Tree = ConstructASTNode(OP_COMP, RET_NONE, Body, NULL, Postop, NULL, 0);
    // We need to be able to jump to the top of the condition and fall through to the body,
    //  so we group it with the last block
    Tree = ConstructASTNode(OP_LOOP, RET_NONE, Condition, NULL, Tree, NULL, 0);

    // We need to append the postop to the loop, to form the final for loop
    return ConstructASTNode(OP_COMP, RET_NONE, Preop, NULL, Tree, NULL, 0);
}


/*
 * Handles the surrounding logic for the Print statement.
 * 
 * This is a legacy hold-over from the early testing, and it
 *  serves merely as a wrapper around the cstdlib printf function.
 * 
 * It does, however (//TODO), attempt to guess the type that you
 *  want to print, which takes a lot of the guesswork out of printing.
 * 
 * @return the AST of this statement
 */
struct ASTNode* PrintStatement(void) {
    struct ASTNode* Tree;
    int LeftType, RightType;

    VerifyToken(KW_PRINT, "print");

    Tree = ParsePrecedenceASTNode(0);

    LeftType = RET_INT;
    RightType = Tree->ExprType;

    Tree = MutateType(Tree, RightType, 0);
    if(!Tree)
        DieDecimal("Attempting to print an invalid type:", RightType);
    
    if(RightType)
        Tree = ConstructASTBranch(Tree->Right->Operation, RET_INT, Tree, NULL, 0);
    
    Tree = ConstructASTBranch(OP_PRINT, RET_NONE, Tree, NULL, 0);

    //ParseAST(Tree);

    return Tree;
   
}

/*
 * Handles the surrounding logic for all of the logical and semantic
 *  postfixes.
 * 
 * Postfixes are tokens that are affixed to the end of another, and
 *  change behaviour in some way. These can be added calculations,
 *   some form of transformation, or other.
 * 
 * A current list of postfixes:
 *  * (): Call a function
 *  * []: Index or define an array.
 *  * ++: Increment a variable AFTER it is returned
 *          NOTE: there is a prefix variant of this for incrementing BEFOREhand.
 *  * --: Decrement a variable AFTER it is returned
 *          NOTE: there is a prefix variant of this for decrementing BEFOREhand.
 * 
 * Planned postfixes:
 *  * >>: Arithmetic-Shift-Right a variable by one (Divide by two)
 *          NOTE: there is a prefix variant of this for shifting left - multiplying by two.
 * 
 * @return the AST of the statement plus its' postfix
 */
struct ASTNode* PostfixStatement() {
    struct ASTNode* Tree;
    struct SymbolTableEntry* Entry;

    Tokenise();
    
    if(CurrentToken.type == LI_LPARE)
        return CallFunction();
    
    if(CurrentToken.type == LI_LBRAS)
        return AccessArray();
    
    // If we get here, we must be a variable.
    //  (as functions have been called and arrays have been indexed)
    // Check that the variable is recognized..

    if((Entry = FindSymbol(CurrentIdentifier)) == NULL || Entry->Structure != ST_VAR)
        DieMessage("Unknown Variable", CurrentIdentifier);

    // Here we check for postincrement and postdecrement.

    switch(CurrentToken.type) {
        case PPMM_PLUS:
            Tokenise();
            Tree = ConstructASTLeaf(OP_POSTINC, Entry->Type, Entry, 0);
            break;
        case PPMM_MINUS:
            Tokenise();
            Tree = ConstructASTLeaf(OP_POSTDEC, Entry->Type, Entry, 0);
            break;
        default:
            Tree = ConstructASTLeaf(REF_IDENT, Entry->Type, Entry, 0);
    }

    return Tree;
    
}

/*
 * Handles the surrounding logic for all of the logical and semantic
 *  prefixes.
 * 
 * Prefixes are tokens that are affixed to the start of another, and
 *  change behaviour in some way. These can be added calculations,
 *   some form of transformation, or other.
 * 
 * A current list of prefixes:
 *  *  !: Invert the boolean result of a statement or truthy value.
 *  *  ~: Invert the individual bits in a number
 *  *  -: Invert the number around the axis of 0 (negative->positive, positive->negative)
 *  * ++: Increment a variable BEFORE it is returned.
 *          NOTE: there is a postfix variant of this for incrementing AFTER the fact.
 *  * --: Decrement a variable BEFORE it is returned.
 *          NOTE: there is a postfix variant of this for decrementing AFTER the fact.
 *  *  &: Dereference the following object (Get the address that contains it)
 *  *  *: Get the object pointed at by the number following
 * 
 * Planned prefixes:
 *  * <<: Arithmetic-Shift-Left a variable by one (Multiply by two)
 *          NOTE: there is a postfix variant of this for shifting right - dividing by two.
 * 
 * @return the AST of this statement, plus its' prefixes and any postfixes.
 */
struct ASTNode* PrefixStatement() {
    struct ASTNode* Tree;

    switch (CurrentToken.type) {
        case BOOL_INVERT:
            Tokenise();
            Tree = PrefixStatement();
            Tree->RVal = 1;
            Tree = ConstructASTBranch(OP_BOOLNOT, Tree->ExprType, Tree, NULL, 0);
            break;

        case BIT_NOT:
            Tokenise();
            Tree = PrefixStatement();
            Tree->RVal = 1;
            Tree = ConstructASTBranch(OP_BITNOT, Tree->ExprType, Tree, NULL, 0);
            break;
        
        case AR_MINUS:
            Tokenise();
            Tree = PrefixStatement();
            
            Tree = ConstructASTBranch(OP_NEGATE, Tree->ExprType, Tree, NULL, 0);
            break;

        case PPMM_PLUS:
            Tokenise();
            Tree = PrefixStatement();

            if(Tree->Operation != REF_IDENT)
                Die("++ not followed by identifier");
            Tree = ConstructASTBranch(OP_PREINC, Tree->ExprType, Tree, NULL, 0);
            break;

        case PPMM_MINUS:
            Tokenise();
            Tree = PrefixStatement();

            if(Tree->Operation != REF_IDENT)
                Die("-- not followed by identifier");
            
            Tree = ConstructASTBranch(OP_PREDEC, Tree->ExprType, Tree, NULL, 0);
            break;

        case BIT_AND:
            Tokenise();

            // To allow things like:
            // x = &&y;
            // We need to recursively parse prefixes;
            Tree = PrefixStatement();

            if(Tree->Operation != REF_IDENT) 
                Die("& must be followed by another & or an identifier.");

            Tree->Operation = OP_ADDRESS;
            Tree->ExprType = PointerTo(Tree->ExprType);
            break;
        case AR_STAR:
            Tokenise();

            Tree = PrefixStatement();

            if(Tree->Operation != REF_IDENT && Tree->Operation != OP_DEREF)
                Die("* must be followed by another * or an identifier.");

            Tree = ConstructASTBranch(OP_DEREF, ValueAt(Tree->ExprType), Tree, NULL, 0);
            break;

        default:
            Tree = ParsePrimary();
            
    }

    return Tree;
}