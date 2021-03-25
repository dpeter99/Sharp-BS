/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#pragma once
#include <stdio.h>
#include <Defs.h>
#include <stdbool.h>

#ifndef extern_
#define extern_ extern
#endif

#define TEXTLEN 512
#define SYMBOLS 1024

extern_ struct SymbolTableEntry* Globals, *GlobalsEnd;
extern_ struct SymbolTableEntry* Locals, *LocalsEnd;
extern_ struct SymbolTableEntry* Params, *ParamsEnd;
extern_ struct SymbolTableEntry* Structs, *StructsEnd;
extern_ struct SymbolTableEntry* StructMembers, *StructMembersEnd;

extern_ struct SymbolTableEntry* Unions, *UnionsEnd;
extern_ struct SymbolTableEntry* Enums, *EnumsEnd;

extern_ bool OptDumpTree;
extern_ bool OptKeepAssembly;
extern_ bool OptAssembleFiles;
extern_ bool OptLinkFiles;
extern_ bool OptVerboseOutput;

extern_ char* OutputFileName;
extern_ char* CurrentASMFile, *CurrentObjectFile;

extern_ int   TypeSizes[5];

extern_ char* TokenNames[];

extern_ int CurrentFunction;
extern_ struct SymbolTableEntry* FunctionEntry;
extern_ int Line;
extern_ int Overread;

extern_ FILE* SourceFile;
extern_ FILE* OutputFile;

extern_ struct Token CurrentToken;
extern_ char CurrentIdentifier[TEXTLEN + 1];

extern_ int CurrentGlobal;
extern_ int CurrentLocal;