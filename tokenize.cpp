#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <memory>
#include <sstream>
using namespace std;

enum TokenType {
    T_FUNCTION, T_INT, T_FLOAT, T_STRING, T_BOOL, T_RETURN,
    T_IF, T_ELSE, T_FOR, T_WHILE, T_BREAK, T_CONTINUE,
    T_IDENTIFIER, T_INTLIT, T_FLOATLIT, T_STRINGLIT, T_BOOLLIT,
    T_ASSIGNOP, T_EQUALSOP, T_PLUS, T_MINUS, T_MULT, T_DIV, T_MOD,
    T_LT, T_GT, T_LTE, T_GTE, T_NEQ, T_AND, T_OR, T_NOT,
    T_BITAND, T_BITOR, T_BITXOR, T_BITNOT, T_LEFTSHIFT, T_RIGHTSHIFT,
    T_PARENL, T_PARENR, T_BRACEL, T_BRACER, T_BRACKL, T_BRACKR,
    T_COMMA, T_SEMICOLON, T_COLON, T_QUESTION, T_DOT, T_COMMENT,
    T_UNKNOWN, T_EOF, T_INVALID_IDENTIFIER, T_INCREMENT, T_PLUS_ASSIGN
};

struct Token {
    TokenType type;
    string value;
    int line;
    int column;
};

// List of keywords for comparison
const set<string> keywords = {
    "fn", "int", "float", "string", "bool",
    "return", "if", "else", "for", "while",
    "break", "continue", "true", "false"
};

vector<Token> tokenize(const string& src) {
    vector<Token> tokens;
    size_t pos = 0;
    int line = 1, col = 1;

    while (pos < src.size()) {
        char c = src[pos];

        // Skip whitespace
        if (isspace(c)) {
            if (c == '\n') { line++; col = 1; }
            else col++;
            pos++;
            continue;
        }

        // Identifier or keyword (Unicode supported)
        if (isalpha(c) || c == '_' || (unsigned char)c >= 128) {
            int startCol = col;
            string acc;
            while (pos < src.size()) {
                unsigned char ch = src[pos];
                if (isalnum(ch) || ch == '_' || ch >= 128) {
                    acc += ch;
                    pos++;
                    col++;
                } else break;
            }

            if (keywords.count(acc)) {
                if (acc == "true" || acc == "false") tokens.push_back({T_BOOLLIT, acc, line, startCol});
                else if (acc == "fn") tokens.push_back({T_FUNCTION, acc, line, startCol});
                else if (acc == "int") tokens.push_back({T_INT, acc, line, startCol});
                else if (acc == "float") tokens.push_back({T_FLOAT, acc, line, startCol});
                else if (acc == "string") tokens.push_back({T_STRING, acc, line, startCol});
                else if (acc == "bool") tokens.push_back({T_BOOL, acc, line, startCol});
                else if (acc == "return") tokens.push_back({T_RETURN, acc, line, startCol});
                else if (acc == "if") tokens.push_back({T_IF, acc, line, startCol});
                else if (acc == "else") tokens.push_back({T_ELSE, acc, line, startCol});
                else if (acc == "for") tokens.push_back({T_FOR, acc, line, startCol});
                else if (acc == "while") tokens.push_back({T_WHILE, acc, line, startCol});
                else if (acc == "break") tokens.push_back({T_BREAK, acc, line, startCol});
                else if (acc == "continue") tokens.push_back({T_CONTINUE, acc, line, startCol});
            } else {
                tokens.push_back({T_IDENTIFIER, acc, line, startCol});
            }
            continue;
        }

        // Number literal
        if (isdigit(c)) {
            int startCol = col;
            string acc;
            bool dotSeen = false;
            while (pos < src.size()) {
                if (isdigit(src[pos])) {
                    acc += src[pos];
                } 
                else if (src[pos] == '.') {
                    if (dotSeen) {
                        cerr << "LexerError: Multiple decimal points in number '" 
                            << acc << "' at Line " << line << ", Col " << startCol << endl;
                        tokens.push_back({T_INVALID_IDENTIFIER, acc, line, startCol});
                        return tokens; // or handle appropriately
                    }
                    dotSeen = true;
                    acc += '.';
                } 
                else break;
                pos++;
                col++;
            }
            // Check invalid identifier like 123abc
            if (pos < src.size() && (isalpha(src[pos]) || src[pos]=='_')) {
                while (pos < src.size() && (isalnum(src[pos]) || src[pos]=='_')) {
                    acc += src[pos];
                    pos++;
                    col++;
                }
                tokens.push_back({T_INVALID_IDENTIFIER, acc, line, startCol});
            } else {
                tokens.push_back(dotSeen ? Token{T_FLOATLIT, acc, line, startCol} : Token{T_INTLIT, acc, line, startCol});
            }
            continue;
        }

        // String literal
        if (c == '"') {
            int startCol = col;
            string acc;
            pos++; col++; // skip opening "
            while (pos < src.size() && src[pos] != '"') {
                char ch = src[pos++];
                col++;
                if (ch == '\\' && pos < src.size()) {
                    char esc = src[pos++];
                    col++;
                    if (esc == 'n') acc += '\n';
                    else if (esc == 't') acc += '\t';
                    else if (esc == 'r') acc += '\r';
                    else if (esc == '\\') acc += '\\';
                    else if (esc == '"') acc += '"';
                    else acc += esc;
                } else acc += ch;
            }
            if (pos < src.size() && src[pos] == '"') { pos++; col++; }
            tokens.push_back({T_STRINGLIT, acc, line, startCol});
            continue;
        }

        // Comments
        if (c == '/' && pos+1 < src.size() && src[pos+1]=='/') {
            int startCol = col;
            string acc;
            pos += 2; col += 2;
            while (pos < src.size() && src[pos] != '\n') { acc += src[pos]; pos++; col++; }
            tokens.push_back({T_COMMENT, acc, line, startCol});
            continue;
        }
        if (c == '/' && pos+1 < src.size() && src[pos+1]=='*') {
            int startCol = col;
            string acc;
            pos += 2; col += 2;
            while (pos+1 < src.size() && !(src[pos]=='*' && src[pos+1]=='/')) {
                acc += src[pos];
                if (src[pos] == '\n') { line++; col = 1; } else col++;
                pos++;
            }
            if (pos+1 < src.size()) { pos += 2; col += 2; }
            tokens.push_back({T_COMMENT, acc, line, startCol});
            continue;
        }

        // Symbols & operators
        int startCol = col;
        string val(1, c);
        pos++; col++;
        if (c == '=' && pos < src.size() && src[pos]=='=') { val = "=="; pos++; col++; tokens.push_back({T_EQUALSOP,val,line,startCol}); continue; }
        else if (c == '+' && pos < src.size() && src[pos]=='+') { val = "++"; pos++; col++; tokens.push_back({T_INCREMENT,val,line,startCol}); continue; }
        else if (c == '+' && pos < src.size() && src[pos]=='=') { val = "+="; pos++; col++; tokens.push_back({T_PLUS_ASSIGN,val,line,startCol}); continue; }

        // Map single-character symbols
        TokenType type = T_UNKNOWN;
        if (c == '+') type = T_PLUS;
        else if (c == '-') type = T_MINUS;
        else if (c == '*') type = T_MULT;
        else if (c == '/') type = T_DIV;
        else if (c == '%') type = T_MOD;
        else if (c == '<') type = T_LT;
        else if (c == '>') type = T_GT;
        else if (c == '!') type = T_NOT;
        else if (c == '&') type = T_BITAND;
        else if (c == '|') type = T_BITOR;
        else if (c == '^') type = T_BITXOR;
        else if (c == '~') type = T_BITNOT;
        else if (c == '(') type = T_PARENL;
        else if (c == ')') type = T_PARENR;
        else if (c == '{') type = T_BRACEL;
        else if (c == '}') type = T_BRACER;
        else if (c == '[') type = T_BRACKL;
        else if (c == ']') type = T_BRACKR;
        else if (c == ',') type = T_COMMA;
        else if (c == ';') type = T_SEMICOLON;
        else if (c == ':') type = T_COLON;
        else if (c == '?') type = T_QUESTION;
        else if (c == '.') type = T_DOT;
        else if (c == '=') type = T_ASSIGNOP;

        tokens.push_back({type,val,line,startCol});
    }

    tokens.push_back({T_EOF,"",line,col});
    return tokens;
}
// Helper function to get token type as string
string tokenTypeToString(TokenType type) {
    switch(type) {
        case T_FUNCTION: return "T_FUNCTION";
        case T_INT: return "T_INT";
        case T_FLOAT: return "T_FLOAT";
        case T_STRING: return "T_STRING";
        case T_BOOL: return "T_BOOL";
        case T_RETURN: return "T_RETURN";
        case T_IF: return "T_IF";
        case T_ELSE: return "T_ELSE";
        case T_FOR: return "T_FOR";
        case T_WHILE: return "T_WHILE";
        case T_BREAK: return "T_BREAK";
        case T_CONTINUE: return "T_CONTINUE";
        case T_IDENTIFIER: return "T_IDENTIFIER";
        case T_INTLIT: return "T_INTLIT";
        case T_FLOATLIT: return "T_FLOATLIT";
        case T_STRINGLIT: return "T_STRINGLIT";
        case T_BOOLLIT: return "T_BOOLLIT";
        case T_ASSIGNOP: return "T_ASSIGNOP";
        case T_EQUALSOP: return "T_EQUALSOP";
        case T_PLUS: return "T_PLUS";
        case T_MINUS: return "T_MINUS";
        case T_MULT: return "T_MULT";
        case T_DIV: return "T_DIV";
        case T_MOD: return "T_MOD";
        case T_LT: return "T_LT";
        case T_GT: return "T_GT";
        case T_LTE: return "T_LTE";
        case T_GTE: return "T_GTE";
        case T_NEQ: return "T_NEQ";
        case T_AND: return "T_AND";
        case T_OR: return "T_OR";
        case T_NOT: return "T_NOT";
        case T_BITAND: return "T_BITAND";
        case T_BITOR: return "T_BITOR";
        case T_BITXOR: return "T_BITXOR";
        case T_BITNOT: return "T_BITNOT";
        case T_LEFTSHIFT: return "T_LEFTSHIFT";
        case T_RIGHTSHIFT: return "T_RIGHTSHIFT";
        case T_PARENL: return "T_PARENL";
        case T_PARENR: return "T_PARENR";
        case T_BRACEL: return "T_BRACEL";
        case T_BRACER: return "T_BRACER";
        case T_BRACKL: return "T_BRACKL";
        case T_BRACKR: return "T_BRACKR";
        case T_COMMA: return "T_COMMA";
        case T_SEMICOLON: return "T_SEMICOLON";
        case T_COLON: return "T_COLON";
        case T_QUESTION: return "T_QUESTION";
        case T_DOT: return "T_DOT";
        case T_COMMENT: return "T_COMMENT";
        case T_UNKNOWN: return "T_UNKNOWN";
        case T_EOF: return "T_EOF";
        case T_INVALID_IDENTIFIER: return "T_INVALID_IDENTIFIER";
        case T_INCREMENT: return "T_INCREMENT";
        case T_PLUS_ASSIGN: return "T_PLUS_ASSIGN";
        default: return "T_UNKNOWN";
    }
}