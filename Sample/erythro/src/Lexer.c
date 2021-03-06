
/*************/
/*GEMWIRE    */
/*    ERYTHRO*/
/*************/

#include <Defs.h>
#include <Data.h>


/* * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * *    C H A R       S T R E AM     * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * The Lexer holds a "stream" of characters.
 * You may read a character from the stream, and if it is not
 *  the desired character, it may be placed into an "overread" buffer.
 * The overread buffer is checked before the source file is read any further.
 * This provides an effective way to "un-read" a character.
 * 
 * @param Char: The character to "un-read"
 * 
 */

static void ReturnCharToStream(int Char) {
    Overread = Char;
}

/*
 * NextChar allows you to ask the Lexer for the next useful character.
 * As mentioned above, it checks the overread buffer first.
 * 
 * @return the character as int
 * 
 */
static int NextChar(void) {
    int Char;

    if(Overread) {
        Char = Overread;
        Overread = 0;
        return Char;
    }

    Char = fgetc(SourceFile);

    if(Char == '\n')
        Line++;
    
    return Char;
}

/*
 * Searches for the next useful character, skipping whitespace.
 * @return the character as int.
 */

static int FindChar() {
    int Char;

    Char = NextChar();

    while(Char == ' ' || Char == '\t' || Char == '\n' || Char == '\r') {
        Char = NextChar();
    }

    return Char;
}

/*
 * Allows the conversion between ASCII, hex and numerals.
 * @param String: The set of all valid results
 * @param Char: The ASCII character to convert
 * @return the ASCII character in int form, if in the set of valid results. -1 if not.
 */

static int FindDigitFromPos(char* String, char Char) {
    char* Result = strchr(String, Char);
    return(Result ? Result - String : -1);
}

/*
 * Facilitates the easy checking of expected tokens.
 *  NOTE: there is (soon to be) an optional variant of this function that
 *         reads a token but does not consume it ( via Tokenise )
 * 
 * @param Type: The expected token, in terms of value of the TokenTypes enum.
 * @param TokenExpected: A string to output when the token is not found.
 * 
 */

void VerifyToken(int Type, char* TokenExpected) {
    if(CurrentToken.type == Type)
        Tokenise();
    else {
        printf("Expected %s on line %d\n", TokenExpected, Line);
        exit(1);
    }
}

static struct Token* RejectedToken = NULL;

/*
 * Rejected Tokens and the Overread Stream are identical concepts.
 * This was implemented first, but it is no longer used.
 * TODO: Refactor this function out.
 */

void RejectToken(struct Token* Token) {
    if(RejectedToken != NULL)
        Die("Cannot reject two tokens in a row!");
    
    RejectedToken = Token;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * *     L I T E R A L S   A N D   I D E N T I F I E R S     * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Facilitates the parsing of integer literals from the file.
 * Currently only supports the decimal numbers, despite the
 *  FindDigitFromPos function allowing conversion.
 * 
 * The functon loops over the characters, multiplying by 10 and adding
 *  the new value on top, until a non-numeric character is found.
 * At that point, it returns the non-numeric character to the Overread Stream
 *  and returns the calculated number.
 * 
 * @param Char: The first number to scan.
 * @return the full parsed number as an int.
 * 
 */

static int ReadInteger(int Char) {
    int CurrentChar = 0;
    int IntegerValue = 0;

    while((CurrentChar = FindDigitFromPos("0123456789", Char)) >= 0) {
        IntegerValue = IntegerValue * 10 + CurrentChar;
        Char = NextChar();
    }

    ReturnCharToStream(Char);

    return IntegerValue;
}

/*
 * An Identifier can be any of:
 *  * A function name
 *  * A variable name
 *  * A struct name
 *  / A class name
 *  / An annotation name
 * 
 * This function allows a full name to be read into a buffer, with a defined
 *  start character and a defined maximum text size limit.
 * 
 * @param Char: The first char of the Identifier.
 * @param Buffer: The location to store the Identifier. (usually CurrentIdentifer, a compiler global defined for this purpose)
 * @param Limit: The maximum Identifer length.
 * @return the length of the parsed identifier
 * 
 */
static int ReadIdentifier(int Char, char* Buffer, int Limit) {
    int ind = 0;   

    // This defines the valid chars in a keyword/variable/function.
    while(isalpha(Char) || isdigit(Char) || Char == '_') {
        if (ind >= Limit - 1) {
            printf("Identifier too long: %d\n", Line);
            exit(1);
        } else {
            Buffer[ind++] = Char;
        }

        Char = NextChar();
    }

    // At this point, we've reached a non-keyword character
    ReturnCharToStream(Char);
    Buffer[ind] = '\0';
    return ind;
}

/*
 * Char literals appear as 'x'
 * 
 * They are bounded by two apostrophes.
 * They can contain any 1-byte ASCII character, as well as some
 *  predefined, standard escape codes.
 * This function attempts to get the character from the file, with escape codes intact.
 * 
 * @return the character as an int 
 * 
 */
static int ReadCharLiteral() {
    int Char;
    Char = NextChar();
    if(Char == '\\') {
        switch(Char = NextChar()) {
            case 'a': return '\a';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            case 'v': return '\v';
            case '\\': return '\\';
            case '"': return '"';
            case '\'': return '\'';
            default:
                DieChar("Unknown Escape: ", Char);
        }
    }

    return Char;
}

/*
 * String literals appear as "hello world"
 * 
 * They are bounded by two quotation marks.
 * They can contain an arbitrary length of text.
 *  They are backed by an array of chars (hence the char* type) and thus
 *   have a practically unlimited length.
 * 
 * To read a String Literal, it is a simple matter of reading Char Literals until 
 *  the String termination token is identified - the last quotation mark.
 * 
 * @param Buffer: The buffer into which to write the string. (usually CurrentIdentifer, a compiler global defined for this purpose)
 * 
 */
static int ReadStringLiteral(char* Buffer) {
    int Char;

    for(int i = 0; i < TEXTLEN - 1; i++) {
        if((Char = ReadCharLiteral()) == '"') {
            Buffer[i] = 0; return i;
        }
        
        Buffer[i] = Char;
    }

    Die("String Literal Too Long");
    return 0;
}

/*
 * Keywords are source-code tokens / strings that are reserved for the compiler.
 *  They cannot be used as identifers on their own.
 * 
 * This function is where all of the keywords are added, and where most aliases are going to be stored.
 * 
 * It uses a switch on the first character of the input string as an optimisation - rather than checking each
 *  keyword against the String individually, it only needs to compare a single number. This can be optimised into
 *   a hash table by the compiler for further optimisation, making this one of the fastest ways to switch
 *    on a full string.
 *
 * @param Str: The keyword input to try to parse
 * @return the token expressed in terms of values of the TokenTypes enum 
 * 
 */
static int ReadKeyword(char* Str) {
    // First, scan with reference intact.
    switch(*Str) {
        // This lets us case against the first char:
        case ':':
            if(!strcmp(Str, "::"))
                return KW_FUNC;
            break;

        case 'c':
            if(!strcmp(Str, "char"))
                return TY_CHAR;
            break;

        case 'e':
            if(!strcmp(Str, "else"))
                return KW_ELSE;

            break;
    
        case 'f':
            if(!strcmp(Str, "for"))
                return KW_FOR;
            break;

        case 'i':
            // alias char, int and long types

            if(!strcmp(Str, "i8"))
                return TY_CHAR;
            if(!strcmp(Str, "i32"))
                return TY_INT;
            if(!strcmp(Str, "i64"))
                return TY_LONG;

            if(!strcmp(Str, "int"))
                return TY_INT;
            
            if(!strcmp(Str, "if"))
                return KW_IF;

            break;

        case 'l':
            if(!strcmp(Str, "long"))
                return TY_LONG;

            break;

        case 'p':
            if(!strcmp(Str, "print"))
                return KW_PRINT;
            break;

        case 'r':
            if(!strcmp(Str, "return"))
                return KW_RETURN;
            break;

        case 's':
            if(!strcmp(Str, "struct"))
                return KW_STRUCT;
            break;
            
        case 'v':
            if(!strcmp(Str, "void"))
                return TY_VOID;
            break;
            
        case 'w':
            if(!strcmp(Str, "while"))
                return KW_WHILE;
            break;


        
    }

    return 0;
}

/* * * * * * * * * * * * * * * * * * * * *
 * * * *      T O K E N I S E R    * * * *
 * * * * * * * * * * * * * * * * * * * * */

/*
 * Handles the majority of the work of reading tokens into the stream.
 * It reads chars with FindChar, categorizing individual characters or small 
 *  strings into their proper expression (as a value of the TokenTypes enum)
 * 
 * It also defers the reading of numeric literals and char literals to the proper functions.
 * 
 * If needed, it can also read Identifiers, for variable or function naming.
 * 
 * This function may be the main bottleneck in the lexer.
 * 
 */
void Tokenise() {
    int Char, TokenType;
    struct Token* Token = &CurrentToken;

    if(RejectedToken != NULL) {
        Token = RejectedToken;
        RejectedToken = NULL;
        return;
    }

    Char = FindChar();

    switch(Char) {
        case EOF:
            Token->type = LI_EOF;
            return;

        case '+':
            // + can be either "+" or "++".
            Char = NextChar();
            if(Char == '+') {
                Token->type = PPMM_PLUS;
            } else {
                Token->type = AR_PLUS;
                ReturnCharToStream(Char);
            }
            break;

        case '-':
            // - can be either "-" or "--"
            Char = NextChar();
            if(Char == '-') {
                Token->type = PPMM_MINUS;
            } else {
                Token->type = AR_MINUS;
                ReturnCharToStream(Char);
            }
            break;

        case '*':
            Token->type = AR_STAR;
            break;

        case '/':
            Token->type = AR_SLASH;
            break;

        case '&':
            Char = NextChar();
            if(Char == '&') {
                Token->type = BOOL_AND;
            } else {
                Token->type = BIT_AND;
                ReturnCharToStream(Char);
            }
            break;
        
        case '|':
            Char = NextChar();
            if(Char == '|') {
                Token->type = BOOL_OR;
            } else {
                Token->type = BIT_OR;
                ReturnCharToStream(Char);
            }
            break;
        
        case '^':
            Token->type = BIT_XOR;
            break;
        
        case '~':
            Token->type = BIT_NOT;
            break;

        case ',':
            Token->type = LI_COM;
            break;
        
        case '=':
            Char = NextChar();
            // If the next char is =, we have ==, the compare equality token.
            if(Char == '?') {
                Token->type = CMP_EQUAL;
            // if the next char is >, we have =>, the greater than or equal token.
            } else if(Char == '>') {
                Token->type = CMP_GTE;
            // If none of the above match, we have = and an extra char. Return the char and set the token
            } else {
                ReturnCharToStream(Char);
                Token->type = LI_EQUAL;
            }
            break;
        
        case '!':
            Char = NextChar();
            // If the next char is =, we have !=, the compare inequality operator.
            if(Char == '=') {
                Token->type = CMP_INEQ;
            // Otherwise, we have a spare char
            } else {
                Token->type = BOOL_INVERT;
                ReturnCharToStream(Char);            
            }
            break;

        case '<':
            Char = NextChar();
            // If the next char is =, we have <=, the less than or equal comparator.
            if(Char == '=') {
                Token->type = CMP_LTE;
            } else if(Char == '<') { // But if the next char is <, we have << - the Shift Left operator.
                Token->type = SH_LEFT;
            } else {
                ReturnCharToStream(Char);
                Token->type = CMP_LT;
            }
            break;

        case '>':
            // For >, Less than or equal is => so we can ignore it, but the Shift Right operator is >>.
            Char = NextChar();
            if(Char == '>') {
                Token->type = SH_RIGHT;
            } else {
                Token->type = CMP_GT;
                ReturnCharToStream(Char);
            }
            break;

        case ';':
            Token->type = LI_SEMIC;
            break;

        case '(':
            Token->type = LI_LPARE;
            break;
        
        case ')':
            Token->type = LI_RPARE;
            break;
        
        case '{':
            Token->type = LI_LBRAC;
            break;

        case '}':
            Token->type = LI_RBRAC;
            break;

        case '[':
            Token->type = LI_LBRAS;
            break;
        
        case ']':
            Token->type = LI_RBRAS;
            break;
            
        case ':':
            Char = NextChar();

            if(Char == ':') {
                Token->type = KW_FUNC;
            } else {
                ReturnCharToStream(Char);
            }
            break;

        case '\'':
            Token->value = ReadCharLiteral();
            Token->type = LI_INT;

            if(NextChar() != '\'')
                Die("Expected '\\'' at the end of a character.");
            break;

        case '"':
            ReadStringLiteral(CurrentIdentifier);
            Token->type = LI_STR;
            break;

        default:
            if(isdigit(Char)) {

                Token->value = ReadInteger(Char);
                Token->type = LI_INT;
                break;
            
            } else if(isalpha(Char) || Char == '_') { // This is what defines what a variable/function/keyword can START with.
                ReadIdentifier(Char, CurrentIdentifier, TEXTLEN);

                if(TokenType = ReadKeyword(CurrentIdentifier)) {
                    Token->type = TokenType;
                    break;
                }
                
                Token->type = TY_IDENTIFIER;
                break;
                //printf("Line %d: Unrecognized symbol %s\n", CurrentIdentifier, Line);
                //exit(1);
            }

            
            DieChar("Unrecognized character", Char);

    }
}

