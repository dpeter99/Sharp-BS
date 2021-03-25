
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>

/*
 * Find the position of a symbol in a given symbol table.
 *  @param Name: The string name of the symbol
 *  @param List: The linked list to search in.
 *  @return the list if found,
 *      NULL if no found.
 */

static struct SymbolTableEntry* SearchList(char* Name, struct SymbolTableEntry* List) {
    for(; List != NULL; List = List->NextSymbol)
        if((List->Name != NULL) && !strcmp(Name, List->Name))
            return (List);
    return NULL;
}

/*
 * Search all the tables for a symbol.
 * Use the overrides for polluted types 
 *  eg. if you need a global from a local scope
 * 
 * @param Symbol: The string name of the symbol to search for
 * @return the Node corresponding to the most likely 
 *  symbol required.
 * 
 * 
 */
struct SymbolTableEntry* FindSymbol(char* Symbol) {
    struct SymbolTableEntry* Node;

    if(CurrentFunction) {
        Node = SearchList(Symbol, FunctionEntry->Start);
        if(Node)
            return Node;
    }

    Node = SearchList(Symbol, Locals);
    if(Node)
        return Node;
    
    return SearchList(Symbol, Globals);
}

/*
 * An override for FindSymbol.
 * Searches only the parameters and local variables.
 *  @param Symbol: The string name of the symbol to search for.
 *  @return a pointer to the node if found, else NULL
 */
struct SymbolTableEntry* FindLocal(char* Symbol) {
    struct SymbolTableEntry* Node;

    if(FunctionEntry) {
        Node = SearchList(Symbol, FunctionEntry->Start);
        if(Node)
            return Node;
    }

    return SearchList(Symbol, Locals);
}

/*
 * An override for FindSymbol.
 * Searches only the global variables.
 *  @param Symbol: The string name of the symbol to search for.
 *  @return a pointer to the node if found, else NULL
 * 
 */
struct SymbolTableEntry* FindGlobal(char* Symbol) {
    return SearchList(Symbol, Globals);
}

/*
 * An override for FindSymbol.
 * Searches only the defined Structs.
 *  @param Symbol: The string name of the symbol to search for.
 *  @return a pointer to the node if found, else NULL
 * 
 */
struct SymbolTableEntry* FindStruct(char* Symbol) {
    return SearchList(Symbol, Structs);
}

/*
 * An override for FindSymbol.
 * Searches only the defined Struct & Enum Members.
 *  @param Symbol: The string name of the symbol to search for.
 *  @return a pointer to the node if found, else NULL
 * 
 */
struct SymbolTableEntry* FindMember(char* Symbol) {
    return SearchList(Symbol, StructMembers);
}

/*
 * Given a particular linked list,
 *  Take Node and append it to the Tail.
 * 
 * If there is no tail, set it to the Head.
 *  This prevents orphaned lists.
 * 
 * @param Head: The start of the desired linked list 
 * @param Tail: The end of the desired linked list
 * @param Node: The new item to append
 * 
 */
void AppendSymbol(struct SymbolTableEntry** Head, struct SymbolTableEntry** Tail, struct SymbolTableEntry* Node) {
    if(Head == NULL || Tail == NULL || Node == NULL)
        Die("Not enough data to append a symbol to the tables");
    
    if(*Tail) {
        (*Tail)->NextSymbol = Node;
        *Tail = Node;
    } else {
        *Head = *Tail = Node;
    }

    Node->NextSymbol = NULL;
}


/*
 * Reset the local variables of functions.
 */

void FreeLocals() {
    Locals = LocalsEnd = NULL;
    Params = ParamsEnd = NULL;
    FunctionEntry = NULL;

}

/*
 * Reset all tables.
 */
void ClearTables() {
    Globals = GlobalsEnd = NULL;
    Locals = LocalsEnd = NULL;
    Params = ParamsEnd = NULL;
    StructMembers = StructMembersEnd = NULL;
    Structs = StructsEnd = NULL;
}


/*
 * Create a symbol item, and set all the metadata.
 *  @param Name:      The string representing the name of the symbol.
 *  @param Type:      The return type in terms of DataTypes enum values.
 *  @param Structure: The type of symbol this is, in terms of StructureType enum.
 *  @param Storage:   The storage scope of this symbol. For functions this is always SC_GLOBAL (for now). Vars and Arrays can be GLOBAL or SC_LOCAL.
 *  @param Length:  The label # to jump to to exit the function or array, where appropriate.  
 *                    The size of the struct/array in units of 1xbase
 * 
 *  @return The SymbolTableEntry* pointer that corresponds to this newly constructed node.
 */
struct SymbolTableEntry* AddSymbol(char* Name, int Type, int Structure, int Storage, int Length, int SinkOffset, struct SymbolTableEntry* CompositeType) {

    struct SymbolTableEntry* Node = 
        (struct SymbolTableEntry*) malloc(sizeof(struct SymbolTableEntry));

    Node->Name = strdup(Name);
    Node->Type = Type;
    Node->Structure = Structure;
    Node->Storage = Storage;
    Node->Length = Length;
    Node->SinkOffset = SinkOffset;
    Node->CompositeType = CompositeType;

    switch(Storage) {
        case SC_GLOBAL:
            AppendSymbol(&Globals, &GlobalsEnd, Node);
            // We don't want to generate a static block for functions.
            if(Structure != ST_FUNC) AsGlobalSymbol(Node);
            break;
        case SC_STRUCT:
            AppendSymbol(&Structs, &StructsEnd, Node);
            break;
        case SC_MEMBER:
            AppendSymbol(&StructMembers, &StructMembersEnd, Node);
        case SC_LOCAL:
            AppendSymbol(&Locals, &LocalsEnd, Node);
            break;
        case SC_PARAM:
            AppendSymbol(&Params, &ParamsEnd, Node);
            break;
        
    }

    
    return Node;
}