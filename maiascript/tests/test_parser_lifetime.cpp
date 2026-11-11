#include "parser.hpp"
#include <cassert>
#include <iostream>

// Returned ASTs must own their data after temporary CST storage is destroyed,
// including across later successful and failed parses on the same thread.
int main() {
    std::string error;
    auto retained = maiascript::Parser::parseSource(
        "i32 saved_identifier_longer_than_a_small_string_buffer() { return 73; }\n"
        "string saved_text() { return \"a literal longer than a small string buffer\"; }", error);
    assert(retained && error.empty());

    std::string source = "i32 main() { i32 result = 0;\n";
    for (int i = 0; i < 256; ++i)
        source += "result = result + 1;\n";
    source += "return result; }\n";

    for (int i = 0; i < 8; ++i) {
        error.clear();
        auto program = maiascript::Parser::parseSource(source, error);
        assert(program && error.empty());
        auto functions = program->getFunctions();
        assert(functions.size() == 1 && functions[0]->name == "main");
        assert(functions[0]->body->statements.size() == 258);

        error.clear();
        auto invalid = maiascript::Parser::parseSource("i32 main( { return ;", error);
        assert(!invalid && !error.empty());

        auto saved = retained->getFunctions();
        assert(saved.size() == 2);
        assert(saved[0]->name == "saved_identifier_longer_than_a_small_string_buffer");
        auto statement = maiascript::ast_cast<maiascript::ReturnStmt>(saved[0]->body->statements[0]);
        assert(statement);
        auto value = maiascript::ast_cast<maiascript::LiteralExpr>(statement->expr);
        assert(value && value->intVal == 73);
        auto textStatement = maiascript::ast_cast<maiascript::ReturnStmt>(saved[1]->body->statements[0]);
        assert(textStatement);
        auto text = maiascript::ast_cast<maiascript::LiteralExpr>(textStatement->expr);
        assert(text && text->strVal.find("a literal longer than a small string buffer") != std::string::npos);
    }
    std::cout << "PASS: AST lifetime after CST release, repeated large parses, syntax failure/recovery\n";
}
