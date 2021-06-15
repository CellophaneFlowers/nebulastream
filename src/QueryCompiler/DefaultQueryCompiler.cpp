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
#include <Nodes/Util/DumpContext.hpp>
#include <Nodes/Util/VizDumpHandler.hpp>
#include <Phases/ConvertLogicalToPhysicalSink.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/DefaultQueryCompiler.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalSourceOperator.hpp>
#include <QueryCompiler/Phases/AddScanAndEmitPhase.hpp>
#include <QueryCompiler/Phases/CodeGenerationPhase.hpp>
#include <QueryCompiler/Phases/PhaseFactory.hpp>
#include <QueryCompiler/Phases/Pipelining/PipeliningPhase.hpp>
#include <QueryCompiler/Phases/Translations/LowerLogicalToPhysicalOperators.hpp>
#include <QueryCompiler/Phases/Translations/LowerPhysicalToGeneratableOperators.hpp>
#include <QueryCompiler/Phases/Translations/LowerToExecutableQueryPlanPhase.hpp>
#include <QueryCompiler/QueryCompilationRequest.hpp>
#include <QueryCompiler/QueryCompilationResult.hpp>
#include <QueryCompiler/QueryCompilerOptions.hpp>
#include <Util/Logger.hpp>

namespace NES::QueryCompilation {

DefaultQueryCompiler::DefaultQueryCompiler(QueryCompilerOptionsPtr const &options, Phases::PhaseFactoryPtr const &phaseFactory)
    : QueryCompiler(options), lowerLogicalToPhysicalOperatorsPhase(phaseFactory->createLowerLogicalQueryPlanPhase(options)),
      lowerPhysicalToGeneratableOperatorsPhase(phaseFactory->createLowerPhysicalToGeneratableOperatorsPhase(options)),
      lowerToExecutableQueryPlanPhase(phaseFactory->createLowerToExecutableQueryPlanPhase(options)),
      pipeliningPhase(phaseFactory->createPipeliningPhase(options)),
      addScanAndEmitPhase(phaseFactory->createAddScanAndEmitPhase(options)),
      codeGenerationPhase(phaseFactory->createCodeGenerationPhase(options)) {}

QueryCompilerPtr DefaultQueryCompiler::create(QueryCompilerOptionsPtr const &options, Phases::PhaseFactoryPtr const &phaseFactory) {
    return std::make_shared<DefaultQueryCompiler>(DefaultQueryCompiler(options, phaseFactory));
}

QueryCompilationResultPtr DefaultQueryCompiler::compileQuery(QueryCompilationRequestPtr request) {

    try {

        auto queryId = request->getQueryPlan()->getQueryId();
        auto subPlanId = request->getQueryPlan()->getQuerySubPlanId();

        // create new context for handling debug output
        auto dumpContext = DumpContext::create("QueryCompilation-" + std::to_string(queryId) + "-" + std::to_string(subPlanId));
        if (request->isDumpEnabled()) {
            dumpContext->registerDumpHandler(VizDumpHandler::create());
        }

        NES_DEBUG("compile query with id: " << queryId << " subPlanId: " << subPlanId);
        auto logicalQueryPlan = request->getQueryPlan();
        dumpContext->dump("1. LogicalQueryPlan", logicalQueryPlan);

        auto physicalQueryPlan = lowerLogicalToPhysicalOperatorsPhase->apply(logicalQueryPlan);
        dumpContext->dump("2. PhysicalQueryPlan", physicalQueryPlan);

        auto pipelinedQueryPlan = pipeliningPhase->apply(physicalQueryPlan);
        dumpContext->dump("3. AfterPipelinedQueryPlan", pipelinedQueryPlan);

        addScanAndEmitPhase->apply(pipelinedQueryPlan);
        dumpContext->dump("4. AfterAddScanAndEmitPhase", pipelinedQueryPlan);

        lowerPhysicalToGeneratableOperatorsPhase->apply(pipelinedQueryPlan);
        dumpContext->dump("5. GeneratableOperators", pipelinedQueryPlan);

        codeGenerationPhase->apply(pipelinedQueryPlan);
        dumpContext->dump("6. ExecutableOperatorPlan", pipelinedQueryPlan);

        auto executableQueryPlan = lowerToExecutableQueryPlanPhase->apply(pipelinedQueryPlan, request->getNodeEngine());
        return QueryCompilationResult::create(executableQueryPlan);
    } catch (const QueryCompilationException& exception) {
        auto currentException = std::current_exception();
        return QueryCompilationResult::create(currentException);
    }
}

}// namespace NES::QueryCompilation