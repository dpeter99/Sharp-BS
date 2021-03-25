/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#define extern_
#include <Data.h>
#undef extern_
#include <errno.h>


int TypeSizes[5] = { 0, 1, 4, 8, 0}; // in BYTES

char* TokenNames[] = { 
    "End of file",
    "Equivalency",

    "Boolean Logic OR",
    "Boolean Logic AND",

    "Bitwise OR",
    "Bitwise XOR",
    "Bitwise AND",

    "Equality Check",
    "Inequality Check",
    "Less Than", 
    "Greater Than",
    "Less Than or Equal",
    "Greater Than or Equal",

    "Left Shift",
    "Right Shift",

    "Addition",
    "Subtraction",
    "Multiplication",
    "Division",

    "Increment",
    "Decrement",

    "Statement Logical Invert",
    "Bitwise Invert",
    
    "Integer literal",
    "String literal",
    "Statement End",

    "Compound Block Start",
    "Compound Block End",

    "Array index start",
    "Array index end",

    "Logical Block Start",
    "Logical Block End",

    "Comma",

    "Identifier",
    "None Type",
    "Char Type",
    "Int Type",
    "Long Type",
    "Void Type",

    "Function keyword",
    "Print Keyword",
    "If keyword",
    "Else keyword",
    "While keyword",
    "For keyword",

    "Return keyword",
    
    "Struct keyword"
};

int main(int argc, char* argv[]) {
    // Option initialisers
    OptDumpTree = false;
    OptKeepAssembly = false;
    OptAssembleFiles = false;
    OptLinkFiles = true;
    OptVerboseOutput = false;

    // Temporary .o storage and counter
    char* ObjectFiles[100];
    int ObjectCount = 0;

    // Parse command line arguments.
    int i;
    for(i = 1/*skip 0*/; i < argc; i++) {
        // If we're not a flag, we can skip.
        // We only care about flags in rows.
        // ie. erc >> -v -T -o << test.exe src/main.er
        if(*argv[i] != '-')
            break;
        
        // Once we identify a flag, we need to make sure it's not just a minus in-place.
        for(int j = 1; (*argv[i] == '-') && argv[i][j]; j++) {
            // Finally, identify what option is being invoked.
            switch(argv[i][j]) {
                case 'o':  // output
                    OutputFileName = argv[++i];
                    
                    break;
                case 'T': // Debug
                   OptDumpTree = true;
                   break;
                case 'c': // Compile only
                    OptAssembleFiles = true;
                    OptKeepAssembly = false;
                    OptLinkFiles = false;
                    break;
                case 'S': // aSsemble only
                    OptAssembleFiles = false;
                    OptKeepAssembly = true;
                    OptLinkFiles = false;
                    break;
                case 'v': // Verbose output
                    OptVerboseOutput = true;
                    break;
                default:
                    DisplayUsage(argv[0]);
            }
        }
    }

    // If we didn't provide anything other than flags, we need to show how to use the program.
    if(i >= argc) 
        DisplayUsage(argv[0]);

    // For the rest of the files specified, we can iterate them right to left.
    while(i < argc) {
        // Compile the file by invoking the Delegate
        CurrentASMFile = Compile(argv[i]);
        if(OptLinkFiles || OptAssembleFiles) {
            // If we need to assemble (or link, which requires assembly)
            // then we invoke the Delegate again
            CurrentObjectFile = Assemble(CurrentASMFile);
            // We can only keep track of 99 objects, so we should crash at 98 to ensure we have enough room for the output file too.
            if(ObjectCount == 98) {
                fprintf(stderr, "Too many inputs");
                return 1; // We use return because we're in main, rather than invoking Die.
            }

            // Move the ObjectCount forward.
            ObjectFiles[ObjectCount++] = CurrentObjectFile;
            // Clear the new, forwarded index
            ObjectFiles[ObjectCount] = NULL;
        }

        if(!OptKeepAssembly)
            // unlink = delete
            unlink(CurrentASMFile);

        i++;
    }

    if(OptLinkFiles) {
        // If needed, invoke the Delegate one last time.
        Link(OutputFileName, ObjectFiles);
        if(!OptAssembleFiles) {
            // Even though we need to assemble to link, we can respect the user's options and delete the intermediary files.
            for(i = 0; ObjectFiles[i] != NULL; i++)
                unlink(ObjectFiles[i]);
        }
    }

    return 0;

}

/*
 * Akin to a Halt and Catch Fire method.
 * Simply prints an error, cleans up handles, and closes.
 */

void Die(char* Error) {
    fprintf(stderr, "%s on line %d\n", Error, Line);
    fclose(OutputFile);
    unlink(OutputFileName);
    exit(1);
}

/*
 * A variant of Die with an extra String attached.
 */
void DieMessage(char* Error, char* Reason) {
    fprintf(stderr, "%s: %s on line %d\n", Error, Reason, Line);
    fclose(OutputFile);
    unlink(OutputFileName);
    exit(1);
}

/*
 * A variant of Die with an extra integer attached.
 */
void DieDecimal(char* Error, int Number) {
    fprintf(stderr, "%s: %d on line %d\n", Error, Number, Line);
    fclose(OutputFile);
    unlink(OutputFileName);
    exit(1);
}

/*
 * A variant of Die with an extra character attached.
 */
void DieChar(char* Error, int Char) {
    fprintf(stderr, "%s: %c on line %d\n", Error, Char, Line);
    fclose(OutputFile);
    unlink(OutputFileName);
    exit(1);
}