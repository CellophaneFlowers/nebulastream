

#include <iostream>
#include <sstream>
#include <vector>

#include <Operators/Impl/MapOperator.hpp>
#include <API/UserAPIExpression.hpp>
#include <QueryCompiler/CodeGenerator.hpp>

namespace NES {

MapOperator::MapOperator(AttributeFieldPtr field, PredicatePtr ptr) : predicate_(ptr), field_(field) {}

MapOperator::MapOperator(const MapOperator& other) : predicate_(other.predicate_), field_(other.field_) {}

MapOperator& MapOperator::operator=(const MapOperator& other)
{
   if (this != &other) {
        predicate_ = NES::copy(other.predicate_);
        field_ = other.field_->copy();
    }
    return *this;
}

void MapOperator::produce(CodeGeneratorPtr codegen, PipelineContextPtr context, std::ostream& out)
{
    childs[0]->produce(codegen, context, out);
}
void MapOperator::consume(CodeGeneratorPtr codegen, PipelineContextPtr context, std::ostream& out)
{
    codegen->generateCode(field_, predicate_, context, out);
    parent->consume(codegen, context, out);
}

const OperatorPtr MapOperator::copy() const
{
    return std::make_shared<MapOperator>(*this);
}

const std::string MapOperator::toString() const
{
    std::stringstream ss;
    ss << "MAP_UDF(" << field_->toString() << " = " << predicate_->toString() << ")";
    return ss.str();
}

OperatorType MapOperator::getOperatorType() const
{
    return MAP_OP;
}

MapOperator::~MapOperator() {}

const OperatorPtr createMapOperator(AttributeFieldPtr field, PredicatePtr ptr)
{
    return std::make_shared<MapOperator>(field, ptr);
}

} // namespace NES
