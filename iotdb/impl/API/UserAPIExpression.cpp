/**
 * UserAPIExpression class file
 * 
 * */

#include <string>
#include <sstream>

#include <CodeGen/CodeGen.hpp>
#include <CodeGen/C_CodeGen/BinaryOperatorStatement.hpp>
#include <CodeGen/C_CodeGen/Declaration.hpp>
#include <CodeGen/C_CodeGen/FileBuilder.hpp>
#include <CodeGen/C_CodeGen/FunctionBuilder.hpp>
#include <CodeGen/C_CodeGen/Statement.hpp>
#include <CodeGen/C_CodeGen/UnaryOperatorStatement.hpp>

#include <API/UserAPIExpression.hpp>

#include "../../include/CodeGen/DataTypes.hpp"

namespace iotdb
{

    Predicate::Predicate(const BinaryOperatorType& op, const UserAPIExpressionPtr left, const UserAPIExpressionPtr right, const std::string& functionCallOverload, bool bracket) :
            _op(op),
            _left(left),
            _right(right),
            _bracket(bracket),
            _functionCallOverload(functionCallOverload)
    {}

	Predicate::Predicate(const BinaryOperatorType& op, const UserAPIExpressionPtr left, const UserAPIExpressionPtr right, bool bracket) :
		_op(op),
		_left(left),
		_right(right),
		_bracket(bracket),
		_functionCallOverload("")
	{}
	
	UserAPIExpressionPtr Predicate::copy() const{
		return std::make_shared<Predicate>(*this);
	}

    const ExpressionStatmentPtr Predicate::generateCode(GeneratedCode& code) const{
		if(_functionCallOverload.empty()) {
		    if (_bracket) return BinaryOperatorStatement(*(_left->generateCode(code)), _op, *(_right->generateCode(code)), BRACKETS).copy();
            return BinaryOperatorStatement(*(_left->generateCode(code)), _op, *(_right->generateCode(code))).copy();
        } else {
		    std::stringstream str;
            FunctionCallStatement expr = FunctionCallStatement(_functionCallOverload);
            expr.addParameter(_left->generateCode(code));
            expr.addParameter(_right->generateCode(code));
            if(_bracket) return BinaryOperatorStatement(expr , _op, (ConstantExprStatement((createBasicTypeValue(BasicType::UINT8, "0")))), BRACKETS).copy();
            return BinaryOperatorStatement(expr , _op, (ConstantExprStatement((createBasicTypeValue(BasicType::UINT8, "0"))))).copy();
        }
	}



    const ExpressionStatmentPtr PredicateItem::generateCode(GeneratedCode& code) const{
		if(_attribute){
		    //toDo: Need an equals operator instead of true
		    if(code.struct_decl_input_tuple.getField(_attribute->name) &&
		            code.struct_decl_input_tuple.getField(_attribute->name)->getType() == _attribute->getDataType()){
                      VariableDeclaration var_decl_attr = code.struct_decl_input_tuple.getVariableDeclaration(_attribute->name);
                      return ((VarRef(code.var_decl_input_tuple)[VarRef(*code.var_decl_id)]).accessRef(VarRef(var_decl_attr))).copy();
		      } else{
			IOTDB_FATAL_ERROR("Could not Retrieve Attribute from StructDeclaration!");
		      }
		}else if(_value){
		    return ConstantExprStatement(_value).copy();
		  }else{
		    IOTDB_FATAL_ERROR("PredicateItem has only NULL Pointers!");
		  }
	}
	
	const std::string Predicate::toString() const{
			std::stringstream stream;
			if(_bracket) stream << "(";
			stream << _left->toString() << " " << ::iotdb::toCodeExpression(_op)->code_ << " " << _right->toString() << " ";
			if(_bracket) stream << ")";
			return stream.str();
	}
	
	PredicateItem::PredicateItem(AttributeFieldPtr attribute) : 
	_mutation(PredicateItemMutation::ATTRIBUTE),
	_attribute(attribute) {}
	
	PredicateItem::PredicateItem(ValueTypePtr value) : 
	_mutation(PredicateItemMutation::VALUE),
	_value(value) {}

    PredicateItem::PredicateItem(int8_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::INT8, std::to_string(val))){}
    PredicateItem::PredicateItem(uint8_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::UINT8, std::to_string(val))) {}
    PredicateItem::PredicateItem(int16_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::INT16, std::to_string(val))) {}
    PredicateItem::PredicateItem(uint16_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::UINT16, std::to_string(val))) {}
    PredicateItem::PredicateItem(int32_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::INT32, std::to_string(val))) {}
    PredicateItem::PredicateItem(uint32_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::UINT32, std::to_string(val))) {}
    PredicateItem::PredicateItem(int64_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::INT64, std::to_string(val))) {}
    PredicateItem::PredicateItem(uint64_t val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::UINT64, std::to_string(val))) {}
    PredicateItem::PredicateItem(float val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::FLOAT32, std::to_string(val))) {}
    PredicateItem::PredicateItem(double val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::FLOAT64, std::to_string(val))) {}
    PredicateItem::PredicateItem(bool val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::BOOLEAN, std::to_string(val))) {}
    PredicateItem::PredicateItem(char val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createBasicTypeValue(BasicType::CHAR, std::to_string(val))) {}
    PredicateItem::PredicateItem(const char* val) :
            _mutation(PredicateItemMutation::VALUE),
            _value(createStringValueType(val)) {}

	const std::string PredicateItem::toString() const{
			switch(_mutation)
			{
				case PredicateItemMutation::ATTRIBUTE:
					return _attribute->toString();
				case PredicateItemMutation::VALUE:
					return _value->getCodeExpression()->code_;
			}
			return "";
	}

	const bool PredicateItem::isStringType() const {
	    return (getDataTypePtr()->isCharDataType()) && (getDataTypePtr()->isArrayDataType());
	}

	const DataTypePtr PredicateItem::getDataTypePtr() const {
        if(_attribute) return _attribute->getDataType();
        return _value->getType();
    };

	UserAPIExpressionPtr PredicateItem::copy() const{
		return std::make_shared<PredicateItem>(*this);
	}

    Field::Field(AttributeFieldPtr field) : PredicateItem(field), _name(field->name) {}


    const PredicatePtr createPredicate(const UserAPIExpression& expression){
        PredicatePtr value = std::dynamic_pointer_cast<Predicate>(expression.copy());
        if(!value){
            IOTDB_FATAL_ERROR("UserAPIExpression is not a predicate")
        }
        return value;
    }

    Predicate operator == (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
        return Predicate(BinaryOperatorType::EQUAL_OP, lhs.copy(), rhs.copy());
    }
	Predicate operator != (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::UNEQUAL_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator > (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::GREATER_THEN_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator < (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::LESS_THEN_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator >= (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::GREATER_THEN_OP , lhs.copy(), rhs.copy());
	}
	Predicate operator <= (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::LESS_THEN_EQUAL_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator + (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::PLUS_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator - (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::MINUS_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator * (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::MULTIPLY_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator / (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::DIVISION_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator % (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::MODULO_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator && (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::LOGICAL_AND_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator || (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::LOGICAL_OR_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator & (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::BITWISE_AND_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator | (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::BITWISE_OR_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator ^ (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::BITWISE_XOR_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator << (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::BITWISE_LEFT_SHIFT_OP, lhs.copy(), rhs.copy());
	}
	Predicate operator >> (const UserAPIExpression &lhs, const UserAPIExpression &rhs){
		return Predicate(BinaryOperatorType::BITWISE_RIGHT_SHIFT_OP, lhs.copy(), rhs.copy());
	}

    Predicate operator== (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return (lhs == dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator != (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator !=(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator > (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator >(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator < (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator <(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator >= (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator >=(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator <= (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator <=(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator + (const UserAPIExpression &lhs, const PredicateItem &rhs){
        if(rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator +(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator - (const UserAPIExpression &lhs, const PredicateItem &rhs){
        if(rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator -(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator * (const UserAPIExpression &lhs, const PredicateItem &rhs){
        if(rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator *(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator / (const UserAPIExpression &lhs, const PredicateItem &rhs){
        if(rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator /(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator % (const UserAPIExpression &lhs, const PredicateItem &rhs){
        if(rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator %(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator && (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator &&(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator || (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator ||(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator | (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator |(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator ^ (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator ^(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator << (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator <<(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator >> (const UserAPIExpression &lhs, const PredicateItem &rhs){
        return operator >>(lhs,dynamic_cast<const UserAPIExpression&>(rhs));
    }

    Predicate operator== (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return (dynamic_cast<const UserAPIExpression&>(lhs) == rhs);
    }
    Predicate operator != (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator !=(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator > (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator >(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator < (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator <(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator >= (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator >=(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator <= (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator <=(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator + (const PredicateItem &lhs, const UserAPIExpression &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator +(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator - (const PredicateItem &lhs, const UserAPIExpression &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator -(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator * (const PredicateItem &lhs, const UserAPIExpression &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator *(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator / (const PredicateItem &lhs, const UserAPIExpression &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator /(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator % (const PredicateItem &lhs, const UserAPIExpression &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))) IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator %(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator && (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator &&(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator || (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator ||(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator | (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator |(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator ^ (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator ^(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator << (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator <<(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }
    Predicate operator >> (const PredicateItem &lhs, const UserAPIExpression &rhs){
        return operator >>(dynamic_cast<const UserAPIExpression&>(lhs),rhs);
    }

    /**
     * Operator overload includes String compare by define a function-call-overload in the code generation process
     * @param lhs
     * @param rhs
     * @return
     */
    Predicate operator== (const PredicateItem &lhs, const PredicateItem &rhs){
        //possible use of memcmp when arraytypes equal with length is equal...
        int checktype = lhs.isStringType();
        checktype += rhs.isStringType();
        if(checktype == 1) IOTDB_ERROR("NOT COMPARABLE TYPES")
        if(checktype == 2) return Predicate(BinaryOperatorType::EQUAL_OP, lhs.copy(),rhs.copy(), "strcmp", false);
        return (dynamic_cast<const UserAPIExpression&>(lhs) == dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator != (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator !=(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator > (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator >(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator < (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator <(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator >= (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator >=(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator <= (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator <=(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator + (const PredicateItem &lhs, const PredicateItem &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))
            || rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR)))
            IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator +(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator - (const PredicateItem &lhs, const PredicateItem &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))
            || rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR)))
            IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator -(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator * (const PredicateItem &lhs, const PredicateItem &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))
            || rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR)))
            IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator *(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator / (const PredicateItem &lhs, const PredicateItem &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))
            || rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR)))
            IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator /(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator % (const PredicateItem &lhs, const PredicateItem &rhs){
        if(lhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR))
            || rhs.getDataTypePtr()->isEqual(createDataType(BasicType::CHAR)))
            IOTDB_ERROR("NOT A NUMERICAL VALUE")
        return operator %(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator && (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator &&(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator || (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator ||(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator | (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator |(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator ^ (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator ^(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator << (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator <<(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
    Predicate operator >> (const PredicateItem &lhs, const PredicateItem &rhs){
        return operator >>(dynamic_cast<const UserAPIExpression&>(lhs),dynamic_cast<const UserAPIExpression&>(rhs));
    }
} //end namespace iotdb
