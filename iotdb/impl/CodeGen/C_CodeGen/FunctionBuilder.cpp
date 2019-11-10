
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <CodeGen/C_CodeGen/CodeCompiler.hpp>
#include <CodeGen/CodeExpression.hpp>
#include <CodeGen/PipelineStage.hpp>
#include <Util/ErrorHandling.hpp>

#include <CodeGen/C_CodeGen/BinaryOperatorStatement.hpp>
#include <CodeGen/C_CodeGen/Declaration.hpp>
#include <CodeGen/C_CodeGen/FileBuilder.hpp>
#include <CodeGen/C_CodeGen/FunctionBuilder.hpp>
#include <CodeGen/C_CodeGen/Statement.hpp>
#include <CodeGen/C_CodeGen/UnaryOperatorStatement.hpp>
#include "../../../include/CodeGen/DataTypes.hpp"

namespace iotdb {

class StructBuilder {
  public:
    static StructBuilder create(const std::string& struct_name);
    StructBuilder& addField(AttributeFieldPtr attr);
    StructDeclaration build();
};

class StatementBuilder {
  public:
    static StatementBuilder create(const std::string& struct_name);
};

FunctionBuilder::FunctionBuilder(const std::string& function_name) : name(function_name) {}

FunctionBuilder FunctionBuilder::create(const std::string& function_name) { return FunctionBuilder(function_name); }

FunctionDeclaration FunctionBuilder::build()
{
    std::stringstream function;
    if (!returnType) {
        function << "void";
    }
    else {
        function << returnType->getCode()->code_;
    }
    function << " " << name << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        function << parameters[i].getCode();
        if (i + 1 < parameters.size())
            function << ", ";
    }
    function << "){";

    function << std::endl << "/* variable declarations */" << std::endl;
    for (size_t i = 0; i < variable_declarations.size(); ++i) {
        function << variable_declarations[i].getCode() << ";";
    }
    function << std::endl << "/* statements section */" << std::endl;
    for (size_t i = 0; i < statements.size(); ++i) {
        function << statements[i]->getCode()->code_ << ";";
    }
    function << "}";

    return FunctionDeclaration(function.str());
}

FunctionBuilder& FunctionBuilder::returns(DataTypePtr type)
{
    returnType = type;
    return *this;
}

FunctionBuilder& FunctionBuilder::addParameter(VariableDeclaration var_decl)
{
    parameters.push_back(var_decl);
    return *this;
}
FunctionBuilder& FunctionBuilder::addStatement(StatementPtr statement)
{
    if (statement)
        statements.push_back(statement);
    return *this;
}

FunctionBuilder& FunctionBuilder::addVariableDeclaration(VariableDeclaration vardecl)
{
    variable_declarations.push_back(vardecl);
    return *this;
}

} // namespace iotdb
