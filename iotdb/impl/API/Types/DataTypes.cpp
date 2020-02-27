#include <API/Types/DataTypes.hpp>
#include <QueryCompiler/CCodeGenerator/Statement.hpp>
#include <QueryCompiler/CCodeGenerator/BinaryOperatorStatement.hpp>

namespace NES {

DataType::DataType(const DataType &) {

}

DataType &DataType::operator=(const DataType &) {
  return *this;
}

const StatementPtr DataType::getStmtCopyAssignment(const AssignmentStatment &aParam) const {
  return VarRef(aParam.lhs_tuple_var)[VarRef(aParam.lhs_index_var)].accessRef(VarRef(aParam.lhs_field_var))
      .assign(VarRef(aParam.rhs_tuple_var)[VarRef(aParam.rhs_index_var)].accessRef(VarRef(aParam.rhs_field_var)))
      .copy();
}

DataType::~DataType() {}

} // namespace NES

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/access.hpp>
BOOST_CLASS_EXPORT(NES::DataType);
//BOOST_CLASS_EXPORT(NES::ArrayDataType);
//BOOST_CLASS_EXPORT(NES::ArrayValueType);
