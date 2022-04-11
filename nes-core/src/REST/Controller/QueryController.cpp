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

#include <Components/NesCoordinator.hpp>
#include <Exceptions/InvalidArgumentException.hpp>
#include <Exceptions/InvalidQueryStatusException.hpp>
#include <Exceptions/QueryNotFoundException.hpp>
#include <GRPC/Serialization/QueryPlanSerializationUtil.hpp>
#include <Plans/Global/Execution/GlobalExecutionPlan.hpp>
#include <Plans/Utils/PlanJsonGenerator.hpp>
#include <REST/Controller/QueryController.hpp>
#include <SerializableQueryPlan.pb.h>
#include <Services/QueryCatalogService.hpp>
#include <Services/QueryService.hpp>
#include <Util/Logger/Logger.hpp>
#include <cpprest/http_msg.h>
#include <log4cxx/helpers/exception.h>
#include <utility>

namespace NES {

const std::string DEFAULT_TOLERANCE_TYPE = "NONE";

QueryController::QueryController(QueryServicePtr queryService,
                                 QueryCatalogServicePtr queryCatalogService,
                                 GlobalExecutionPlanPtr globalExecutionPlan)
    : queryService(std::move(queryService)), queryCatalogService(std::move(queryCatalogService)),
      globalExecutionPlan(std::move(globalExecutionPlan)) {}

void QueryController::handleGet(const std::vector<utility::string_t>& path, web::http::http_request& request) {

    auto parameters = getParameters(request);

    if (path[1] == "execution-plan") {
        NES_INFO("QueryController:: GET execution-plan");

        auto const queryParameter = parameters.find("queryId");
        if (queryParameter == parameters.end()) {
            NES_ERROR("QueryController: Unable to find query ID for the GET execution-plan request");
            web::json::value errorResponse{};
            errorResponse["detail"] = web::json::value::string("Parameter queryId must be provided");
            badRequestImpl(request, errorResponse);
            return;
        }
        try {
            // get the queryId from user input
            QueryId queryId = std::stoi(queryParameter->second);
            NES_DEBUG("QueryController:: execution-plan requested queryId: " << queryId);
            // get the execution-plan for given query id
            auto executionPlanJson = PlanJsonGenerator::getExecutionPlanAsJson(globalExecutionPlan, queryId);
            NES_DEBUG("QueryController:: execution-plan: " << executionPlanJson.serialize());
            //Prepare the response
            successMessageImpl(request, executionPlanJson);
            return;
        } catch (...) {
            NES_ERROR("RestServer: Unable to start REST server unknown exception.");
            internalServerErrorImpl(request);
        }

    } else if (path[1] == "query-plan") {
        //Check if the path contains the query id
        auto param = parameters.find("queryId");
        if (param == parameters.end()) {
            NES_ERROR("QueryController: Unable to find query ID for the GET query-plan request");
            web::json::value errorResponse{};
            errorResponse["detail"] = web::json::value::string("Parameter queryId must be provided");
            badRequestImpl(request, errorResponse);
            return;
        }

        try {
            // get the queryId from user input
            QueryId queryId = std::stoi(param->second);

            //Call the service
            QueryCatalogEntryPtr queryCatalogEntry = queryCatalogService->getEntryForQuery(queryId);

            NES_DEBUG("UtilityFunctions: Getting the json representation of the query plan");
            auto basePlan = PlanJsonGenerator::getQueryPlanAsJson(queryCatalogEntry->getInputQueryPlan());

            //Prepare the response
            successMessageImpl(request, basePlan);
            return;
        } catch (const std::exception& exc) {
            NES_ERROR("QueryController: handleGet -query-plan: Exception occurred while building the query plan for user "
                      "request:"
                      << exc.what());
            handleException(request, exc);
            return;
        } catch (...) {
            NES_ERROR("RestServer: Unable to start REST server unknown exception.");
            internalServerErrorImpl(request);
        }
    } else if (path[1] == "optimization-phases") {
        //Check if the path contains the query id
        auto param = parameters.find("queryId");
        if (param == parameters.end()) {
            NES_ERROR("QueryController: Unable to find query ID for the GET optimization-phases request");
            web::json::value errorResponse{};
            errorResponse["detail"] = web::json::value::string("Parameter queryId must be provided");
            badRequestImpl(request, errorResponse);
            return;
        }

        try {
            // get the queryId from user input
            QueryId queryId = std::stoi(param->second);
            NES_DEBUG("Query Controller: Get the registered query");
            QueryCatalogEntryPtr queryCatalogEntry = queryCatalogService->getEntryForQuery(queryId);
            auto optimizationPhases = queryCatalogEntry->getOptimizationPhases();

            NES_DEBUG("UtilityFunctions: Getting the json representation of the optimized query plans");
            auto response = web::json::value::object();
            for (auto const& [phaseName, queryPlan] : optimizationPhases) {
                auto queryPlanJson = PlanJsonGenerator::getQueryPlanAsJson(queryPlan);
                response[phaseName] = queryPlanJson;
            }

            //Prepare the response
            successMessageImpl(request, response);
            return;
        } catch (const std::exception& exc) {
            NES_ERROR("QueryController: handleGet -query-plan: Exception occurred while building the query plan for user "
                      "request:"
                      << exc.what());
            handleException(request, exc);
            return;
        } catch (...) {
            NES_ERROR("RestServer: Unable to start REST server unknown exception.");
            internalServerErrorImpl(request);
        }
    } else if (path[1] == "query-status") {
        NES_INFO("QueryController:: GET query-status");

        auto const queryParameter = parameters.find("queryId");

        if (queryParameter == parameters.end()) {
            NES_ERROR("QueryController: Unable to find query ID for the GET query-status request");
            web::json::value errorResponse{};
            errorResponse["detail"] = web::json::value::string("Parameter queryId must be provided");
            badRequestImpl(request, errorResponse);
            return;
        }
        try {
            // get the queryId from user input
            QueryId queryId = std::stoi(queryParameter->second);
            //Call the service
            QueryCatalogEntryPtr queryCatalogEntry = queryCatalogService->getEntryForQuery(queryId);

            NES_DEBUG("QueryController:: Getting the json representation of status: queryId="
                      << queryId << " status=" << queryCatalogEntry->getQueryStatusAsString());
            web::json::value result{};
            auto node = web::json::value::object();
            // use the id of the root operator to fill the id field
            node["status"] = web::json::value::string(queryCatalogEntry->getQueryStatusAsString());
            //Prepare the response
            successMessageImpl(request, node);
            return;
        } catch (...) {
            NES_ERROR("RestServer: Unable to start REST server unknown exception.");
            internalServerErrorImpl(request);
        }
    } else {
        resourceNotFoundImpl(request);
    }
}

void QueryController::handlePost(const std::vector<utility::string_t>& path, web::http::http_request& message) {

    if (path[1] == "execute-query") {

        NES_DEBUG(" QueryController: Trying to execute query");

        message.extract_string(true)
            .then([this, message](utility::string_t body) {
                try {
                    //Prepare Input query from user string
                    std::string userRequest(body.begin(), body.end());
                    NES_DEBUG("QueryController: handlePost -execute-query: Request body: " << userRequest
                                                                                           << "try to parse query");
                    web::json::value req = web::json::value::parse(userRequest);
                    NES_DEBUG("QueryController: handlePost -execute-query: get user query");
                    std::string userQuery;
                    if (req.has_field("userQuery")) {
                        userQuery = req.at("userQuery").as_string();
                    } else {
                        NES_ERROR("QueryController: handlePost -execute-query: Wrong key word for user query, use 'userQuery'.");
                    }
                    std::string optimizationStrategyName = req.at("strategyName").as_string();
                    std::string faultToleranceString = DEFAULT_TOLERANCE_TYPE;
                    std::string lineageString = DEFAULT_TOLERANCE_TYPE;
                    if (req.has_field("faultTolerance")) {
                        std::string faultToleranceString = req.at("faultTolerance").as_string();
                    }
                    if (req.has_field("lineage")) {
                        lineageString = req.at("lineage").as_string();
                    }
                    auto faultToleranceMode = stringToFaultToleranceTypeMap(faultToleranceString);
                    if (faultToleranceMode == FaultToleranceType::INVALID) {
                        web::json::value errorResponse{};
                        errorResponse["detail"] = web::json::value::string("QueryController: " + faultToleranceString);
                        badRequestImpl(message, errorResponse);
                        throw log4cxx::helpers::Exception("QueryController: " + faultToleranceString);
                    }
                    auto lineageMode = stringToLineageTypeMap(lineageString);
                    if (lineageMode == LineageType::INVALID) {
                        web::json::value errorResponse{};
                        errorResponse["detail"] = web::json::value::string("QueryController: " + lineageString);
                        badRequestImpl(message, errorResponse);
                        throw log4cxx::helpers::Exception("QueryController: " + lineageString);
                    }
                    NES_DEBUG("QueryController: handlePost -execute-query: Params: userQuery= " << userQuery << ", strategyName= "
                                                                                                << optimizationStrategyName);
                    QueryId queryId = queryService->validateAndQueueAddRequest(userQuery,
                                                                               optimizationStrategyName,
                                                                               faultToleranceMode,
                                                                               lineageMode);

                    //Prepare the response
                    web::json::value restResponse{};
                    restResponse["queryId"] = web::json::value::number(queryId);
                    successMessageImpl(message, restResponse);
                    return;
                } catch (const std::exception& exc) {
                    NES_ERROR("QueryController: handlePost -execute-query: Exception occurred while building the query plan for "
                              "user request:"
                              << exc.what());
                    handleException(message, exc);
                    return;
                } catch (...) {
                    NES_ERROR("RestServer: Unable to start REST server unknown exception.");
                    internalServerErrorImpl(message);
                }
            })
            .wait();
    } else if (path[1] == "execute-query-ex") {
        message.extract_string(true)
            .then([this, message](utility::string_t body) {
                try {
                    NES_DEBUG("QueryController: handlePost -execute-query: Request body: " << body);
                    // decode string into protobuf message
                    std::shared_ptr<SubmitQueryRequest> protobufMessage = std::make_shared<SubmitQueryRequest>();

                    if (!protobufMessage->ParseFromArray(body.data(), body.size())) {
                        web::json::value errorResponse{};
                        errorResponse["detail"] = web::json::value::string("QueryController: Invalid Protobuf message");
                        badRequestImpl(message, errorResponse);
                        throw log4cxx::helpers::Exception("QueryController: Invalid Protobuf message");
                    }
                    // decode protobuf message into c++ obj repr
                    SerializableQueryPlan* queryPlanSerialized = protobufMessage->mutable_queryplan();
                    QueryPlanPtr queryPlan(QueryPlanSerializationUtil::deserializeQueryPlan(queryPlanSerialized));

                    std::string* queryString = protobufMessage->mutable_querystring();
                    auto* context = protobufMessage->mutable_context();
                    if (!context->contains("placement")) {
                        web::json::value errorResponse{};
                        errorResponse["detail"] =
                            web::json::value::string("QueryController: No placement strategy found in query string");
                        badRequestImpl(message, errorResponse);
                        throw log4cxx::helpers::Exception("QueryController: No placement strategy found in query string");
                    }
                    std::string placementStrategy = context->at("placement").value();
                    std::string faultToleranceString = DEFAULT_TOLERANCE_TYPE;
                    std::string lineageString = DEFAULT_TOLERANCE_TYPE;
                    if (context->contains("faultTolerance")) {
                        faultToleranceString = context->at("faultTolerance").value();
                    }
                    if (context->contains("lineage")) {
                        lineageString = context->at("lineage").value();
                    }
                    auto faultToleranceMode = stringToFaultToleranceTypeMap(faultToleranceString);
                    if (faultToleranceMode == FaultToleranceType::INVALID) {
                        web::json::value errorResponse{};
                        errorResponse["detail"] = web::json::value::string("QueryController: " + faultToleranceString);
                        badRequestImpl(message, errorResponse);
                        throw log4cxx::helpers::Exception("QueryController: " + faultToleranceString);
                    }
                    auto lineageMode = stringToLineageTypeMap(lineageString);
                    if (lineageMode == LineageType::INVALID) {
                        web::json::value errorResponse{};
                        errorResponse["detail"] = web::json::value::string("QueryController: " + lineageString);
                        badRequestImpl(message, errorResponse);
                        throw log4cxx::helpers::Exception("QueryController: " + lineageString);
                    }

                    QueryId queryId =
                        queryService->addQueryRequest(*queryString, queryPlan, placementStrategy, faultToleranceMode, lineageMode);

                    web::json::value restResponse{};
                    restResponse["queryId"] = web::json::value::number(queryId);
                    successMessageImpl(message, restResponse);
                    return;
                } catch (const std::exception& exc) {
                    NES_ERROR("QueryController: handlePost -execute-query: Exception occurred while building the query plan for "
                              "user request:"
                              << exc.what());
                    handleException(message, exc);
                    return;
                } catch (...) {
                    NES_ERROR("RestServer: Unable to start REST server unknown exception.");
                    internalServerErrorImpl(message);
                }
            })
            .wait();
    } else {
        resourceNotFoundImpl(message);
    }
}// namespace NES

void QueryController::handleDelete(const std::vector<utility::string_t>& path, web::http::http_request& request) {

    //Extract parameters if any
    auto parameters = getParameters(request);

    if (path[1] == "stop-query") {
        NES_DEBUG("QueryController: Request received for stoping a query");
        //Check if the path contains the query id
        auto param = parameters.find("queryId");
        if (param == parameters.end()) {
            NES_ERROR("QueryController: Unable to find query Id for the stop-query request");
            web::json::value errorResponse{};
            errorResponse["detail"] = web::json::value::string("Parameter queryId must be provided");
            errorResponse["queryId"] = web::json::value::null();
            badRequestImpl(request, errorResponse);
            return;
        }

        try {
            QueryId queryId = std::stoi(param->second);
            NES_DEBUG("QueryController: Prepare Input query from user string: " << std::to_string(queryId));

            bool success = queryService->validateAndQueueStopRequest(queryId);
            //Prepare the response
            web::json::value result{};
            result["success"] = web::json::value::boolean(success);
            result["queryId"] = web::json::value::number(queryId);
            successMessageImpl(request, result);
            return;
        } catch (QueryNotFoundException const& exc) {
            NES_ERROR("QueryCatalogController: handleDelete -query: Exception occurred while building the query plan for "
                      "user request:"
                      << exc.what());
            handleException(request, exc);
            return;
        } catch (InvalidQueryStatusException const& exc) {
            NES_ERROR("QueryCatalogController: handleDelete -query: Exception occurred while building the query plan for "
                      "user request:"
                      << exc.what());
            web::json::value errorResponse{};
            errorResponse["detail"] = web::json::value::string(exc.what());
            errorResponse["queryId"] = web::json::value::number(std::stoi(param->second));
            badRequestImpl(request, errorResponse);
            return;
        } catch (...) {
            NES_ERROR("QueryCatalogController: unknown exception.");
            internalServerErrorImpl(request);
        }
    } else {
        resourceNotFoundImpl(request);
    }
}

}// namespace NES
