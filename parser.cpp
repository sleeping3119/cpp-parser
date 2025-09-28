#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include "tokenize.cpp"

struct ParseException {
    std::string message;
    Token token;

    ParseException(const std::string &msg, const Token &tok)
        : message(msg), token(tok) {}
};

struct Expr {
    virtual ~Expr() = default;
    virtual void print(int indent = 0) const = 0;
};

struct LiteralExpr : Expr {
    std::string value;
    std::string type;

    LiteralExpr(std::string v, std::string t) : value(std::move(v)), type(std::move(t)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Literal(" << type << ": " << value << ")\n";
    }
};

struct IdentExpr : Expr {
    std::string name;
    IdentExpr(std::string n) : name(std::move(n)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Identifier(" << name << ")\n";
    }
};

struct VarDecl {
    std::string type;
    std::string name;
    Expr *initializer;

    VarDecl(std::string t, std::string n, Expr *init = nullptr)
        : type(std::move(t)), name(std::move(n)), initializer(init) {}

    ~VarDecl() {
        delete initializer;
    }

    void print(int indent = 0) const {
        std::cout << std::string(indent, ' ') << "VarDecl(" << type << " " << name << ")\n";
        if (initializer) {
            std::cout << std::string(indent + 2, ' ') << "Initializer:\n";
            initializer->print(indent + 4);
        }
    }
};

class Parser {
public:
    Parser(const std::vector<Token> &tokens) : tokens(tokens), index(0) {}

    std::vector<VarDecl *> parseProgram() {
        std::vector<VarDecl *> program;
        while (!isAtEnd()) {
            program.push_back(parseVarDecl());
        }
        return program;
    }

private:
    const std::vector<Token> &tokens;
    size_t index;

    const Token &current() const { return tokens[index]; }
    const Token &previous() const { return tokens[index - 1]; }

    bool isAtEnd() const { return current().type == T_EOF; }

    const Token &advance() {
        if (!isAtEnd()) index++;
        return previous();
    }

    bool match(TokenType type) {
        if (isAtEnd() || current().type != type) return false;
        advance();
        return true;
    }

    void expect(TokenType type, const std::string &errorMsg) {
        if (current().type != type) {
            throw ParseException(errorMsg, current());
        }
        advance();
    }

    VarDecl *parseVarDecl() {
        std::string type = current().value;
        if (!(match(T_INT) || match(T_FLOAT) || match(T_STRING) || match(T_BOOL))) {
            throw ParseException("Expected type (int, float, string, bool)", current());
        }

        Token nameToken = current();
        expect(T_IDENTIFIER, "Expected variable name after type");
        std::string name = nameToken.value;

        Expr *initializer = nullptr;
        if (match(T_ASSIGNOP)) {
            initializer = parseExpression(type);
        }

        expect(T_SEMICOLON, "Missing ';' at end of statement");
        return new VarDecl(type, name, initializer);
    }

    Expr *parseExpression(const std::string &expectedType) {
        Token tok = current();

        switch (tok.type) {
            case T_INTLIT:
                if (expectedType != "int")
                    throw ParseException("Type mismatch: expected int", tok);
                advance();
                return new LiteralExpr(tok.value, "int");

            case T_FLOATLIT:
                if (expectedType != "float")
                    throw ParseException("Type mismatch: expected float", tok);
                advance();
                return new LiteralExpr(tok.value, "float");

            case T_STRINGLIT:
                if (expectedType != "string")
                    throw ParseException("Type mismatch: expected string", tok);
                advance();
                return new LiteralExpr(tok.value, "string");

            case T_BOOLLIT:
                if (expectedType != "bool")
                    throw ParseException("Type mismatch: expected bool", tok);
                advance();
                return new LiteralExpr(tok.value, "bool");

            case T_IDENTIFIER:
                advance();
                return new IdentExpr(tok.value);

            default:
                throw ParseException("Expected an expression after '='", tok);
        }
    }
};

void runTest(const std::string &code, const std::string &description) {
    std::cout << "\n=============================\n";
    std::cout << "TEST: " << description << "\n";
    std::cout << "=============================\n";
    std::cout << "Code:\n" << code << "\n\n";

    auto tokens = tokenize(code);
    std::vector<VarDecl *> program;

    std::cout << "--- Tokens ---\n";
    for (const auto &t : tokens) {
        std::cout << tokenTypeToString(t.type) << "\t\"" << t.value
                  << "\"\tLine: " << t.line << "\tCol: " << t.column << "\n";
    }

    std::cout << "\n--- Parsing ---\n";
    try {
        Parser parser(tokens);
        program = parser.parseProgram();
        std::cout << "AST Generated Successfully:\n";
        for (auto stmt : program) {
            stmt->print(0);
        }
    } catch (const ParseException &ex) {
        std::cerr << "Parse error: " << ex.message
                  << " at token (" << tokenTypeToString(ex.token.type)
                  << ", \"" << ex.token.value << "\") on line " << ex.token.line << "\n";
    }

    for (auto stmt : program) {
        delete stmt;
    }
}

int main() {
    runTest("int x1 = 42;", "Valid variable declaration");
    runTest("int 1x = 53;", "Invalid variable name");
    runTest("int x = 42", "Missing semicolon");
    runTest("x = 42;", "Declaration without type");
    runTest("int = 42;", "Missing variable name");
    runTest("int x = \"Rahim\";", "Type mismatch: int assigned string");
    runTest("float pi = true;", "Type mismatch: float assigned bool");
    runTest("string name = 42;", "Type mismatch: string assigned int");
    runTest("bool flag = 123;", "Type mismatch: bool assigned int");
    runTest("int x = ;", "Missing expression after assignment");

    return 0;
}
