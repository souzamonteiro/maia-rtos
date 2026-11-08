#ifndef MAIASCRIPT_PARSER_HPP
#define MAIASCRIPT_PARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include "ast.hpp"

namespace maiascript {

class Parser {
public:
    static std::shared_ptr<Program> parseSource(const std::string& source, std::string& outError,
                                                const std::string& currentPath = "",
                                                const std::vector<std::string>& searchPaths = {});
    static std::shared_ptr<Program> parseFile(const std::string& path, std::string& outError,
                                              const std::vector<std::string>& searchPaths = {});
    static std::string expandIncludes(const std::string& source, const std::string& currentPath,
                                      const std::vector<std::string>& searchPaths,
                                      std::unordered_set<std::string>& includedFiles,
                                      std::string& outError);
};

} // namespace maiascript

#endif // MAIASCRIPT_PARSER_HPP
