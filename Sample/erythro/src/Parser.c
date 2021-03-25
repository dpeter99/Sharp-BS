
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <stdio.h>
#include <stdlib.h>
#include "Defs.h"
#include "Data.h"

/*
 * The Precedence of an operator is directly related to Token Type.
 * Precedence determines how soon the operator and its surrounding values
 *  will be calculated and aliased.
 * This allows for things like the common Order of Operations.
 */
static int Precedence[] = {
           0, 10, // EOF, ASSIGN
          20, 30, // || &&
          40, 50, // | ^
          60, 70, // & =?
          70, 80, // != <
          80, 80, // > <=
          80, 90, // => <<
         90, 100, // >> +
        100, 110, // - *
        110       // /
};    

/*
 * Handles gathering the precedence of an operator from its token,
 *  in terms of values of the TokenTypes enum.
 * 
 * Error handling is also done here, so that EOF or non-operators are not executed.
 * 
 */
static int OperatorPrecedence(int Token) {
    int Prec = Precedence[Token];

    if(Prec == 0 || Token >= PPMM_PLUS) {
        DieMessage("Attempting to determine operator precedence of an EOF or INT literal", TokenNames[Token]);
    }

    return Prec;
}

/*
 * If the value is a right-expression, or in other words is right associative,
 *  then it can be safely calculated beforehand and aliased to a value.
 * In this case, we can try to alias (or constant fold) everything on the right side
 *  of an assignment.
 */

static int IsRightExpr(int Token) {
   return (Token == LI_EQUAL);
}

/* * * * * * * * * * * * * * * * * * * * * * * *
 * * * N O D E     C O N S T R U C T I O N * * *
 * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * ASTNodes form the structure of the language that moves the bulk of
 *  data around within the compiler.
 * They contain:
 *  * An Operation (usually 1:1 with an input token),
 *  * A Type (to identify the size of data it contains),
 *  * Two more Left and Right ASTNodes (to form a doubly-linked list)
 *  * An extra Middle ASTNode in case it is needed (typically in the middle case of a For loop)
 *  * A Symbol Table Entry
 *  * An Integer Value
 *  * A flag to determine whether this node (and its sub-nodes) contain a right associative or Rval
 * 
 * This is the only function where they are constructed.
 * 
 * @param Operation: The input Op of this Node, in terms of values of the SyntaxOps enum
 * @param Type: The data type of this Node, in terms of values of the DataTypes enum.
 * @param Left: The Node that is attached to the left side branch of this root.
 * @param Middle: The Node that is attached to the middle of this root, if applicable.
 * @param Right: The Node that is attached to the right side branch of this root.
 * @param Symbol: The Symbol Table Entry that represents this Node, if applicable.
 * @param IntValue: The integer value encoded by this Node, if applicable.
 * @return a newly constructed AST Node
 */
struct ASTNode* ConstructASTNode(int Operation, int Type,
                                 struct ASTNode* Left,
                                 struct ASTNode* Middle,
                                 struct ASTNode* Right,
                                 struct SymbolTableEntry* Symbol,
                                 int IntValue) {
                                    
    struct ASTNode* Node;

    Node = (struct ASTNode*) malloc(sizeof(struct ASTNode));

    if(!Node)
        Die("Unable to allocate node!");


    Node->Operation = Operation;
    Node->ExprType = Type;
    Node->Left = Left;
    Node->Middle = Middle;
    Node->Right = Right;
    Node->Symbol = Symbol;
    Node->IntValue = IntValue;

    return Node;
}


/*
 * AST Leaves are categorized by their lack of child nodes.
 * @param Operation: The input Op of this Node, in terms of values of the SyntaxOps enum
 * @param Type: The data type of this Node, in terms of values of the DataTypes enum.
 * @param Symbol: The Symbol Table Entry that represents this Node, if applicable.
 * @param IntValue: The integer value encoded by this Node, if applicable.
 * @return a newly constructed AST Node
 */
struct ASTNode* ConstructASTLeaf(int Operation, int Type, struct SymbolTableEntry* Symbol, int IntValue) {
    return ConstructASTNode(Operation, Type, NULL, NULL, NULL, Symbol, IntValue);
}

/*
 * AST Branches are categorized by having only one child node.
 *  These are sometimes called Unary Branches.
 * @param Operation: The input Op of this Node, in terms of values of the SyntaxOps enum
 * @param Type: The data type of this Node, in terms of values of the DataTypes enum.
 * @param Left: The Node that is attached to the left side branch of this root.
 * @param Symbol: The Symbol Table Entry that represents this Node, if applicable.
 * @param IntValue: The integer value encoded by this Node, if applicable.
 * @return a newly constructed AST Node
 */
struct ASTNode* ConstructASTBranch(int Operation, int Type, struct ASTNode* Left, struct SymbolTableEntry* Symbol, int IntValue) {
    return ConstructASTNode(Operation, Type, Left, NULL, NULL, Symbol, IntValue);
}


/* * * * * * * * * * * * * * * * * * * * * * * *
 * * * *  T O K E N       P A R S I N G  * * * *
 * * * * * * * * * * * * * * * * * * * * * * * */

/* 
 * TokenTypes and SyntaxOps are mostly 1:1, so some minor effort can ensure that
 *  these are synchronized well.
 * This allows the parsing operation to be little more than a bounds check.
 * Otherwise, this would be a gigantic switch statement.
 */

int ParseTokenToOperation(int Token) {
    if(Token > LI_EOF && Token < LI_INT)
        return Token;

    DieDecimal("ParseToken: Unknown token", Token);
}

/*
 * Primary expressions may be any one of:
 *  * A terminal integer literal
 *  * A terminal string literal
 *  * A variable
 *  * A collection of expressions bounded by parentheses.
 * 
 * @return the AST Node that represents this expression
 */

struct ASTNode* ParsePrimary(void) {
    struct ASTNode* Node;
    int ID;

    switch(CurrentToken.type) {
        case LI_INT:

            if((CurrentToken.value >= 0) && (CurrentToken.value < 256))
                Node = ConstructASTLeaf(TERM_INTLITERAL, RET_CHAR, NULL, CurrentToken.value);
            else 
                Node = ConstructASTLeaf(TERM_INTLITERAL, RET_INT, NULL, CurrentToken.value);
            break;

        case LI_STR:

            ID = AsNewString(CurrentIdentifier);
            Node = ConstructASTLeaf(TERM_STRLITERAL, PointerTo(RET_CHAR), NULL, ID);
            break;

        case TY_IDENTIFIER:
            return PostfixStatement();

        case LI_LPARE:
            // Starting a ( expr ) block
            Tokenise();

            Node = ParsePrecedenceASTNode(0);

            // Make sure we close
            VerifyToken(LI_RPARE, ")");
            return Node;
    }

        
    Tokenise();

    return Node;
}


/*
 * Parse a single binary expression.
 * It ensures that these expressions are parsed to their full extent, that
 *  the order of operations is upheld, that the precedence of the prior
 *  iteration is considered, and that every error is handled.
 * 
 * This is where all of the right-associative statements are folded, where
 *  type mismatches and widening are handled properly, and that all parsing 
 *   is over by the time the end tokens ") } ] ;" are encountered.
 * 
 * @param PreviousTokenPrecedence: The precedence of the operator to the left.
 * @return the AST Node corresponding to this block.
 * 
 */
struct ASTNode* ParsePrecedenceASTNode(int PreviousTokenPrecedence) {
    struct ASTNode* LeftNode, *RightNode;
    struct ASTNode* LeftTemp, *RightTemp;
    // int LeftType, RightType;
    int NodeType, OpType;

    LeftNode = PrefixStatement();

    NodeType = CurrentToken.type;

    if(NodeType == LI_SEMIC || NodeType == LI_RPARE || NodeType == LI_RBRAS || NodeType == LI_COM) {
        LeftNode->RVal = 1; return LeftNode;
    }
    
    while((OperatorPrecedence(NodeType) > PreviousTokenPrecedence) || (IsRightExpr(OpType) && OperatorPrecedence(OpType) == PreviousTokenPrecedence)) {
        Tokenise();
        if(CurrentToken.type == LI_RPARE)
            break;

        RightNode = ParsePrecedenceASTNode(Precedence[NodeType]);


        /**
         * While parsing this node, we may need to widen some types.
         * This requires a few functions and checks.
         */

        OpType = ParseTokenToOperation(NodeType);

        if(OpType == OP_ASSIGN) {
            printf("\tParsePrecedenceASTNode: Assignment statement\r\n");
            RightNode->RVal = 1;
            LeftNode->RVal = 0;

            RightNode = MutateType(RightNode, LeftNode->ExprType, 0);
            if(LeftNode == NULL)
                Die("Incompatible Expression encountered in assignment");

            // LeftNode holds the target, the target variable in this case
            printf("\t\tAssigning variable: %s\n", LeftNode->Symbol->Name);

            LeftTemp = LeftNode;
            LeftNode = RightNode;
            RightNode = LeftTemp;
            
            // Clear temps as ensurance
            RightTemp = NULL;
            LeftTemp = NULL;
        } else {
            printf("\t\tAttempting to handle a %d in Binary Expression parsing\r\n", CurrentToken.type);
            LeftNode->RVal = 1;
            RightNode->RVal = 1;

            LeftTemp = MutateType(LeftNode, RightNode->ExprType, OpType);

            RightTemp = MutateType(RightNode, LeftNode->ExprType, OpType);
            /**
             * If both are null, the types are incompatible.
             */

            if(LeftTemp == NULL && RightTemp == NULL)
                Die("Incompatible types in parsing nodes");

            /**
             * If the left was valid, or valid for 
             *  expansion, then it will be non-null.
             * 
             * If it was valid, then this will be 
             *  equivalent to LeftNode = LeftNode
             */
        
            if(LeftTemp)
                LeftNode = LeftTemp;

            /**
             * Same here, but there is a higher chance
             * for the right node to be incompatible due
             * to the nature of widening types.
             */

            if(RightTemp)
                RightNode = RightTemp;
        }
        
        /**
         * Checks over, back to normal parsing.
         */
        
        if(LeftTemp != NULL)
            LeftNode = LeftTemp; //ConstructASTBranch(LeftType, RightNode->ExprType, LeftNode, 0);
        if(RightTemp != NULL)
            RightNode = RightTemp; // ConstructASTBranch(RightType, LeftNode->ExprType, RightNode, 0);
        
        LeftNode = ConstructASTNode(ParseTokenToOperation(NodeType), LeftNode->ExprType, LeftNode, NULL, RightNode, NULL, 0);
        NodeType = CurrentToken.type;
        if(NodeType == LI_SEMIC || NodeType == LI_RPARE || NodeType == LI_RBRAS) {
            LeftNode->RVal = 1;
            return LeftNode;
        }
        
    }
    LeftNode->RVal = 1;
    return LeftNode;
}



/* * * * * * * * * * * * * * * * * * * * *
 * * * *     F U N C T I O N S     * * * *
 * * * * * * * * * * * * * * * * * * * * */

/*
 * Handles the logic for calling a function.
 * This is invoked by an identifier being recognized, followed by a "(.*)" string.
 * 
 * It simply checks that the function exists, that the parameters given are valid,
 *  and generates the AST Node for calling it.
 * 
 * @return the AST Node for calling the function stored in CurrentIdentifer
 * 
 */
struct ASTNode* CallFunction() {
    struct ASTNode* Tree;
    struct SymbolTableEntry* Function;

    //TODO: Test structural type!
    if((Function = FindSymbol(CurrentIdentifier)) == NULL || (Function->Structure != ST_FUNC))
        DieMessage("Undeclared function", CurrentIdentifier);

    VerifyToken(LI_LPARE, "(");

    Tree = GetExpressionList();

    Tree = ConstructASTBranch(OP_CALL, Function->Type, Tree, Function, 0);

    VerifyToken(LI_RPARE, ")");

    return Tree;
}


/*
 * An expression list is used:
 *  * In the call to a function
 * 
 * It is parsed by seeking left parentheses "(", parsing binary expressions
 *  until either a comma or a right parentheses is found.
 * 
 * The former will cause another expression to be parsed, the latter will cause
 *  parsing to stop.
 * 
 * @return the AST Node representing every expression in the list, glued end to
 *  end with a COMPOSITE operation.
 * 
 */
struct ASTNode* GetExpressionList() {
    struct ASTNode* Tree = NULL, *Child = NULL;
    int Count;

    while(CurrentToken.type != LI_RPARE) {
        Child = ParsePrecedenceASTNode(0);
        Count++;

        Tree = ConstructASTNode(OP_COMP, PointerTo(RET_VOID), Tree, NULL, Child, NULL, Count);

        switch(CurrentToken.type) {
            case LI_COM:
                Tokenise();
                break;
            case LI_RPARE:
                break;
            default:
                Die("Unexpected token in argument list");
        }
    }

    return Tree;
}


/* * * * * * * * * * * * * * * * * * * * * *
 * * * *     S T A T E M E N T S     * * * *
 * * * * * * * * * * * * * * * * * * * * * */

/*
 * Handles parsing an individual statement.
 * 
 * It serves as a wrapper around:
 *  * If Statement
 *  * While Statement
 *  * For Statement
 *  * Return Statement
 *  * Numeric literals and variables
 *  * Binary Expressions
 * @return the AST Node representing this single statement
 */
struct ASTNode* ParseStatement(void) {
    int Type;
    
    printf("\t\tBranch leads to here, type %s/%d\r\n", TokenNames[CurrentToken.type], CurrentToken.type);
    switch(CurrentToken.type) {
        case TY_CHAR:
        case TY_LONG:
        case TY_INT:
            printf("\t\tNew Variable: %s\n", CurrentIdentifier);
            Type = ParseOptionalPointer(NULL);
            VerifyToken(TY_IDENTIFIER, "ident");
            BeginVariableDeclaration(Type, NULL, SC_LOCAL);
            VerifyToken(LI_SEMIC, ";"); // TODO: single line assignment?
            return NULL;

        case KW_IF:
            return IfStatement();
        
        case KW_WHILE:
            return WhileStatement();
        
        case KW_FOR:
            return ForStatement();

        case KW_RETURN:
            return ReturnStatement();
        
        default:
            ParsePrecedenceASTNode(0);
    }
}


/*
 * Handles parsing multiple statements or expressions in a row.
 * These are typically grouped together with the Compound tokens "{ }"
 *  and seperated by the semicolon ";".
 *
 * Single Statements are parsed until a semicolon is reached, at which
 *  point another statement will be parsed, or until a Right Compound
 *   token is reached ("}"), at which point parsing will stop.
 * 
 * It is useful for:
 *  * Tightly identifying related blocks of code
 *  * Containing the many statements of functions
 * 
 * @return the AST Node representing this compound statement
 * 
 */
struct ASTNode* ParseCompound() {
    struct ASTNode* Left = NULL, *Tree;
    
    // Compound statements are defined by comprising
    //  multiple statements inside { a bracket block }
    VerifyToken(LI_LBRAC, "{");

    while(1) {
        printf("\tNew branch in compound\n");
       
        Tree = ParseStatement();

        if(Tree && (Tree->Operation == OP_PRINT || Tree->Operation == OP_ASSIGN
                        || Tree->Operation == OP_RET || Tree->Operation == OP_CALL))
            VerifyToken(LI_SEMIC, ";");
        
        if(Tree) {
            if(Left == NULL)
                Left = Tree;
            else
                Left = ConstructASTNode(OP_COMP, RET_NONE, Left, NULL, Tree, NULL, 0);
        }

        if(CurrentToken.type == LI_RBRAC) {
            VerifyToken(LI_RBRAC, "}");
            return Left;
        }
    }
}

/*
 * This is the entry point to the parser/lexer.
 * 
 * By definition, Global definitions are accessible anywhere.
 *  As of right now (20/01/2021), classe are unimplemented.
 * This means that all functions and all function prototypes are globally scoped.
 * 
 * You may also define variables, constants, preprocessor directives and other text
 *  in the global scope.
 * 
 * The function itself loops, parsing either variables or functions, until it 
 *  reaches the end of the file. 
 * 
 */

void ParseGlobals() {
    struct ASTNode* Tree;
    struct SymbolTableEntry* Composite;
    int Type, FunctionComing;

    printf("Parsing global definitions\r\n");

    while(1) {
        
        // We loop early if there's a struct, and since a struct may be the last 
        // thing in a file, we need to check for eof before anything else
        if(CurrentToken.type == LI_EOF)
            break;

        printf("New definition incoming..\r\n\n");
        Type = ParseOptionalPointer(&Composite);

        //TODO: converge pathways on this block?
        if(CurrentToken.type == KW_FUNC) {
            VerifyToken(KW_FUNC, "::");
            FunctionComing = 1;
        }

        // Structs are parsed fully in ParseOptionalPointer
        // TODO: FIX THAT!!
        if(Type == DAT_STRUCT && CurrentToken.type == LI_SEMIC) {
            Tokenise();
            continue;
        }

        VerifyToken(TY_IDENTIFIER, "ident");

        if(FunctionComing && CurrentToken.type == LI_LPARE) {
            printf("\tParsing function");
            Tree = ParseFunction(Type);
            if(Tree) {
                printf("\nBeginning assembler creation of new function %s\n", Tree->Symbol->Name);
                AssembleTree(Tree, -1, 0);
                FreeLocals();
            } else {
                printf("\nFunction prototype saved\r\n");
            }
        } else {
            printf("\tParsing global variable declaration\n");
            BeginVariableDeclaration(Type, Composite, SC_GLOBAL);
            VerifyToken(LI_SEMIC, ";");
        }
        
    }
}


