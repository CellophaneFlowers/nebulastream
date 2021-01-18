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

#include <API/Schema.hpp>
#include <Operators/LogicalOperators/MergeLogicalOperatorNode.hpp>
#include <Optimizer/Utils/QuerySignatureUtil.hpp>
#include <Util/Logger.hpp>

namespace NES {

MergeLogicalOperatorNode::MergeLogicalOperatorNode(OperatorId id) : UnaryOperatorNode(id) {}

bool MergeLogicalOperatorNode::isIdentical(NodePtr rhs) const {
    return equal(rhs) && rhs->as<MergeLogicalOperatorNode>()->getId() == id;
}

const std::string MergeLogicalOperatorNode::toString() const {
    std::stringstream ss;
    ss << "Merge(" << id << ")";
    return ss.str();
}

std::string MergeLogicalOperatorNode::getStringBasedSignature() {
    std::stringstream ss;
    std::vector<std::string> fields;
    for (auto& field : outputSchema->fields) {
        fields.push_back(field->name);
    }
    std::sort(fields.begin(), fields.end());
    ss << "MERGE(";
    for (auto field : fields) {
        ss << " " << field << " ";
    }
    ss << ")";
    ss << ".(" << children[0]->as<LogicalOperatorNode>()->getStringBasedSignature() + ").";
    ss << children[1]->as<LogicalOperatorNode>()->getStringBasedSignature();
    return ss.str();
}

bool MergeLogicalOperatorNode::inferSchema() {
    if (!UnaryOperatorNode::inferSchema()) {
        return false;
    }
    if (getChildren().size() < 2) {
        NES_THROW_RUNTIME_ERROR("MergeLogicalOperator: merge need two child operators.");
        return false;
    }

    std::vector<SchemaPtr> schemas;
    for (auto& child : children) {
        if (child->instanceOf<UnaryOperatorNode>()) {
            auto op = child->as<UnaryOperatorNode>();
            schemas.push_back(op->getOutputSchema());
        }
    }

    //test if all schemas are the same
    if (!all_of(schemas.begin(), schemas.end(), [&](SchemaPtr i) {
            return i->equals(schemas[0]);
        })) {
        NES_ERROR("MergeLogicalOperator: the two input streams have different schema.");
        return false;
    } else {
        return true;
    }
}

OperatorNodePtr MergeLogicalOperatorNode::copy() {
    auto copy = LogicalOperatorFactory::createMergeOperator(id);
    copy->setInputSchema(inputSchema);
    copy->setOutputSchema(outputSchema);
    return copy;
}

bool MergeLogicalOperatorNode::equal(const NodePtr rhs) const {
    if (rhs->instanceOf<MergeLogicalOperatorNode>()) {
        return true;
    }
    return false;
}

}// namespace NES