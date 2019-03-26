

#pragma once

#include <string>
#include <memory>

#include <API/ParameterTypes.hpp>
#include <Operators/Operator.hpp>

namespace iotdb {

class KeyByOperator : public Operator {
public:
  KeyByOperator(const Attributes& keyby_spec);
  KeyByOperator(const KeyByOperator& other);
  KeyByOperator& operator = (const KeyByOperator& other);
  void produce(CodeGeneratorPtr codegen, PipelineContextPtr context, std::ostream& out) override;
  void consume(CodeGeneratorPtr codegen, PipelineContextPtr context, std::ostream& out) override;
  const OperatorPtr copy() const override;
  const std::string toString() const override;
  OperatorType getOperatorType() const override;
  ~KeyByOperator() override;
private:
  Attributes keyby_spec_;
};

}

