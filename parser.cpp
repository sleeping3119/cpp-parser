#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <memory>
#include <sstream>
#include "tokenize.cpp"
using namespace std;

using namespace std;

enum ParseError {
    UnexpectedEOF,
    FailedToFindToken,
    ExpectedTypeToken,
    ExpectedIdentifier,
    UnexpectedToken,
    ExpectedFloatLit,
    ExpectedIntLit,
    ExpectedStringLit,
    ExpectedBoolLit,
    ExpectedExpr
};

struct ParseException {
    ParseError error;
    Token token;
    string message;

    ParseException(ParseError e, const Token &t, const string &msg = "")
        : error(e), token(t), message(msg) {}
};

 
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
    string indentStr(int n) const { return string(n * 2, ' '); }
};

struct Expr : ASTNode {};
struct LiteralExpr : Expr {
    string value;
    string type;
    LiteralExpr(string v, string t) : value(v), type(t) {}
    void print(int indent = 0) const override {
        cout << indentStr(indent) << "Literal(" << type << ": " << value << ")\n";
    }
};

struct IdentExpr : Expr {
    string name;
    IdentExpr(string n) : name(n) {}
    void print(int indent = 0) const override {
        cout << indentStr(indent) << "Identifier(" << name << ")\n";
    }
};

// ---- Statements ----
struct Stmt : ASTNode {};
struct VarDeclStmt : Stmt {
    string type;
    string name;
    Expr* init;
    VarDeclStmt(string t, string n, Expr* i = nullptr) : type(t), name(n), init(i) {}
    void print(int indent = 0) const override {
        cout << indentStr(indent) << "VarDecl(" << type << " " << name << ")\n";
        if (init) {
            cout << indentStr(indent + 1) << "Initializer:\n";
            init->print(indent + 2);
        }
    }
};

// ---- Parser Class ----
class Parser {
public:
    Parser(const vector<Token> &tokens) : tokens(tokens), index(0) {}

    vector<Stmt*> parseProgram() {
        vector<Stmt*> statements;
        while (!isAtEnd()) {
            statements.push_back(parseStatement());
        }
        return statements;
    }

private:
    const vector<Token> &tokens;
    size_t index;

    // --- Utility ---
    const Token& current() const {
        if (index >= tokens.size()) {
            static Token eofToken = {T_EOF, ""};
            return eofToken;
        }
        return tokens[index];
    }

    const Token& advance() {
        if (!isAtEnd()) index++;
        return previous();
    }

    const Token& previous() const {
        return tokens[index - 1];
    }

    bool isAtEnd() const {
        return current().type == T_EOF;
    }

    bool match(TokenType type) {
        if (current().type == type) {
            advance();
            return true;
        }
        return false;
    }

    void expect(TokenType type, ParseError errType, const string &msg) {
        if (current().type != type) {
            throw ParseException(errType, current(), msg);
        }
        advance();
    }

    // --- Parse Functions ---
    Stmt* parseStatement() {
        if (current().type == T_INT || current().type == T_FLOAT ||
            current().type == T_STRING || current().type == T_BOOL) {
            return parseVarDecl();
        }
        throw ParseException(ExpectedTypeToken, current(), "Expected a type at start of statement");
    }

    Stmt* parseVarDecl() {
        string type = current().value;
        advance();

        if (current().type != T_IDENTIFIER) {
            throw ParseException(ExpectedIdentifier, current(), "Expected variable name after type");
        }
        string name = current().value;
        advance();

        Expr* initializer = nullptr;
        if (match(T_ASSIGNOP)) {
            initializer = parseExpression(type);
        }

        expect(T_SEMICOLON, FailedToFindToken, "Expected ';' after variable declaration");
        return new VarDeclStmt(type, name, initializer);
    }

    Expr* parseExpression(const string &expectedType) {
        // Match based on expected variable type
        switch (current().type) {
            case T_INTLIT:
                if (expectedType != "int")
                    throw ParseException(ExpectedIntLit, current(), "Expected integer literal");
                return new LiteralExpr(current().value, "int");

            case T_FLOATLIT:
                if (expectedType != "float")
                    throw ParseException(ExpectedFloatLit, current(), "Expected float literal");
                return new LiteralExpr(current().value, "float");

            case T_STRINGLIT:
                if (expectedType != "string")
                    throw ParseException(ExpectedStringLit, current(), "Expected string literal");
                return new LiteralExpr(current().value, "string");

            case T_BOOLLIT:
                if (expectedType != "bool")
                    throw ParseException(ExpectedBoolLit, current(), "Expected boolean literal");
                return new LiteralExpr(current().value, "bool");

            case T_IDENTIFIER:
                return new IdentExpr(current().value);

            default:
                throw ParseException(ExpectedExpr, current(), "Expected an expression after '='");
        }
    }
};

string parseErrorToString(ParseError err) {
    switch (err) {
        case UnexpectedEOF: return "UnexpectedEOF";
        case FailedToFindToken: return "FailedToFindToken";
        case ExpectedTypeToken: return "ExpectedTypeToken";
        case ExpectedIdentifier: return "ExpectedIdentifier";
        case UnexpectedToken: return "UnexpectedToken";
        case ExpectedFloatLit: return "ExpectedFloatLit";
        case ExpectedIntLit: return "ExpectedIntLit";
        case ExpectedStringLit: return "ExpectedStringLit";
        case ExpectedBoolLit: return "ExpectedBoolLit";
        case ExpectedExpr: return "ExpectedExpr";
        default: return "UnknownError";
    }
}

// Test runner
void runTest(const string &code, const string &description) {
    cout << "\n=============================\n";
    cout << "TEST: " << description << "\n";
    cout << "=============================\n";
    cout << code << "\n\n";

    vector<Token> tokens = tokenize(code);

    // Print all tokens
    cout << "--- Tokens ---\n";
    for (const auto &t : tokens) {
        cout << tokenTypeToString(t.type) << "\t\"" << t.value
             << "\"\tLine: " << t.line << "\tCol: " << t.column << endl;
    }

    cout << "\n--- Parsing ---\n";
    try {
        Parser parser(tokens);
        auto program = parser.parseProgram();
        cout << "AST Generated Successfully:\n";
        for (auto &stmt : program) stmt->print(0);
    } catch (const ParseException &ex) {
        cerr << "Parse error: " << parseErrorToString(ex.error)
             << " at token (" << tokenTypeToString(ex.token.type)
             << ", \"" << ex.token.value << "\")"
             << " msg: " << ex.message << "\n";
    }
}

int main() {
    // 1. Successful parsing
    runTest(
        R"(int x = 42;)",
        "Valid variable declaration"
    );

    // 2. Missing semicolon -> FailedToFindToken
    runTest(
        R"(int x = 42)",
        "Missing semicolon after declaration"
    );

    // 3. No type keyword -> ExpectedTypeToken
    runTest(
        R"(x = 42;)",
        "Declaration without type keyword"
    );

    // 4. No identifier -> ExpectedIdentifier
    runTest(
        R"(int = 42;)",
        "Missing variable name after type"
    );

    // 5. Unexpected token (number as identifier) -> UnexpectedToken
    runTest(
        R"(int 123 = 5;)",
        "Unexpected token: number used as variable name"
    );

    // 6. Type mismatch (int assigned string) -> ExpectedIntLit
    runTest(
        R"(int x = "Rahim";)",
        "Type mismatch: int variable assigned string literal"
    );

    // 7. Type mismatch (float assigned bool) -> ExpectedFloatLit
    runTest(
        R"(float pi = true;)",
        "Type mismatch: float variable assigned boolean literal"
    );

    // 8. Type mismatch (string assigned int) -> ExpectedStringLit
    runTest(
        R"(string name = 42;)",
        "Type mismatch: string variable assigned int literal"
    );

    // 9. Type mismatch (bool assigned int) -> ExpectedBoolLit
    runTest(
        R"(bool flag = 123;)",
        "Type mismatch: bool variable assigned int literal"
    );

    // 10. Missing expression -> ExpectedExpr
    runTest(
        R"(int x = ;)",
        "Missing expression after assignment"
    );

    // 11. Unexpected EOF inside program -> UnexpectedEOF
    runTest(
        R"(int y = 5; int z = )",
        "Unexpected EOF inside code"
    );

    // 12. '=' missing between type and value -> UnexpectedToken
    runTest(
        R"(int x 42;)",
        "Unexpected token: '=' missing before literal"
    );

    // 13. Float assigned string -> ExpectedFloatLit
    runTest(
        R"(float pi = "abc";)",
        "Type mismatch: float variable assigned string literal"
    );

    return 0;
}
