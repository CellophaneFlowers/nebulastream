/*
    Copyright (C) 2020 by the NebulaStream project (https://nebula.stream)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef NES_WATERMARKASSIGNERLOGICALOPERATORNODE_HPP
#define NES_WATERMARKASSIGNERLOGICALOPERATORNODE_HPP

#include <Operators/AbstractOperators/Arity/UnaryOperatorNode.hpp>
#include <Operators/AbstractOperators/AbstractWatermarkAssignerOperator.hpp>
#include <Operators/LogicalOperators/LogicalUnaryOperatorNode.hpp>
#include <Operators/LogicalOperators/LogicalOperatorForwardRefs.hpp>


namespace NES {

class WatermarkAssignerLogicalOperatorNode : public AbstractWatermarkAssignerOperator, public LogicalUnaryOperatorNode {
  public:
    WatermarkAssignerLogicalOperatorNode(const Windowing::WatermarkStrategyDescriptorPtr watermarkStrategyDescriptor,
                                         OperatorId id);
    bool equal(const NodePtr rhs) const override;

    bool isIdentical(NodePtr rhs) const override;

    const std::string toString() const override;

    OperatorNodePtr copy() override;
    std::string getStringBasedSignature() override;
    bool inferSchema() override;
};

typedef std::shared_ptr<WatermarkAssignerLogicalOperatorNode> WatermarkAssignerLogicalOperatorNodePtr;

}// namespace NES

#endif//NES_WATERMARKASSIGNERLOGICALOPERATORNODE_HPP
