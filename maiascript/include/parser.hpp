#ifndef MAIASCRIPT_PARSER_HPP
#define MAIASCRIPT_PARSER_HPP

#include <string>
#include <memory>
#include "ast.hpp"

namespace maiascript {

class Parser {
public:
    static std::shared_ptr<Program> parseSource(const std::string& source, std::string& outError);
    static std::shared_ptr<Program> parseFile(const std::string& path, std::string& outError);
};

} // namespace maiascript

#endif // MAIASCRIPT_PARSER_HPP

