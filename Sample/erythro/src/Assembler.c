
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>


/*
 * Stores how many hardware registers are being used at any one time.
 * It is empirically proven that only 4 clobber registers are
 *  needed for any arbitrary length program.
 * 
 * If UsedRegisters[i] =? 1, then Registers[i] contains useful data.
 * If UsedRegisters[i] =? 0, then Registers[i] is unused.
 * 
 */

static int UsedRegisters[4];

/* The https://en.wikipedia.org/wiki/X86_calling_conventions#Microsoft_x64_calling_convention
 *  calling convention on Windows requires that
 *   the last 4 arguments are placed in registers
 *   rcx, rdx, r8 and r9.
 * This order must be preserved, and they must be placed
 *  right to left.
 * 
 * The 4 clobber registers are first, and the 4 parameter registers are last.
 */
static char* Registers[8]       = { "%r10",  "%r11" , "%r12" , "%r13",  "%r9" , "%r8",  "%rdx", "%rcx" };
static char* DoubleRegisters[8] = { "%r10d", "%r11d", "%r12d", "%r13d", "%r9d", "%r8d", "%edx", "%ecx" };
static char* ByteRegisters[8]   = { "%r10b", "%r11b", "%r12b", "%r13b", "%r9b", "%r8b", "%dl" , "%cl"  };

/*
 * For ease of reading later code, we store the valid x86 comparison instructions,
 *  and the inverse jump instructions together, in a synchronized fashion.
 */

static char* Comparisons[6]     = { "sete", "setne", "setl", "setg", "setle", "setge" };
static char* InvComparisons[6]  = { "jne", "je",     "jge",  "jle",  "jg",    "jl"};

// How far above the base pointer is the last local?
static int LocalVarOffset;
// How far must we lower the base pointer to retrieve the parameters?
static int StackFrameOffset;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * *   R O O T    O F    A S S E M B L E R   * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Just a short "hack" to make sure we only dump the tree the first time this function is called
static int Started = 0;

/*
 * Walk the AST tree given, and generate the assembly code that represents
 *  it.
 * 
 * @param Node: The current Node to compile. If needed, its children will be parsed recursively.
 * @param Register: The index of Registers to store the result of the current compilation.
 * @param ParentOp: The Operation of the parent of the current Node.
 * 
 * @return dependant on the Node. Typically the Register that stores the result of the Node's operation.
 * 
 */
int AssembleTree(struct ASTNode* Node, int Register, int ParentOp) {
    int LeftVal, RightVal;
    if(!Started && OptDumpTree)
        DumpTree(Node, 0);
    Started = 1;

    printf("Current operation: %d\r\n", Node->Operation);
    switch(Node->Operation) {
        case OP_IF:
            return AsIf(Node);
        
        case OP_LOOP:
            return AsWhile(Node);

        case OP_COMP:
            AssembleTree(Node->Left, -1, Node->Operation);
            DeallocateAllRegisters();
            AssembleTree(Node->Right, -1, Node->Operation);
            DeallocateAllRegisters();
            return -1;

        case OP_CALL:
            return (AsCallWrapper(Node));

        case OP_FUNC:
            AsFunctionPreamble(Node->Symbol);
            AssembleTree(Node->Left, -1, Node->Operation);
            AsFunctionEpilogue(Node->Symbol);
            return -1;
    }


    if(Node->Left)
        LeftVal = AssembleTree(Node->Left, -1, Node->Operation);
    
    if(Node->Right)
        RightVal = AssembleTree(Node->Right, LeftVal, Node->Operation);

    switch(Node->Operation) {
        case OP_ADD:
            return AsAdd(LeftVal, RightVal);

        case OP_SUBTRACT:
            return AsSub(LeftVal, RightVal);

        case OP_MULTIPLY:
            return AsMul(LeftVal, RightVal);
            
        case OP_DIVIDE:
            return AsDiv(LeftVal, RightVal);
            
        case OP_SCALE:
            // We can (ab)use the powers of 2 to do
            // efficient scaling with bitshifting.
            switch(Node->Size) {
                case 2: return AsShl(LeftVal, 1);
                case 4: return AsShl(LeftVal, 2);
                case 8: return AsShl(LeftVal, 3);
                
                default:
                    RightVal = AsLoad(Node->Size);
                    return AsMul(LeftVal, RightVal);
            }
        case OP_ADDRESS:
            return AsAddr(Node->Symbol);

        case OP_DEREF:
            return Node->RVal ? AsDeref(LeftVal, Node->Left->ExprType) : LeftVal;

        case OP_ASSIGN:
            printf("Preparing for assignment..\r\n");
            if(Node->Right == NULL)
                Die("Fault in assigning a null rvalue");
            
            printf("\tCalculating assignment for target %s:\r\n", Node->Right->Symbol->Name);
            switch(Node->Right->Operation) {
                case REF_IDENT: 
                    if(Node->Right->Symbol->Storage == SC_LOCAL)
                        return AsStrLocalVar(Node->Right->Symbol, LeftVal);
                    else 
                        return AsStrGlobalVar(Node->Right->Symbol, LeftVal);

                case OP_DEREF: return AsStrDeref(LeftVal, RightVal, Node->Right->ExprType);
                default: DieDecimal("Can't ASSIGN in AssembleTree: ", Node->Operation);
            }

        case OP_WIDEN:
            printf("\tWidening types..\r\n");
            return LeftVal;
            
        case OP_RET:
            printf("\tReturning from %s\n", Node->Symbol->Name);
            AsReturn(FunctionEntry, LeftVal);
            return -1;

        case OP_EQUAL:
        case OP_INEQ:
        case OP_LESS:
        case OP_GREAT:
        case OP_LESSE:
        case OP_GREATE:
            if(ParentOp == OP_IF || ParentOp == OP_LOOP)
                return AsCompareJmp(Node->Operation, LeftVal, RightVal, Register);
            else
                return AsCompare(Node->Operation, LeftVal, RightVal);
            

        case REF_IDENT:
            if(Node->RVal || ParentOp == OP_DEREF) {
                if(Node->Symbol->Storage == SC_LOCAL || Node->Symbol->Storage == SC_PARAM)
                    return AsLdLocalVar(Node->Symbol, Node->Operation);
                else 
                    return AsLdGlobalVar(Node->Symbol, Node->Operation);
            } else 
                return -1;

        case TERM_INTLITERAL:
            return AsLoad(Node->IntValue);
        
        case TERM_STRLITERAL:
            return AsLoadString(Node->IntValue);

        case OP_PRINT:
            AssemblerPrint(LeftVal);
            DeallocateAllRegisters();
            return -1;

        case OP_BITAND:
            return AsBitwiseAND(LeftVal, RightVal);
        
        case OP_BITOR:
            return AsBitwiseOR(LeftVal, RightVal);
        
        case OP_BITXOR:
            return AsBitwiseXOR(LeftVal, RightVal);

        case OP_SHIFTL:
            return AsShiftLeft(LeftVal, RightVal);
        
        case OP_SHIFTR:
            return AsShiftRight(LeftVal, RightVal);
        
        case OP_POSTINC:
            return AsLdGlobalVar(Node->Symbol, Node->Operation);
        
        case OP_POSTDEC:
            return AsLdGlobalVar(Node->Symbol, Node->Operation);
        
        case OP_PREINC:
            return AsLdGlobalVar(Node->Symbol, Node->Operation);
        
        case OP_PREDEC:
            return AsLdGlobalVar(Node->Symbol, Node->Operation);
        
        case OP_BOOLNOT:
            return AsBooleanNOT(LeftVal);
        
        case OP_BITNOT:
            return AsInvert(LeftVal);

        case OP_NEGATE:
            return AsNegate(LeftVal);
        
        case OP_BOOLCONV:
            return AsBooleanConvert(LeftVal, ParentOp, Register);

        default:
            DieDecimal("Unknown ASM Operation", Node->Operation);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * *     R E G I S T E R     M A N A G E M E N T     * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Set all Registers to unused.
void DeallocateAllRegisters() {
    UsedRegisters[0] = UsedRegisters[1] = UsedRegisters[2] = UsedRegisters[3] = 0;
}

/*
 * Search for an unused register, allocate it, and return it.
 * If none available, cancel compilation.
 */
int RetrieveRegister() {
    for (size_t i = 0; i < 4; i++) {
        if(UsedRegisters[i] == 0) {
            UsedRegisters[i] = 1;
            return i;
        }
    }
    fprintf(stderr, "Out of registers!\n");
    exit(1);
}

/*
 * Set the given register to unused.
 * If the register is not used, it is an invalid state.
 * @param Register: The Registers index to deallocate.
 */
void DeallocateRegister(int Register) {
    if(UsedRegisters[Register] != 1) {
        fprintf(stderr, "Error trying to free register %d\n", Register);
        exit(1);
    }

    UsedRegisters[Register] = 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * *    S T A C K     M A N A G E M E N T    * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Prepare a new stack frame pointer.
 * This resets the highest local.
 * 
 */
void AsNewStackFrame() {
    LocalVarOffset = 0;
}

/*
 * Given the type of input, how far do we need to go down the stack frame
 *  to store or retrieve this type?
 * 
 * The stack must be 4-bytes aligned, so we set a hard minimum.
 * 
 * @param Type: The DataTypes we want to store.
 * @return the offset to store the type, taking into account the current state of the stack frame.
 * 
 */
int AsCalcOffset(int Type) {
    LocalVarOffset += PrimitiveSize(Type) > 4 ? PrimitiveSize(Type) : 4;
    return -LocalVarOffset;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * *     C O D E     G E N E R A T I O N     * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * A way to keep track of the largest label number.
 * Call this function to increase the number SRG-like.
 * 
 * @return the highest available label number
 * 
 */
int NewLabel(void) {
    static int id = 1;
    return id++;
}

/*
 * Align non-char types to a 4 byte alignment.
 * Chars need no alignment on x86_64.
 * 
 * @param Type: The DataTypes representation of the data to align
 * @param Offset: The offset to align
 * @param Direction: The desired direction to move the address for alignment. 1 = up, -1 = down.
 * @return the new alignment
 * 
 */
int AsAlignMemory(int Type, int Offset, int Direction) {
    switch(Type) {
        case RET_CHAR: return Offset;
        case RET_INT: case RET_LONG: break;
        default: 
          DieDecimal("Unable to align type", Type);
    }

    int Alignment = 4;
    Offset = (Offset + Direction * (Alignment-1)) & ~(Alignment-1);
    return (Offset);
}
 
// Assemble an If statement
int AsIf(struct ASTNode* Node) {
    int FalseLabel, EndLabel;

    FalseLabel = NewLabel();
    if(Node->Right)
        EndLabel = NewLabel();

    
    // Left is the condition
    AssembleTree(Node->Left, FalseLabel, Node->Operation);
    DeallocateAllRegisters();

    // Middle is the true block
    AssembleTree(Node->Middle, -1, Node->Operation);
    DeallocateAllRegisters();

    // Right is the optional else
    if(Node->Right)
        AsJmp(EndLabel);
    
    AsLabel(FalseLabel);

    if(Node->Right) {
        AssembleTree(Node->Right, -1, Node->Operation);
        DeallocateAllRegisters();
        AsLabel(EndLabel);
    }

    return -1;
}

// Assemble a comparison
int AsCompare(int Operation, int RegisterLeft, int RegisterRight) {
    printf("Comparing registers %d & %d\n", RegisterLeft, RegisterRight);

    if(Operation < OP_EQUAL || Operation > OP_GREATE)
        Die("Bad Operation in AsCompare");
    
    fprintf(OutputFile, "\tcmpq\t%s, %s\n", Registers[RegisterRight], Registers[RegisterLeft]);
    fprintf(OutputFile, "\t%s\t\t%s\n", Comparisons[Operation - OP_EQUAL], ByteRegisters[RegisterRight]);
    fprintf(OutputFile, "\tmovzbq\t%s, %s\n", ByteRegisters[RegisterRight], Registers[RegisterLeft]);
    DeallocateRegister(RegisterLeft);
    return RegisterRight;
}

// Assemble an inverse comparison (a one-line jump)
int AsCompareJmp(int Operation, int RegisterLeft, int RegisterRight, int Label) {
    if(Operation < OP_EQUAL || Operation > OP_GREATE)
        Die("Bad Operation in AsCompareJmp");

    printf("\tBranching on comparison of registers %d & %d, with operation %s\n\n", RegisterLeft, RegisterRight, Comparisons[Operation - OP_EQUAL]);
    
    fprintf(OutputFile, "\tcmpq\t%s, %s\n", Registers[RegisterRight], Registers[RegisterLeft]);
    fprintf(OutputFile, "\t%s\tL%d\n", InvComparisons[Operation - OP_EQUAL], Label);
    DeallocateAllRegisters();

    return -1;
}

// Assemble an immediate jump
void AsJmp(int Label) {
    printf("\t\tJumping to label %d\n", Label);
    fprintf(OutputFile, "\tjmp\tL%d\n", Label);
}

/* Create a new base label
 * @param Label: The number to create the label of 
 */
void AsLabel(int Label) {
    printf("\tCreating label %d\n", Label);
    fprintf(OutputFile, "\nL%d:\n", Label);
}

/*
 * Assemble a new global string into the data segment.
 * @param Value: The name of the string, as a string
 */
int AsNewString(char* Value) {
    int Label = NewLabel();
    char* CharPtr;

    AsLabel(Label);

    for(CharPtr = Value; *CharPtr; CharPtr++) 
        fprintf(OutputFile, "\t.byte\t%d\r\n", *CharPtr);
    fprintf(OutputFile, "\t.byte\t0\r\n");
    
    return Label;
}

/*
 * Load a string into a Register.
 * @param ID: the Label number of the string
 */ 
int AsLoadString(int ID) {
    int Register = RetrieveRegister();
    fprintf(OutputFile, "\tleaq\tL%d(\%%rip), %s\r\n", ID, Registers[Register]);
    return Register;
}

// Assemble a While loop
int AsWhile(struct ASTNode* Node) {
    int BodyLabel, BreakLabel;
    
    BodyLabel = NewLabel();
    BreakLabel = NewLabel();

    printf("\tInitiating loop between labels %d and %d\n", BodyLabel, BreakLabel);

    // Mark the start position
    AsLabel(BodyLabel);

    // Assemble the condition - this should include a jump to end!
    AssembleTree(Node->Left, BreakLabel, Node->Operation);
    DeallocateAllRegisters();

    // Assemble the body
    AssembleTree(Node->Right, -1, Node->Operation);
    DeallocateAllRegisters();

    // Jump back to the body - as we've already failed the condition check if we get here
    AsJmp(BodyLabel);

    // Set up the label to break out of the loop.
    AsLabel(BreakLabel);


    return -1;

}

// Load a value into a register.
int AsLoad(int Value) {
    int Register = RetrieveRegister();

    printf("\tStoring value %d into %s\n", Value, Registers[Register]);

    fprintf(OutputFile, "\tmovq\t$%d, %s\n", Value, Registers[Register]);

    return Register;
}

// Assemble an addition.
int AsAdd(int Left, int Right) {
    printf("\tAdding Registers %s, %s\n", Registers[Left], Registers[Right]);
    fprintf(OutputFile, "\taddq\t%s, %s\n", Registers[Left], Registers[Right]);

    DeallocateRegister(Left);

    return Right;
}

// Assemble a multiplication.
int AsMul(int Left, int Right) {
    printf("\tMultiplying Registers %s, %s\n", Registers[Left], Registers[Right]);
    fprintf(OutputFile, "\timulq\t%s, %s\n", Registers[Left], Registers[Right]);

    DeallocateRegister(Left);
    
    return Right;
}

// Assemble a subtraction.
int AsSub(int Left, int Right) {
    printf("\tSubtracting Registers %s, %s\n", Registers[Left], Registers[Right]);
    fprintf(OutputFile, "\tsubq\t%s, %s\n", Registers[Right], Registers[Left]);

    DeallocateRegister(Right);

    return Left;
}

// Assemble a division.
int AsDiv(int Left, int Right) {
    printf("\tDividing Registers %s, %s\n", Registers[Left], Registers[Right]);
    fprintf(OutputFile, "\tmovq\t%s, %%rax\n", Registers[Left]);
    fprintf(OutputFile, "\tcqo\n");
    fprintf(OutputFile, "\tidivq\t%s\n", Registers[Right]);
    fprintf(OutputFile, "\tmovq\t%%rax, %s\n", Registers[Left]);

    DeallocateRegister(Right);

    return Left;
}

// Assemble an ASL
int AsShl(int Register, int Val) {
    printf("\tShifting %s to the left by %d bits.\n", Registers[Register], Val);
    fprintf(OutputFile, "\tsalq\t$%d, %s\n", Val, Registers[Register]);
    return Register;
}

/*
 * Load a global variable into a register, with optional pre/post-inc/dec
 * @param Entry: The variable to load.
 * @param Operation: An optional SyntaxOps element
 */ 
int AsLdGlobalVar(struct SymbolTableEntry* Entry, int Operation) {
    int Reg = RetrieveRegister();

    printf("\tStoring %s's contents into %s, globally\n", Entry->Name, Registers[Reg]);

    int TypeSize = PrimitiveSize(Entry->Type);
    switch(TypeSize) {
        case 1:
            switch(Operation) {
                case OP_PREINC:
                    fprintf(OutputFile, "\tincb\t%s(\%%rip)\n", Entry->Name); break;
                case OP_PREDEC:
                    fprintf(OutputFile, "\tdecb\t%s(\%%rip)\n", Entry->Name); break;
            }

            fprintf(OutputFile, "\tmovzbq\t%s(\%%rip), %s\n", Entry->Name, Registers[Reg]);

            switch(Operation) {
                case OP_POSTINC:
                    fprintf(OutputFile, "\tincb\t%s(\%%rip)\n", Entry->Name); break;
                case OP_POSTDEC:
                    fprintf(OutputFile, "\tdecb\t%s(\%%rip)\n", Entry->Name); break;
            }

            break;
        
        case 4:
            switch(Operation) {
                case OP_PREINC:
                    fprintf(OutputFile, "\tincl\t%s(\%%rip)\n", Entry->Name); break;
                case OP_PREDEC:
                    fprintf(OutputFile, "\tdecl\t%s(\%%rip)\n", Entry->Name); break;
            }

            fprintf(OutputFile, "\tmovslq\t%s(\%%rip), %s\n", Entry->Name, Registers[Reg]);

            switch(Operation) {
                case OP_POSTINC:
                    fprintf(OutputFile, "\tincl\t%s(\%%rip)\n", Entry->Name); break;
                case OP_POSTDEC:
                    fprintf(OutputFile, "\tdecl\t%s(\%%rip)\n", Entry->Name); break;
            }

            break; 
        case 8:
            switch(Operation) {
                case OP_PREINC:
                    fprintf(OutputFile, "\tincq\t%s(\%%rip)\n", Entry->Name); break;
                case OP_PREDEC:
                    fprintf(OutputFile, "\tdecq\t%s(\%%rip)\n", Entry->Name); break;
            }

            fprintf(OutputFile, "\tmovq\t%s(\%%rip), %s\n", Entry->Name, Registers[Reg]);

            switch(Operation) {
                case OP_POSTINC:
                    fprintf(OutputFile, "\tincq\t%s(\%%rip)\n", Entry->Name); break;
                case OP_POSTDEC:
                    fprintf(OutputFile, "\tdecq\t%s(\%%rip)\n", Entry->Name); break;
            }

            break;

        default:
            DieMessage("Bad type for loading", TypeNames(Entry->Type));
    }

    return Reg;
}

/* 
 * Store a value from a register into a global variable.
 * @param Entry: The variable to store into.
 * @param Regsiter: The Registers index containing the value to store.
 */
int AsStrGlobalVar(struct SymbolTableEntry* Entry, int Register) {
    printf("\tStoring contents of %s into %s, type %d, globally:\n", Registers[Register], Entry->Name, Entry->Type);

    int TypeSize = PrimitiveSize(Entry->Type);
    switch(TypeSize) {
        case 1:
            // movzbq zeroes, then moves a byte into the quad register
            fprintf(OutputFile, "\tmovb\t%s, %s(\%%rip)\n", ByteRegisters[Register], Entry->Name);
            break;
        
        case 4:
            fprintf(OutputFile, "\tmovl\t%s, %s(\%%rip)\n", DoubleRegisters[Register], Entry->Name);
            break;
        
        case 8:
            fprintf(OutputFile, "\tmovq\t%s, %s(%%rip)\n", Registers[Register], Entry->Name);
            break;

        default:
            DieMessage("Bad type for saving", TypeNames(Entry->Type));
    }
    
    return Register;
}

/*
 * Load a value from a local variable into a register, with optional post/pre-inc/dec
 * @param Entry: The local variable to read
 * @param Operation: An optional SyntaxOps entry
 */

int AsLdLocalVar(struct SymbolTableEntry* Entry, int Operation) {
    int Reg = RetrieveRegister();

    printf("\tStoring the var at %d's contents into %s, locally\n", Entry->SinkOffset, Registers[Reg]);
    
    int TypeSize = PrimitiveSize(Entry->Type);
    switch(TypeSize) {
        case 1:
            switch(Operation) {
                case OP_PREINC:
                    fprintf(OutputFile, "\tincb\t%d(\%%rbp)\n", Entry->SinkOffset); break;
                case OP_PREDEC:
                    fprintf(OutputFile, "\tdecb\t%d(\%%rbp)\n", Entry->SinkOffset); break;
            }

            fprintf(OutputFile, "\tmovzbq\t%d(\%%rbp), %s\n", Entry->SinkOffset, Registers[Reg]);

            switch(Operation) {
                case OP_POSTINC:
                    fprintf(OutputFile, "\tincb\t%d(\%%rbp)\n", Entry->SinkOffset); break;
                case OP_POSTDEC:
                    fprintf(OutputFile, "\tdecb\t%d(\%%rbp)\n", Entry->SinkOffset); break;
            }

            break;
        
        case 4:
            switch(Operation) {
                case OP_PREINC:
                    fprintf(OutputFile, "\tincl\t%d(\%%rbp)\n", Entry->SinkOffset); break;
                case OP_PREDEC:
                    fprintf(OutputFile, "\tdecl\t%d(\%%rbp)\n", Entry->SinkOffset); break;
            }

            fprintf(OutputFile, "\tmovslq\t%d(\%%rbp), %s\n", Entry->SinkOffset, Registers[Reg]);

            switch(Operation) {
                case OP_POSTINC:
                    fprintf(OutputFile, "\tincl\t%d(\%%rbp)\n", Entry->SinkOffset); break;
                case OP_POSTDEC:
                    fprintf(OutputFile, "\tdecl\t%d(\%%rbp)\n", Entry->SinkOffset); break;
            }

            break; 
        case 8:
            switch(Operation) {
                case OP_PREINC:
                    fprintf(OutputFile, "\tincq\t%d(\%%rbp)\n", Entry->SinkOffset); break;
                case OP_PREDEC:
                    fprintf(OutputFile, "\tdecq\t%d(\%%rbp)\n", Entry->SinkOffset); break;
            }

            fprintf(OutputFile, "\tmovq\t%d(\%%rbp), %s\n", Entry->SinkOffset, Registers[Reg]);

            switch(Operation) {
                case OP_POSTINC:
                    fprintf(OutputFile, "\tincq\t%d(\%%rbp)\n", Entry->SinkOffset); break;
                case OP_POSTDEC:
                    fprintf(OutputFile, "\tdecq\t%d(\%%rbp)\n", Entry->SinkOffset); break;
            }

            break;

        default:
            DieMessage("Bad type for loading", TypeNames(Entry->Type));
    }

    return Reg;
}
 
/*
 * Store a value from a register into a local variable.
 * @param Entry: The local variable to write to.
 * @param Register: The Registers index containing the desired value
 * 
 */
int AsStrLocalVar(struct SymbolTableEntry* Entry, int Register) {
    printf("\tStoring contents of %s into %s, type %d, locally\n", Registers[Register], Entry->Name, Entry->Type);

    int TypeSize = PrimitiveSize(Entry->Type);
    switch(TypeSize) {
        case 1:
            // movzbq zeroes, then moves a byte into the quad register
            fprintf(OutputFile, "\tmovb\t%s, %d(\%%rbp)\n", ByteRegisters[Register], Entry->SinkOffset);
            break;
        
        case 4:
            fprintf(OutputFile, "\tmovl\t%s, %d(\%%rbp)\n", DoubleRegisters[Register], Entry->SinkOffset);
            break;
        
        case 8:
            fprintf(OutputFile, "\tmovq\t%s, %d(%%rbp)\n", Registers[Register], Entry->SinkOffset);
            break;

        default:
            DieMessage("Bad type for saving", TypeNames(Entry->Type));
    }
    
    return Register;
}

// Assemble a pointerisation
int AsAddr(struct SymbolTableEntry* Entry) {
    int Register = RetrieveRegister();
    printf("\tSaving pointer of %s into %s\n", Entry->Name, Registers[Register]);

    fprintf(OutputFile, "\tleaq\t%s(%%rip), %s\n", Entry->Name, Registers[Register]);
    return Register;
}

// Assemble a dereference
int AsDeref(int Reg, int Type) {

    int DestSize = PrimitiveSize(ValueAt(Type));

    printf("\tDereferencing %s\n", Registers[Reg]);
    switch(DestSize) {
        case 1:
            fprintf(OutputFile, "\tmovzbq\t(%s), %s\n", Registers[Reg], Registers[Reg]);
            break;
        case 2:
            fprintf(OutputFile, "\tmovslq\t(%s), %s\n", Registers[Reg], Registers[Reg]);
        case 4:
        case 8:
            fprintf(OutputFile, "\tmovq\t(%s), %s\n", Registers[Reg], Registers[Reg]);
            break;
        default: 
            DieDecimal("Can't generate dereference for type", Type);
    }

    return Reg;
}

// Assemble a store-through-dereference
int AsStrDeref(int Register1, int Register2, int Type) {
    printf("\tStoring contents of %s into %s through a dereference, type %d\n", Registers[Register1], Registers[Register2], Type);

    switch(Type) {
        case RET_CHAR:
            fprintf(OutputFile, "\tmovb\t%s, (%s)\n", ByteRegisters[Register1], Registers[Register2]);
            break;
        case RET_INT:
        case RET_LONG:
            fprintf(OutputFile, "\tmovq\t%s, (%s)\n", Registers[Register1], Registers[Register2]);
            break;
        default:
            DieDecimal("Can't generate store-into-deref of type", Type);
    }

    return Register1;
}

// Assemble a global symbol (variable, struct, enum, function, string)
void AsGlobalSymbol(struct SymbolTableEntry* Entry) {

    if(Entry == NULL) return;
    if(Entry->Structure == ST_FUNC) return;


    int Size = TypeSize(Entry->Type, Entry->CompositeType);

    fprintf(OutputFile, "\t.data\n"
                        "\t.globl\t%s\n",
                                          Entry->Name);

    fprintf(OutputFile, "%s:\n", Entry->Name);
    
    switch(Size) {
        case 1: fprintf(OutputFile, "\t.byte\t0\r\n", Entry->Name); break;
        case 4: fprintf(OutputFile, "\t.long\t0\r\n", Entry->Name); break;
        case 8: fprintf(OutputFile, "\t.quad\t0\r\n", Entry->Name); break;
        default:
            for(int i = 0; i < Size; i++)
                fprintf(OutputFile, "\t.byte\t0\n");
    }
    
}

// Assemble a function call, with all associated parameter bumping and stack movement.
int AsCallWrapper(struct ASTNode* Node) {
    struct ASTNode* CompositeTree = Node->Left;
    int Register, Args = 0;

    while(CompositeTree) {
        Register = AssembleTree(CompositeTree->Right, -1, CompositeTree->Operation);
        AsCopyArgs(Register, CompositeTree->Size);
        if(Args == 0) Args = CompositeTree->Size;
        DeallocateAllRegisters();
        CompositeTree = CompositeTree->Left;
    }

    return AsCall(Node->Symbol, Args);
}

// Copy a function argument from Register to argument Position
void AsCopyArgs(int Register, int Position) {
    if(Position > 4) { // Args above 4 go on the stack
        fprintf(OutputFile, "\tpushq\t%s\n", Registers[Register]);
    } else {
        fprintf(OutputFile, "\tmovq\t%s, %s\n", Registers[Register], Registers[10 - Position]);
    }
}

// Assemble an actual function call.
// NOTE: this should not be called. Use AsCallWrapper.
int AsCall(struct SymbolTableEntry* Entry, int Args) {

    int OutRegister = RetrieveRegister();

    printf("\t\tCalling function %s with %d parameters\n", Entry->Name, Args);
    printf("\t\t\tFunction returns into %s\n", Registers[OutRegister]);

    fprintf(OutputFile, "\tcall\t%s\n", Entry->Name);
    if(Args > 4)
        fprintf(OutputFile, "\taddq\t$%d, %%rsp\n", 8 * (Args - 4));

    fprintf(OutputFile, "\tmovq\t%%rax, %s\n", Registers[OutRegister]);
    
    return OutRegister;
}

// Assemble a function return.
int AsReturn(struct SymbolTableEntry* Entry, int Register) {

    printf("\t\tCreating return for function %s\n", Entry->Name);

    switch(Entry->Type) {
        case RET_CHAR:
            fprintf(OutputFile, "\tmovzbl\t%s, %%eax\n", ByteRegisters[Register]);
            break;
        
        case RET_INT:
            fprintf(OutputFile, "\tmovl\t%s, %%eax\n", DoubleRegisters[Register]);
            break;
        
        case RET_LONG:
            fprintf(OutputFile, "\tmovq\t%s, %%rax\n", Registers[Register]);
            break;
        
        default:
            DieMessage("Bad function type in generating return", TypeNames(Entry->Type));

    }

    AsJmp(Entry->EndLabel);
}


// Assemble a =?
int AsEqual(int Left, int Right) {
    // Set the lowest bit if left = right
    return AsCompare(OP_EQUAL, Left, Right);
}

// Assemble a !=
int AsIneq(int Left, int Right) {
    // Set the lowest bit if left != right
    return AsCompare(OP_INEQ, Left, Right);
}

// Assemble a <
int AsLess(int Left, int Right) {
    // Set the lowest bit if left < right
    return AsCompare(OP_LESS, Left, Right);
}

// Assemble a >
int AsGreat(int Left, int Right) {
    // Set the lowest bit if left > right
    return AsCompare(OP_GREAT, Left, Right);
}

// Assemble a <=
int AsLessE(int Left, int Right) {
    // Set the lowest bit if left <= right
    return AsCompare(OP_LESSE, Left, Right);
}

// Assemble a =>
int AsGreatE(int Left, int Right) {
    // Set the lowest bit if left => right
    return AsCompare(OP_GREATE, Left, Right);
}

// Assemble a print statement
void AssemblerPrint(int Register) {
    printf("\t\tPrinting Register %s\n", Registers[Register]);

    fprintf(OutputFile, "\tmovq\t%s, %%rcx\n", Registers[Register]);
    //fprintf(OutputFile, "\tleaq\t.LC0(%%rip), %%rcx\n");
    fprintf(OutputFile, "\tcall\tPrintInteger\n");

    DeallocateRegister(Register);
}

// Assemble a &
int AsBitwiseAND(int Left, int Right) {
    fprintf(OutputFile, "\tandq\t%s, %s\n", Registers[Left], Registers[Right]);
    DeallocateRegister(Left);
    return Right;
}

// Assemble a |
int AsBitwiseOR(int Left, int Right) {
    fprintf(OutputFile, "\torq\t%s, %s\n", Registers[Left], Registers[Right]);
    DeallocateRegister(Left);
    return Right;
}

// Assemble a ^
int AsBitwiseXOR(int Left, int Right) {
    fprintf(OutputFile, "\txorq\t%s, %s\n", Registers[Left], Registers[Right]);
    DeallocateRegister(Left);
    return Right;
}

// Assemble a ~
int AsNegate(int Register) {
    fprintf(OutputFile, "\tnegq\t%s\n", Registers[Register]);
    return Register;
}

// Assemble a !
int AsInvert(int Register) {
    fprintf(OutputFile, "\tnotq\t%s\n", Registers[Register]);
    return Register;
}

// Assemble a !
int AsBooleanNOT(int Register) {
    fprintf(OutputFile, "\ttest\t%s, %s\n", Registers[Register], Registers[Register]);
    fprintf(OutputFile, "\tsete\t%s\n", ByteRegisters[Register]);
    fprintf(OutputFile, "\tmovzbq\t%s, %s\n", ByteRegisters[Register], Registers[Register]);
    return Register;
}

// Assemble a <<
int AsShiftLeft(int Left, int Right) {
    fprintf(OutputFile, "\tmovb\t%s, \%%cl\n", ByteRegisters[Right]);
    fprintf(OutputFile, "\tshlq\t\%%cl, %s\n", Registers[Left]);
    DeallocateRegister(Right);
    return Left;
}

// Assemble a >>
int AsShiftRight(int Left, int Right) {
    fprintf(OutputFile, "\tmovb\t%s, \%%cl\n", ByteRegisters[Right]);
    fprintf(OutputFile, "\tshrq\t\%%cl, %s\n", Registers[Left]);
    DeallocateRegister(Right);
    return Left;
}

// Assemble a conversion from arbitrary type to boolean.
// Facilitates if(ptr)
int AsBooleanConvert(int Register, int Operation, int Label) {
    fprintf(OutputFile, "\ttest\t%s, %s\n", Registers[Register], Registers[Register]);

    switch(Operation) {
        case OP_IF:
        case OP_LOOP:
            fprintf(OutputFile, "\tje\tL%d\n", Label);
            break;
        default:
            fprintf(OutputFile, "\tsetnz\t%s\n", ByteRegisters[Register]);
            fprintf(OutputFile, "\tmovzbq\t%s, %s\n", ByteRegisters[Register], Registers[Register]);
            break;
    }

    return Register;
}

// Assemble the start of an assembly file
void AssemblerPreamble() {
    DeallocateAllRegisters();
    fputs(
            "\t.text\n", /* 
            ".LC0:\n"
            "\t.string\t\"%d\\n\"\n", */
            OutputFile);
}

/*
 * Assemble a function block for the Entry.
 * Handles all stack logic for local variables,
 *  as well as copying parameters out of registers and
 *   into the spill space.
 * 
 * @param Entry: The function to generate
 * 
 */
void AsFunctionPreamble(struct SymbolTableEntry* Entry) {
    char* Name = Entry->Name;
    struct SymbolTableEntry* Param, *Local;
    int ParamOffset = 0, ParamReg = 9, ParamCount = 0;

    LocalVarOffset = 4; // Prepare parameters

    fprintf(OutputFile,
            "\t.text\n"
            "\t.globl\t%s\n"
            "\t.def\t%s; .scl 2; .type 32; .endef\n"
            "%s:\n"
            "\tpushq\t%%rbp\n"
            "\tmovq\t%%rsp, %%rbp\r\n",
            Name, Name, Name);
    
    //PECOFF requires we call the global initialisers
    if(!strcmp(Name, "main"))
        fprintf(OutputFile, "\tcall\t__main\n");
    
    // Need to share this between two loops. Fun.
    int LoopIndex;

    // If we have parameters, move them to the last 4 registers
    for(Param = Entry->Start, ParamCount = 1; Param != NULL; Param = Param->NextSymbol, ParamCount++) {
        if(ParamCount > 4) { // We only have 4 argument registers
            Param->SinkOffset = ParamOffset;
            ParamOffset += 8;
        }

        Entry->SinkOffset = AsCalcOffset(Param->Type);
        AsStrLocalVar(Param, ParamReg--);
    }

    // If we have more parameters, move them to the stack
    for(Local = Locals; Local != NULL; Local = Local->NextSymbol) {
        Local->SinkOffset = AsCalcOffset(Local->Type);
    }

    // With all the parameters on the stack, we can allocate the shadow space
    StackFrameOffset = ((LocalVarOffset + 31) & ~31);
    fprintf(OutputFile, 
            "\taddq\t$%d, %%rsp\n", -StackFrameOffset);

}


// Assemble the epilogue of a function
void AsFunctionEpilogue(struct SymbolTableEntry* Entry) {
    AsLabel(Entry->EndLabel);

    fprintf(OutputFile,
            "\tpopq\t%%rbp\n"
            "\taddq\t$%d, %%rsp\n"
            "\tret\n",
            StackFrameOffset);
}