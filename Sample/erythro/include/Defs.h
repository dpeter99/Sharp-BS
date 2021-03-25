/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/*
 * ARithmetic tokens are prefixed AR.
 * LIteral tokens are prefixed LI.
 * KeyWords are prefixed KW.
 * TYpes are prefixed TY.
 * CoMParisons are prefixed CMP.
 * BOOLean maths is prefixed BOOL.
 * BITwise maths is prefixed BIT.
 * Arithmetic SHifts are prefixed SH.
 * PlusPlusMinusMinus operators are prefixed PPMM.
 * 
 * 
 * NOTE: Tokens are different from Syntax Operations!
 * 
 * Tokens should represent the characters that invoke them,
 *  not the actions they perform.
 *  
 */


enum TokenTypes {
    LI_EOF,
    LI_EQUAL,       // =

    BOOL_OR,        // Boolean OR (||)
    BOOL_AND,       // Boolean AND (&&)

    BIT_OR,         // Bitwise OR (|)
    BIT_XOR,        // Bitwise XOR (^)
    BIT_AND,        // Bitwise AND (&)

    CMP_EQUAL,      // =?
    CMP_INEQ,       // !=
    CMP_LT,         // <
    CMP_GT,         // >
    CMP_LTE,        // <=
    CMP_GTE,        // =>

    SH_LEFT,        // Left Shift (<<)
    SH_RIGHT,       // Right Shift (>>)

    AR_PLUS,        // Arithmetic +
    AR_MINUS,       // Arithmetic -
    AR_STAR,        // Arithmetic *
    AR_SLASH,       // Arithmetic /

    PPMM_PLUS,      // PPMM Increment (++)
    PPMM_MINUS,     // PPMM Decrement (--)

    BOOL_INVERT,    // Boolean Invert (!)

    BIT_NOT,        // Bitwise NOT (~)

    LI_INT,         // Integer literal
    LI_STR,         // String literal
    LI_SEMIC,       // ;

    LI_LBRAC,       // {
    LI_RBRAC,       // }

    LI_LBRAS,       // [
    LI_RBRAS,       // ]

    LI_LPARE,       // (
    LI_RPARE,       // )

    LI_COM,         // ,

    TY_IDENTIFIER,  // Identifier name. Variable, function, etc.
    TY_NONE,        // No return type. Literal void.
    TY_CHAR,        // "char" type keyword
    TY_INT,         // "int" type keyword
    TY_LONG,        // "long" type keyword
    TY_VOID,        // "void" type keyword

    KW_FUNC,        // :: function name incoming
    
    KW_PRINT,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_RETURN,
    KW_STRUCT
};

/*
 * All Syntax Operations are prefixed OP.
 *  Terminal Operations are prefixed TERM.
 *  L-Values are prefixed LV.
 *  Reference Operations are prefixed REF.
 *
 * These represent the actions that a token will perform.
 * These are used exclusively in AST construction.
 * 
 * It is important that Tokens and Operations are logically separated,
 *  but that the Operation's index is the same as the Token that invokes it.
 */

enum SyntaxOps {
    OP_ASSIGN = 1,      // Assign an l-value

    OP_BOOLOR,          // Boolean OR two statements
    OP_BOOLAND,         // Boolean AND two statements
    OP_BITOR,           // Bitwise OR a number
    OP_BITXOR,          // Bitwise XOR a number
    OP_BITAND,          // Bitwise AND a number
    
    OP_EQUAL,           // Compare equality
    OP_INEQ,            // Compare inequality
    OP_LESS,            // Less than?
    OP_GREAT,           // Greater than?
    OP_LESSE,           // Less than or Equal to?
    OP_GREATE,          // Greater than or Equal to?

    OP_SHIFTL,          // Arithmetic Shift Left (Multiply by 2)
    OP_SHIFTR,          // Arithmetic Shift Right (Divide by 2)

    OP_ADD,             // Add two numbers.
    OP_SUBTRACT,        // Subtract two numbers.
    OP_MULTIPLY,        // Multiply two numbers.
    OP_DIVIDE,          // Divide two numbers.

    OP_PREINC,          // Increment var before reference.
    OP_PREDEC,          // Decrement var before reference.
    OP_POSTINC,         // Increment var after reference.
    OP_POSTDEC,         // Decrement var after reference.

    OP_BITNOT,          // Invert a number bitwise
    OP_BOOLNOT,         // Invert a statement logically
    OP_NEGATE,          // Negate a number (turn a positive number negative)

    OP_BOOLCONV,        // Convert an expression to a boolean.s
    
    OP_ADDRESS,         // Fetch the address of a var
    OP_DEREF,           // Get the value of the address in a pointer

    TERM_INTLITERAL,    // Integer Literal. This is a virtual operation, so it's a terminal.
    TERM_STRLITERAL,    // String Literal. Also terminal.

    REF_IDENT,          // Reference (read) an identifier (variable).

    OP_WIDEN,           // Something contains a type that needs to be casted up
    OP_SCALE,           // We have a pointer that needs to be scaled!

    OP_CALL,            // Call a function
    OP_RET,             // Return from a function

    OP_COMP,            // Compound statements need a way to be "glued" together. This is one of those mechanisms
    OP_IF,              // If statement
    OP_LOOP,            // FOR, WHILE
    OP_PRINT,           // Print statement

    OP_FUNC,            // Define a function
};


// A node in a Binary Tree that forms the syntax of Erythro
struct ASTNode {
    int Operation;      // SyntaxOps Index
    int ExprType;       // Value->IntValue's DataType
    int RVal;           // True if this node is an Rval, false if Lval
    struct ASTNode* Left;
    struct ASTNode* Middle;
    struct ASTNode* Right;
    struct SymbolTableEntry* Symbol;
    union {
        int Size;     // OP_SCALE's linear representation
        int IntValue; // TERM_INTLIT's Value
    };
};

struct Token {
    int type;
    int value;
};

/*
 * The Symbol Table, used for variables, functions and
 *  assorted goodies.
 */

struct SymbolTableEntry {
    char* Name;
    int Type;       // An entry in DataTypes, referring to the type of this data
    struct SymbolTableEntry* CompositeType; // A pointer to the start of a Symbol Table list that represents a certain Composite type
    int Structure;  // An entry in StructureType - metadata on how to process the data
    int Storage;    // The scope of this symbol - decides when it is discarded.
    union {
        int EndLabel;   // For a function - The number of the label to jump to, in order to exit this function (if applicable)
        int Length;     // For an array - The length of the symbol in units of 1 element -- the size of an array, for example.
        int IntValue;   // For an enum - The value of an Enum entry
    };

    union {
        int SinkOffset; // For a variable - How many times must we sink the rbp to get to this symbol in the stack?
        int Elements;   // For a function - How many parameters?
    };

    struct SymbolTableEntry* NextSymbol; // The next symbol in a list
    struct SymbolTableEntry* Start; // The first member in a list
};

enum StorageScope {
    SC_GLOBAL = 1,  // Global Scope
    SC_STRUCT,      // Struct Definitions
    SC_ENUM,        // Enum Definitions
    SC_MEMBER,      // The members of Structs or Enums
  //SC_CLASS,       // Class-local definitions
  //SC_STATIC,      // Static storage definitions
    SC_PARAM,       // Function parameters
    SC_LOCAL        // Function-local scope.
                    //  There is no deeper scope than function.
};


/*
 * The types of data being held in memory.
 * The lowest 4 bits of these enum values
 *  encode a nested pointer type.
 *
 * This meaning, a single enum can hold
 *  ****************int types.
 * Should be enough for everyone, right?
 */
enum DataTypes {
    RET_NONE,            // No return type. Literal void.
    RET_CHAR = 16,       // "char" type keyword
    RET_INT = 32,        // "int" type keyword
    RET_LONG = 48,       // "long" type keyword
    RET_VOID = 64,       // "void" type keyword
    
    DAT_STRUCT = 80,     // Struct Data
    DAT_UNION,           // Union Data
};

/*
 * The type of the structure of data being examined
 * //TODO: move into TokenTypes?
 */

enum StructureType {
    ST_VAR,         // This is variable
    ST_FUNC,        // This is a function
    ST_ARR          // This is an array
                    // This is an enum
                    // This is a struct
                    // This is a typedef
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * *      A R G U M E N T S      * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

char* Suffixate(char* String, char Suffix);
char* Compile(char* InputFile);
char* Assemble(char* InputFile);
void Link(char* Output, char* Objects[]);
void DisplayUsage(char* ProgName);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * *    L E X I N G      * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void Tokenise();

void VerifyToken(int Type, char* TokenExpected);
void RejectToken(struct Token* Token);

static int ReadIdentifier(int Char, char* Buffer, int Limit);
static int ReadKeyword(char* Str);

/* * * * * * * * * * * * * * * * * * * *
 * * * * *     T Y P E S     * * * * * *
 * * * * * * * * * * * * * * * * * * * */

struct ASTNode* MutateType(struct ASTNode* Tree, int RightType, int Operation);

int TypeIsInt(int Type);
int TypeIsPtr(int Type);

char* TypeNames(int Type);
int TypeSize(int Type, struct SymbolTableEntry* Composite);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * *    S Y N T A X      T R E E     * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */


struct ASTNode* ConstructASTNode(int Operation, int Type,
                                 struct ASTNode* Left,
                                 struct ASTNode* Middle,
                                 struct ASTNode* Right,
                                 struct SymbolTableEntry* Symbol,
                                 int IntValue);

struct ASTNode* ConstructASTLeaf(int Operation, int Type, struct SymbolTableEntry* Symbol, int IntValue);

struct ASTNode* ConstructASTBranch(int Operation, int Type, struct ASTNode* Left, struct SymbolTableEntry* Symbol, int IntValue);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * *    P A R S I N G    * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct ASTNode* ParsePrecedenceASTNode(int PreviousTokenPrecedence);


struct ASTNode* ParsePrimary(void);
struct ASTNode* ParseStatement(void);
struct ASTNode* PrefixStatement();
struct ASTNode* PostfixStatement();

void ParseGlobals();

struct ASTNode* ParseFunction(int Type);
struct ASTNode* ParseCompound();

struct SymbolTableEntry* BeginStructDeclaration();
struct ASTNode* GetExpressionList();
struct ASTNode* CallFunction();
struct ASTNode* ReturnStatement();

int ParseOptionalPointer();
int ValueAt(int Type);
int PointerTo(int Type);

struct ASTNode* AccessArray();

int ParseTokenToOperation(int Token);

struct ASTNode* PrintStatement(void);



/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * *  S Y M B O L       T A B L E    * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct SymbolTableEntry* FindSymbol(char* Symbol);
struct SymbolTableEntry* FindLocal(char* Symbol);
struct SymbolTableEntry* FindGlobal(char* Symbol);
struct SymbolTableEntry* FindStruct(char* Symbol);
struct SymbolTableEntry* FindMember(char* Symbol);

void AppendSymbol(struct SymbolTableEntry** Head, struct SymbolTableEntry** Tail, struct SymbolTableEntry* Node);

void FreeLocals();
void ClearTables();

struct SymbolTableEntry* AddSymbol(char* Name, int Type, int Structure, int Storage, int Length, int SinkOffset, struct SymbolTableEntry* CompositeType);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * *    C O N T R O L       S T A T U S      * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void Die(char* Error);

void DieMessage(char* Error, char* Reason);

void DieDecimal(char* Error, int Number);

void DieChar(char* Error, int Char);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * *     C O D E     G E N E R A T I O N     * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int AssembleTree(struct ASTNode* Node, int Register, int ParentOp);

void DeallocateAllRegisters();

int RetrieveRegister();

void DeallocateRegister(int Register);

int PrimitiveSize(int Type);
int AsAlignMemory(int Type, int Offset, int Direction);

int AsLoad(int Value);
int AsAdd(int Left, int Right);
int AsMul(int Left, int Right);
int AsSub(int Left, int Right);
int AsDiv(int Left, int Right);

int AsLdGlobalVar(struct SymbolTableEntry* Entry, int Operation);
int AsLdLocalVar(struct SymbolTableEntry* Entry, int Operation);
int AsStrGlobalVar(struct SymbolTableEntry* Entry, int Register);
int AsStrLocalVar(struct SymbolTableEntry* Entry, int Register);

int AsCalcOffset(int Type);
void AsNewStackFrame();

int AsDeref(int Reg, int Type);
int AsStrDeref(int Register1, int Register2, int Type);
int AsAddr(struct SymbolTableEntry* Entry);

void AsGlobalSymbol(struct SymbolTableEntry* Entry);
int  AsNewString(char* Value);


int AsLoadString(int ID);

int AsEqual(int Left, int Right);
int AsIneq(int Left, int Right);
int AsLess(int Left, int Right);
int AsGreat(int Left, int Right);
int AsLessE(int Left, int Right);
int AsGreatE(int Left, int Right);

int AsBitwiseAND(int Left, int Right);
int AsBitwiseOR(int Left, int Right);
int AsBitwiseXOR(int Left, int Right);
int AsNegate(int Register);
int AsInvert(int Register);
int AsBooleanNOT(int Register);
int AsShiftLeft(int Left, int Right);
int AsShiftRight(int Left, int Right);
int AsBooleanConvert(int Register, int Operation, int Label);

int AsCompareJmp(int Operation, int RegisterLeft, int RegisterRight, int Label);
int AsCompare(int Operation, int RegisterLeft, int RegisterRight);
int AsIf(struct ASTNode* Node);
int NewLabel(void);

void AsJmp(int Label);
void AsLabel(int Label);

int AsShl(int Register, int Val);

int AsReturn(struct SymbolTableEntry* Entry, int Register);
int AsCallWrapper(struct ASTNode* Node);
void AsCopyArgs(int Register, int Position);
int AsCall(struct SymbolTableEntry* Entry, int Args);

int AsWhile(struct ASTNode* Node);

void AssemblerPrint(int Register);

void AssemblerPreamble();
void AsFunctionPreamble(struct SymbolTableEntry* Entry);
void AsFunctionEpilogue(struct SymbolTableEntry* Entry);


/* * * * * * * * * * * * * * * * * * * * * * *
 * * * *     D E C L A R A T I O N     * * * *
 * * * * * * * * * * * * * * * * * * * * * * */

struct SymbolTableEntry* BeginVariableDeclaration(int Type, struct SymbolTableEntry* Composite, int Scope);
struct ASTNode* ParseIdentifier(void);

struct ASTNode* IfStatement();
struct ASTNode* WhileStatement();
struct ASTNode* ForStatement();


void DumpTree(struct ASTNode* node, int level);