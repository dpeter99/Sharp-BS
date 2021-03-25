/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>
#include <errno.h>

/********************************************************************************
 * The Delegate is what allows the compiler backend to be abstracted.           *
 *                                                                              *
 * It delegates the operation of compiling, assembling and linking              *
 *  to the proper subsystems.                                                   *
 *                                                                              *
 * As of right now (20/01/2021) it uses the GCC backend.                        *
 *                                                                              *
 * Compile parses files to their AST and generates mingw PECOFF32+ assembly,    *
 * Assemble uses GCC-as to compile the assembly to an object file.              *
 * Link links the object files into an executable.                              *
 *                                                                              *
 ********************************************************************************/

/*
 * Files inputted must have a suffix/extension (because we're on Windows right now)
 * This is the way to change the suffix for when a file is converted to another.
 * 
 * @param String: The full, current file name
 * @param Suffix: The new, desired extension.
 * 
 */

char* Suffixate(char* String, char Suffix) {
    char* Pos, *NewStr;

    if((NewStr = strdup(String)) == NULL) 
        return NULL;

    if((Pos = strrchr(NewStr, '.')) == NULL)
        return NULL;
    
    Pos++;

    if(*Pos == '\0')
        return NULL;
    
    *Pos++ = Suffix;
    *Pos = '\0';
    return NewStr;
}


/*
 * Starts most of the work to do with the Erythro compiler.  
 * It:  
 *  Opens the input and output files,  
 *  Parses the global symbols of the file, including function blocks.  
 *  Generates the assembly representation of the source code  
 *  Saves said assembly into the OutputFile  
 *  Returns the name of the file containing the generated assembly.  
 * Note that the Input file must have a valid extension.  
 *  For Erythro code, this is .er  
 * The generated assembly will have the extension .s  
 * 
 * @param InputFile: The filename of the Erythro Source code to compile  
 * @return the filename of the generated PECOFF32+ assembly  
 */
char* Compile(char* InputFile) {
    char* OutputName;
    OutputName = Suffixate(InputFile, 's');
    if(OutputName == NULL) {
        fprintf(stderr, "%s must have a suffix.\r\n", InputFile);
        exit(1);
    }

    if((SourceFile = fopen(InputFile, "r")) == NULL) {
        fprintf(stderr, "Unable to open %s: %s\n", InputFile, strerror(errno));
        exit(1);
    } 

    if((OutputFile = fopen(OutputName, "w")) == NULL) {
        fprintf(stderr, "Unable to open %s: %s\n", OutputName, strerror(errno));
        exit(1);
    }

    Line = 1;
    Overread = '\n';
    CurrentGlobal = 0;
    CurrentLocal = SYMBOLS - 1;

    if(OptVerboseOutput)
        printf("Compiling %s\r\n", InputFile);
    
    Tokenise();

    AssemblerPreamble();

    ParseGlobals();

    fclose(OutputFile);
    return OutputName;
}

/*
 * Processes the output from the Compile function.
 * Passes the generated .s file to (currently, as of
 *  21/01/2021), the GNU GAS assembler, to create an
 *   object file.
 * 
 * It does this by invoking the command on a shell.
 *  TODO: fork it?
 * 
 * @param InputFile: The .s assembly file to be processed
 * @output the name of the generated object file.
 * 
 */

char* Assemble(char* InputFile) {
    char Command[TEXTLEN];
    int Error;
    char* OutputName;
    OutputName = Suffixate(InputFile, 'o');
    if(OutputName == NULL) {
        fprintf(stderr, "%s must have a suffix.\r\n", InputFile);
        exit(1);
    }

    snprintf(Command, TEXTLEN, "%s %s %s", "as -o ", OutputName, InputFile);
    if(OptVerboseOutput)
        printf("%s\n", Command);
    
    Error = system(Command);

    if(Error != 0) {
        fprintf(stderr, "Assembling of %s failed with code %d\n", InputFile, Error);
        exit(1);
    }
    return OutputName;
}

/*
 * Processes the outputted object files, turning them into an executable.
 * It does this by invoking (currently, as of 21/01/2021) the GNU GCC
 *  compiler.
 * It invokes GCC rather than LD so that it automatically links against
 *  libc and the CRT natives.
 * 
 * @param Output: The desired name for the executable.
 * @param Objects: A list of the Object files to be linked.
 * 
 */

void Link(char* Output, char* Objects[]) {
    int Count, Size = TEXTLEN, Error;
    char Command[TEXTLEN], *CommandPtr;

    CommandPtr = Command;
    Count = snprintf(CommandPtr, Size, "%s %s ", "gcc -o ", OutputFileName);
    CommandPtr += Count;
    Size -= Count;

    while(*Objects != NULL) {
        Count = snprintf(CommandPtr, Size, "%s ", *Objects);
        CommandPtr += Count;
        Size -= Count;
        Objects++;
    }

    if(OptVerboseOutput)
        printf("%s\n", Command);
    
    Error = system(Command);

    if(Error != 0) {
        fprintf(stderr, "Link failure\n");
        exit(1);
    }
}

/*
 * Prints information about the available flags and
 *  how to structure the command.
 * @param ProgName: The name of the file that was
 *  attempted to run.
 */

void DisplayUsage(char* ProgName) {
    fprintf(stderr, "Erythro Compiler v5 - Gemwire Institute\n");
    fprintf(stderr, "***************************************\n");
    fprintf(stderr, "Usage: %s -[vcST] {-o output} file [file ...]\n", ProgName);
    fprintf(stderr, "       -v: Verbose Output Level\n");
    fprintf(stderr, "       -c: Compile without Linking\n");
    fprintf(stderr, "       -S: Assemble without Linking\n");
    fprintf(stderr, "       -T: Dump AST\n");
    fprintf(stderr, "       -o: Name of the destination [executable/object/assembly] file.\n");
    exit(1);
}
