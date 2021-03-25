
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>

/****************************************************************
 * Types are enumerated by the DataTypes enum.                  *
 * They are represented by unsigned integers, where the         *
 *  most significant 28 bits differentiate the raw type         *
 *   of the data being encoded.                                 *
 * However, the least significant nibble - that is,             *
 *  the lowest 4 bits, represent the count of indirection.      *
 *                                                              *
 * This means that a raw Integer data type, such as an i32,     *
 *  has the DataType representation 32.                         *
 * However, a pointer to an Integer has DataType value 32+1,    *
 *  or 33.                                                      *
 *                                                              *
 * This means that the maximum valid pointer level is 16.       *
 *  That's a:                                                   *
 *  ****************int                                         *
 * That ought to be enough for everyone, right?                 *
 *                                                              *
 ****************************************************************/

/*
 * Adds 1 to the input Type, to add a level of indirection.
 * If the indirection is already at 16 levels, it aborts.
 * 
 * @param Type: The DataType to pointerise
 * @return the new pointerised DataType value.
 */

int PointerTo(int Type) {
    if((Type & 0xf) == 0xf)
        DieDecimal("Unrecognized type in pointerisation", Type);
    printf("\t\tPointerising a %s\n", TypeNames(Type));
    return (Type + 1);
}

/*
 * Returns the underlying type behind a pointer.
 * If the type is not a pointer (the lowest 4 bits are 0), it halts compliation.
 * 
 * @param Type: The type to un-dereference
 * @return the underlying Type
 */

int ValueAt(int Type) {
    printf("\t\tDereferencing a %s\n", TypeNames(Type));
    if((Type & 0xf) == 0x0)
        DieDecimal("Unrecognized type in defererencing", Type);
    return (Type - 1);
}

/*
 * Type declarations may be raw, they may be pointers.
 * If they are pointers, we need to be able to check
 *  how many levels of indirection.
 * However, being a pointer is optional.
 * 
 * This can parase in just a lone type specifier, or 
 *  any valid level of indirection therefore.
 * 
 * @param Composite: unused
 * @return the parsed DataType, with any indirection.
 * 
 */

int ParseOptionalPointer(struct SymbolTableEntry** Composite) {

    int Type;
    
    switch(CurrentToken.type) {
        case TY_VOID:
            Type = RET_VOID;
            Tokenise();
            break;
        case TY_CHAR:
            Type = RET_CHAR;
            Tokenise();
            break;
        case TY_INT:
            Type = RET_INT;
            Tokenise();
            break;
        case TY_LONG:
            Type = RET_LONG;
            Tokenise();
            break;
        case KW_STRUCT:
            Type = DAT_STRUCT;
            *Composite = BeginStructDeclaration();
            break;
        default: 
            DieDecimal("Illegal type for pointerisation", CurrentToken.type);
    }
    // Recursively scan more *s
    // This makes things like:
    // x = **y;
    // possible.
    while(1) {
        printf("\t\t\tType on parsing is %d\n", CurrentToken.type);
        if(CurrentToken.type != AR_STAR)
            break;
        
        Type = PointerTo(Type);
        Tokenise();
        // Tokenise(); TODO: is this skipping pointers?
    }

    return Type;
}


/*
 * Array Accesses come in the form of x[y].
 * 
 * x must be a pointer type, and an array structure.
 * y can be any binary expression.
 * 
 * It is a wrapper around *((imax*)x + y).
 * 
 * @return the AST Node that represents this statement.
 */

struct ASTNode* AccessArray() {
    struct ASTNode* LeftNode, *RightNode;
    struct SymbolTableEntry* Entry;

    printf("\tAccessing array %s as requested\r\n", CurrentIdentifier);
    if ((Entry = FindSymbol(CurrentIdentifier)) == NULL || Entry->Structure != ST_ARR)
        DieMessage("Accessing undeclared array", CurrentIdentifier);
    
    LeftNode = ConstructASTLeaf(OP_ADDRESS, Entry->Type, Entry, 0);
    Tokenise();

    RightNode = ParsePrecedenceASTNode(0);

    VerifyToken(LI_RBRAS, "]");

    if(!TypeIsInt(RightNode->ExprType))
        Die("Array index is not integer");
    
    printf("\t\tPreparing types - RightNode of type %s must be mutated to LeftNode type %s\r\n", (RightNode->ExprType), TypeNames(LeftNode->ExprType));
    RightNode = MutateType(RightNode, LeftNode->ExprType, OP_ADD);

    LeftNode = ConstructASTNode(OP_ADD, Entry->Type, LeftNode, NULL, RightNode, NULL, 0);
    printf("\tAccessArray: Preparing LeftNode for dereference.\r\n");
    LeftNode = ConstructASTBranch(OP_DEREF, ValueAt(LeftNode->ExprType), LeftNode, NULL, 0);
    printf("\tArray Access constructed\r\n");
    return LeftNode;
}