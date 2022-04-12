/*
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

#include <Operators/AbstractOperators/OriginIdAssignmentOperator.hpp>
#include <Operators/LogicalOperators/LogicalOperatorNode.hpp>
#include <Optimizer/QueryRewrite/OriginIdInferenceRule.hpp>
#include <Windowing/LogicalJoinDefinition.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>
#include <Exceptions/RuntimeException.hpp>

namespace NES::Optimizer {

OriginIdInferenceRule::OriginIdInferenceRule() {}

OriginIdInferenceRulePtr OriginIdInferenceRule::create() {
    return std::make_shared<OriginIdInferenceRule>(OriginIdInferenceRule());
}

QueryPlanPtr OriginIdInferenceRule::apply(QueryPlanPtr queryPlan) {
    // query local origin ids, always start from 1 to n, whereby n is the number of operators that assign new orin ids
    uint64_t originIdCounter = 1;
    // set origin id for all operators of type OriginIdAssignmentOperator. For example, window, joins and sources.
    for (auto originIdAssignmentOperators : queryPlan->getOperatorByType<OriginIdAssignmentOperator>()) {
        originIdAssignmentOperators->setOriginId(originIdCounter++);
    }

    // propagate origin ids through the complete query plan
    for (auto rootOperators : queryPlan->getRootOperators()) {
        if (auto logicalOperator = rootOperators->as_if<LogicalOperatorNode>()) {
            logicalOperator->inferInputOrigins();
        } else {
            throw Exceptions::RuntimeException("During OriginIdInferenceRule all root operators have to be LogicalOperatorNodes");
        }
    }
    return queryPlan;
}

}// namespace NES::Optimizer