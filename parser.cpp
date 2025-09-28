#include <iostream>
#include <vector>
#include <string>
#include <utility>

#include "tokenize.cpp"

enum ParseError
{
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

struct ParseException
{
    ParseError error;
    Token token;
    std::string message;

    ParseException(ParseError e, const Token &t, const std::string &msg = "")
        : error(e), token(t), message(std::move(msg)) {}
};

struct ASTNode
{
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
    std::string indentStr(int n) const { return std::string(n * 2, ' '); }
};

struct Expr : ASTNode
{
};

struct LiteralExpr : Expr
{
    std::string value;
    std::string type;
    LiteralExpr(std::string v, std::string t) : value(std::move(v)), type(std::move(t)) {}
    void print(int indent = 0) const override
    {
        std::cout << indentStr(indent) << "Literal(" << type << ": " << value << ")\n";
    }
};

struct IdentExpr : Expr
{
    std::string name;
    IdentExpr(std::string n) : name(std::move(n)) {}
    void print(int indent = 0) const override
    {
        std::cout << indentStr(indent) << "Identifier(" << name << ")\n";
    }
};

struct Stmt : ASTNode
{
};

struct VarDeclStmt : Stmt
{
    std::string type;
    std::string name;
    Expr *init;
    VarDeclStmt(std::string t, std::string n, Expr *i = nullptr)
        : type(std::move(t)), name(std::move(n)), init(i) {}

    ~VarDeclStmt()
    {
        delete init;
    }

    void print(int indent = 0) const override
    {
        std::cout << indentStr(indent) << "VarDecl(" << type << " " << name << ")\n";
        if (init)
        {
            std::cout << indentStr(indent + 1) << "Initializer:\n";
            init->print(indent + 2);
        }
    }
};

class Parser
{
public:
    Parser(const std::vector<Token> &tokens) : tokens(tokens), index(0) {}

    std::vector<Stmt *> parseProgram()
    {
        std::vector<Stmt *> statements;
        while (!isAtEnd())
        {
            statements.push_back(parseStatement());
        }
        return statements;
    }

private:
    const std::vector<Token> &tokens;
    size_t index;

    const Token &current() const
    {
        return tokens[index];
    }

    const Token &previous() const
    {
        return tokens[index - 1];
    }

    bool isAtEnd() const
    {
        return tokens[index].type == T_EOF;
    }

    const Token &advance()
    {
        if (!isAtEnd())
        {
            index++;
        }
        return previous();
    }

    bool match(TokenType type)
    {
        if (isAtEnd() || current().type != type)
        {
            return false;
        }
        advance();
        return true;
    }

    void expect(TokenType type, ParseError errType, const std::string &msg)
    {
        if (current().type != type)
        {
            throw ParseException(errType, current(), msg);
        }
        advance();
    }

    Stmt *parseStatement()
    {
        if (current().type == T_INT || current().type == T_FLOAT ||
            current().type == T_STRING || current().type == T_BOOL)
        {
            return parseVarDecl();
        }
        throw ParseException(ExpectedTypeToken, current(), "Expected a type at start of statement");
    }

    Stmt *parseVarDecl()
    {
        std::string type = current().value;
        advance();

        const Token &nameToken = current();
        expect(T_IDENTIFIER, ExpectedIdentifier, "Expected variable name after type");
        std::string name = nameToken.value;

        Expr *initializer = nullptr;
        if (match(T_ASSIGNOP))
        {
            initializer = parseExpression(type);
        }

        expect(T_SEMICOLON, FailedToFindToken, "Expected ';' after variable declaration");
        return new VarDeclStmt(type, name, initializer);
    }

    Expr *parseExpression(const std::string &expectedType)
    {
        Token currentToken = current();

        switch (currentToken.type)
        {
        case T_INTLIT:
            if (expectedType != "int")
                throw ParseException(ExpectedIntLit, currentToken, "Type mismatch: expected int literal for int variable.");
            advance();
            return new LiteralExpr(currentToken.value, "int");

        case T_FLOATLIT:
            if (expectedType != "float")
                throw ParseException(ExpectedFloatLit, currentToken, "Type mismatch: expected float literal for float variable.");
            advance();
            return new LiteralExpr(currentToken.value, "float");

        case T_STRINGLIT:
            if (expectedType != "string")
                throw ParseException(ExpectedStringLit, currentToken, "Type mismatch: expected string literal for string variable.");
            advance();
            return new LiteralExpr(currentToken.value, "string");

        case T_BOOLLIT:
            if (expectedType != "bool")
                throw ParseException(ExpectedBoolLit, currentToken, "Type mismatch: expected bool literal for bool variable.");
            advance();
            return new LiteralExpr(currentToken.value, "bool");

        case T_IDENTIFIER:
            advance();
            return new IdentExpr(currentToken.value);

        default:
            throw ParseException(ExpectedExpr, currentToken, "Expected an expression after '='");
        }
    }
};

std::string parseErrorToString(ParseError err)
{
    switch (err)
    {
    case UnexpectedEOF:
        return "UnexpectedEOF";
    case FailedToFindToken:
        return "FailedToFindToken";
    case ExpectedTypeToken:
        return "ExpectedTypeToken";
    case ExpectedIdentifier:
        return "ExpectedIdentifier";
    case UnexpectedToken:
        return "UnexpectedToken";
    case ExpectedFloatLit:
        return "ExpectedFloatLit";
    case ExpectedIntLit:
        return "ExpectedIntLit";
    case ExpectedStringLit:
        return "ExpectedStringLit";
    case ExpectedBoolLit:
        return "ExpectedBoolLit";
    case ExpectedExpr:
        return "ExpectedExpr";
    default:
        return "UnknownError";
    }
}

void runTest(const std::string &code, const std::string &description)
{
    std::cout << "\n=============================\n";
    std::cout << "TEST: " << description << "\n";
    std::cout << "=============================\n";
    std::cout << "Code:\n"
              << code << "\n\n";

    std::vector<Token> tokens = tokenize(code);
    std::vector<Stmt *> program;

    std::cout << "--- Tokens ---\n";
    for (const auto &t : tokens)
    {
        std::cout << tokenTypeToString(t.type) << "\t\"" << t.value
                  << "\"\tLine: " << t.line << "\tCol: " << t.column << std::endl;
    }

    std::cout << "\n--- Parsing ---\n";
    try
    {
        Parser parser(tokens);
        program = parser.parseProgram();
        std::cout << "AST Generated Successfully:\n";
        for (auto const &stmt : program)
        {
            stmt->print(0);
        }
    }
    catch (const ParseException &ex)
    {
        std::cerr << "Parse error: " << parseErrorToString(ex.error)
                  << " at token (" << tokenTypeToString(ex.token.type)
                  << ", \"" << ex.token.value << "\") on line " << ex.token.line
                  << ". Message: " << ex.message << "\n";
    }

    for (auto stmt : program)
    {
        delete stmt;
    }
}

int main()
{

    runTest(R"(int x1 = 42;)", "Valid variable declaration");
    runTest(R"(int 1x = 53;)", "UnValid variable declaration");
    runTest(R"(int x = 42)", "Missing semicolon after declaration");
    runTest(R"(x = 42;)", "Declaration without type keyword");
    runTest(R"(int = 42;)", "Missing variable name after type");
    runTest(R"(int 123 = 5;)", "Unexpected token: number used as variable name");
    runTest(R"(int x = "Rahim";)", "Type mismatch: int variable assigned string literal");
    runTest(R"(float pi = true;)", "Type mismatch: float variable assigned boolean literal");
    runTest(R"(string name = 42;)", "Type mismatch: string variable assigned int literal");
    runTest(R"(bool flag = 123;)", "Type mismatch: bool variable assigned int literal");
    runTest(R"(int x = ;)", "Missing expression after assignment");
    runTest(R"(int y = 5; int z = )", "Unexpected EOF inside code");
    runTest(R"(int x 42;)", "Unexpected token: '=' missing before literal");
    runTest(R"(float pi = "abc";)", "Type mismatch: float variable assigned string literal");

    return 0;
}