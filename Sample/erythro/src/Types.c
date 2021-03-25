
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>

/*
 * Returns whether the input Type represents a raw integer type.
 * It works on the principles outlined in Pointers.c; the lowest
 *  4 bits indicate indirection.
 * 
 * @param Type: The DataTypes representation to check
 * @return a boolean representing whether the input Type is a raw integer
 */

int TypeIsInt(int Type) {
    return ((Type & 0xf) == 0);
}

/*
 * Returns whether the input Type has at least one level of indirection.
 * It works on the principles outlined in Pointers.c; the lowest 4 bits
 *  indicate indirection.
 * 
 * @param Type: The DataTypes representation to check
 * @return a boolean representing whether the input Type is a pointer
 * 
 */

int TypeIsPtr(int Type) {
    return ((Type & 0xf) != 0);
}

/*
 * Turn a token type into its appropriate
 *  primitive type. 
 * 
 */

int PrimitiveSize(int Type) {
    
    if(TypeIsPtr(Type)) return 8;
    switch(Type) {
        case RET_CHAR: return 1;
        case RET_INT: return 4;
        case RET_LONG: return 8;
        default: 
            DieDecimal("Bad type in PrimitiveSize", Type);
    }
    return 0;
}

/*
 * Dynamically calculate the size of an object.
 * This was performed with an array previously, but the addition of
 *  structs and enums makes that irrelevant.
 * 
 */

int TypeSize(int Type, struct SymbolTableEntry* Composite) {
    if(Type == DAT_STRUCT) return Composite->Length;
    return PrimitiveSize(Type);
}

/*
 * A char buffer we can abuse for printing type names.
 * It needs to be 7 because that's 4 (long) + 3 (ptr), the longest
 *  possible name right now.
 */
static char TypeBuffer[7];

/*
 * Get the name of the input Type as a string.
 */
char* TypeNames(int Type) {
    switch(Type) {
        case RET_CHAR: memcpy(TypeBuffer, "Char", 4); break;
        case RET_INT: memcpy(TypeBuffer, "Int ", 4); break;
        case RET_LONG: memcpy(TypeBuffer, "Long", 4); break;
        case RET_VOID: memcpy(TypeBuffer, "Void", 4); break;
        default: DieDecimal("Bad size for printing", Type);
    };
    if(TypeIsPtr(Type)) memcpy((void*)((size_t) TypeBuffer + 4), "Ptr", 3);
    else memcpy((void*)((size_t) TypeBuffer + 4), "   ", 3);
    return TypeBuffer;
    
}

/*
 * Determine if two types are compatible.
 * A left char and a right int are compatible, as the char will fit into the int.
 *  The left char will need to be widened for assignment, however.
 * 
 * If strict is set, you can only widen the Left to the Right.
 * If strict is false, any widening is valid.
 * 
 */

int TypesCompatible(int* Left, int* Right, int STRICT) {

    int LeftSize, RightSize;

    // Same types are compatible. No shrinking required
    if(*Left == *Right) {
        *Left = *Right = 0;
        return 1;
    }

    LeftSize = PrimitiveSize(*Left);
    RightSize = PrimitiveSize(*Right);


    // Types of size 0 are incompatible
    if((LeftSize == 0) || (RightSize == 0))
        return 0;


    /*  char x;
     *  int y;
     *  y = 15;
     * 
     *  x = y;
     *      x needs to be widened, y copied in, then x shrunk back down
     *      AKA, the left must be widened.
     */
    if(LeftSize < RightSize) {
        *Left = OP_WIDEN;
        *Right = 0;
        return 1;
    }

    /*
     * char x;
     * int y;
     * 
     * x = 15;
     * 
     * y = x;
     *      x must be widened to fit into y.
     *      if STRICT mode, this is not allowed.
     *      By default, this is valid.
     * 
     */

    if(LeftSize > RightSize) {
        if(STRICT)
            return 0; // Not compatible if STRICT
        
        *Left = 0;
        *Right = OP_WIDEN;
        return 1; // Compatible by default
    }

    /*
     * Any other cases left, by default, are compatible.
     * 
     */

    *Left = *Right = 0;
    return 1;

}


/**
 * Given an operation on two types, we need to be able to
 *  determine if the operation is valid for both types,
 *  as well as modify the types if the operation is 
 *   theoretically valid but requires some changes.
 *  
 * An example of the latter is assigning an int literal into
 *  a char, or squeezing down the int into the char type.
 * 
 * If the operation is not valid, this will return NULL.
 * If the operaton is valid without changes, this will return Tree.
 * If the operation is valid with changes, this will perform
 *  the changes and return the new tree.
 * 
 * This also serves to consolidate some of the gross widening
 *  code that TypesCompatible led us to.
 */

struct ASTNode* MutateType(struct ASTNode* Tree, int RightType, int Operation) {
    int LeftType;
    int LeftSize, RightSize;

    LeftType = Tree->ExprType;


    printf("\tCalculating compatibility between ltype %d and rtype %d\r\n", LeftType, RightType);
    if(TypeIsInt(LeftType) && TypeIsInt(RightType)) {

        // Short-circuit for valid types
        if(LeftType == RightType) {
            return Tree;
        }

        LeftSize = PrimitiveSize(LeftType);
        RightSize = PrimitiveSize(RightType);

        /**
         * LeftSize > RightSize:
         *  char x = 15000;
         * 
         * (The left branch of the tree contains the current AST)
         * 
         */
        if(LeftSize > RightSize)
            return NULL;

        /**
         * RightSize > LeftSize:
         *  char x = 5;
         *  int y = x;
         * 
         * We have to widen x into an int in order for this to be compatible
         *  BUT it is possible!
         */

        if(RightSize > LeftSize)
            return ConstructASTBranch(OP_WIDEN, RightType, Tree, NULL, 0);
    }

    // Left branch pointers are compatible if we're not doing operations
    if(TypeIsPtr(LeftType)) {
        if(Operation == 0 && LeftType == RightType)
            return Tree;
    }


    // Otherwise, we can perform some scaling for pointer addition & subtraction
    if(Operation == OP_ADD || Operation == OP_SUBTRACT) {

        /**
         * Left int, right pointer:
         *  int x = 5;
         *  int* y;
         * 
         *  x = *(y + 1);
         */

        if(TypeIsInt(LeftType) && TypeIsPtr(RightType)) {
            printf("\t\t\tMutateType: Right node needs adjustment\r\n");
            RightSize = PrimitiveSize(ValueAt(RightType));

            if(RightSize > 1)
                return ConstructASTBranch(OP_SCALE, RightType, Tree, NULL, RightSize);
        }
    }

    // If all else fails, we've constructed a combination of types that are not compatible.
    // ie. left or right is a void.
    // You cannot do pointer arithmetic on void type.
    return NULL;
}
