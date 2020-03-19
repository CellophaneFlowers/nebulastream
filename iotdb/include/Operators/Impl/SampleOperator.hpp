#pragma once

#include <memory>
#include <string>

#include <API/ParameterTypes.hpp>
#include <Operators/Operator.hpp>

namespace NES {

class SampleOperator : public Operator {
 public:
  SampleOperator() = default;

  SampleOperator(const std::string& udfs);
  SampleOperator(const SampleOperator& other);
  SampleOperator& operator=(const SampleOperator& other);
  void produce(CodeGeneratorPtr codegen , PipelineContextPtr context ,
               std::ostream& out) override;
  void consume(CodeGeneratorPtr codegen , PipelineContextPtr context ,
               std::ostream& out) override;
  const OperatorPtr copy() const override;
  const std::string toString() const override;
  OperatorType getOperatorType() const override;
  virtual bool equals(const Operator& _rhs) override;
  ~SampleOperator() override;

 private:
  std::string udfs;

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar , unsigned) {
    ar
        & BOOST_SERIALIZATION_BASE_OBJECT_NVP(
            Operator) & BOOST_SERIALIZATION_NVP(udfs);
  }
};

}  // namespace NES
