#include "Lexer.hpp"

/*
 * Function to return the next token from standard input
 * the variable 'm_IdentifierStr' is set there in case of an identifier,
 * the variable 'm_NumVal' is set there in case of a number.
 */

std::string m_IdentifierStr;
int m_NumVal;

    //   ;                  < = >           !           |
bool isTwoCharOp(char c){
    if (c != 59 && ( (c < 63 && c > 57)  || c == 33 || c == 124)) return true;
    return false;
}

int gettok() {

    static int LastChar = ' ';

    // Skip any whitespace.
    while (isspace(LastChar))
        LastChar = getchar();

   if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
        m_IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            m_IdentifierStr += LastChar;
        if (m_IdentifierStr == "function")
            return tok_function;
        if (m_IdentifierStr == "program")
            return tok_program;
        if (m_IdentifierStr == "begin")
            return tok_begin;
        if (m_IdentifierStr == "end")
            return tok_end;
        if (m_IdentifierStr == "integer")
            return tok_integer;
        if (m_IdentifierStr == "exit")
            return tok_exit;
        if (m_IdentifierStr == "if")
            return tok_if;
        if (m_IdentifierStr == "then")
            return tok_then;
        if (m_IdentifierStr == "else")
            return tok_else;
        if (m_IdentifierStr == "for")
            return tok_for;
        if (m_IdentifierStr == "to")
            return tok_to;
        if (m_IdentifierStr == "do")
            return tok_do;
        if (m_IdentifierStr == "var")
            return tok_var;
        if (m_IdentifierStr == "const")
            return tok_const;
        if (m_IdentifierStr == "downto")
            return tok_downto;
        if (m_IdentifierStr == "procedure")
            return tok_procedure;
        return tok_identifier;
    }


    if (isdigit(LastChar)) { // Decimal [0-9]+
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar));

        m_NumVal = (int) strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '&') { // Hex number [0-9]+
        LastChar = getchar();
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar));
        char * pend;
        m_NumVal = (int) strtol(NumStr.c_str(), &pend, 8);
        return tok_number;
    }

    if (LastChar == '$') { // octal number: [0-9]+
        LastChar = getchar();
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar)||isalnum(LastChar));
        char * pend;
        m_NumVal = (int) strtol(NumStr.c_str(), &pend, 16);
        return tok_number;
    }


    if (LastChar == '#') {
    // Comment until end of line.
    do
        LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
        return gettok();
    }

    // two character operators
    if (isTwoCharOp(LastChar)){
        std::string expressionStr;
        expressionStr = LastChar;

        // read character without removing it from steam
        while(isTwoCharOp(std::cin.peek())){
            LastChar= getchar();
            expressionStr += LastChar;
            if (expressionStr == "<="){
                LastChar = getchar();
                return tok_lessequal;
            }else if(expressionStr == ">=")   {
                LastChar = getchar();
                return tok_greaterequal;
            }else if(expressionStr == ":=")   {
                LastChar = getchar();
                return tok_assign;
            }else if(expressionStr == "!=")   {
                LastChar = getchar();
                return tok_notequal;
            }else if(expressionStr == "||")   {
                LastChar = getchar();
                return tok_or;
            }
        }
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;

}