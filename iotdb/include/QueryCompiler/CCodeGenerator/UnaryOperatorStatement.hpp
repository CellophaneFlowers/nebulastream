

#pragma once

#include <memory>
#include <string>

#include <Operators/OperatorTypes.hpp>
#include <QueryCompiler/CCodeGenerator/Statement.hpp>
#include <QueryCompiler/CodeExpression.hpp>

#include <Util/ErrorHandling.hpp>
#include <API/Types/DataTypes.hpp>

namespace iotdb {

const CodeExpressionPtr toCodeExpression(const UnaryOperatorType& type);

class UnaryOperatorStatement : public ExpressionStatment {
  public:
    UnaryOperatorStatement(const ExpressionStatment& expr, const UnaryOperatorType& op,
                           BracketMode bracket_mode = NO_BRACKETS);

    virtual StatementType getStamentType() const;

    virtual const CodeExpressionPtr getCode() const;

    virtual const ExpressionStatmentPtr copy() const;

    virtual ~UnaryOperatorStatement();

  private:
    ExpressionStatmentPtr expr_;
    UnaryOperatorType op_;
    BracketMode bracket_mode_;
};

UnaryOperatorStatement operator&(const ExpressionStatment& ref);

UnaryOperatorStatement operator*(const ExpressionStatment& ref);

UnaryOperatorStatement operator++(const ExpressionStatment& ref);

UnaryOperatorStatement operator--(const ExpressionStatment& ref);

UnaryOperatorStatement operator~(const ExpressionStatment& ref);

UnaryOperatorStatement operator!(const ExpressionStatment& ref);

UnaryOperatorStatement sizeOf(const ExpressionStatment& ref);

} // namespace iotdb
