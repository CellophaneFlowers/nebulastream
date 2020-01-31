#include "REST/Controller/QueryCatalogController.hpp"

namespace NES {

void QueryCatalogController::handleGet(std::vector<utility::string_t> path, web::http::http_request message) {
    if (path[1] == "queries") {

        message.extract_string(true)
            .then([this, message](utility::string_t body) {
                    try {
                        //Prepare Input query from user string
                        std::string payload(body.begin(), body.end());
                        json::value req = json::value::parse(payload);
                        std::string queryStatus = req.at("status").as_string();

                        //Prepare the response
                        json::value result{};
                        map<string, string> queries = queryCatalogServicePtr->getQueriesWithStatus(queryStatus);

                        for (auto[key, value] :  queries) {
                            result[key] = json::value::string(value);
                        }

                        successMessageImpl(message, result);
                        return;
                    } catch (...) {
                        std::cout << "Exception occurred while building the query plan for user request.";
                        internalServerErrorImpl(message);
                        return;
                    }
                  }
            )
            .wait();

    } else if (path[1] == "allRegisteredQueries") {

        message.extract_string(true)
            .then([this, message](utility::string_t body) {
                    try {
                        //Prepare Input query from user string
                        std::string payload(body.begin(), body.end());
                        json::value req = json::value::parse(payload);
                        std::string queryStatus = req.at("status").as_string();

                        //Prepare the response
                        json::value result{};
                        map<string, string> queries = queryCatalogServicePtr->getAllRegisteredQueries();

                        for (auto[key, value] :  queries) {
                            result[key] = json::value::string(value);
                        }

                        successMessageImpl(message, result);
                        return;
                    } catch (...) {
                        std::cout << "Exception occurred while building the query plan for user request.";
                        internalServerErrorImpl(message);
                        return;
                    }
                  }
            )
            .wait();
    }

    resourceNotFoundImpl(message);
}

void QueryCatalogController::setCoordinatorActorHandle(infer_handle_from_class_t<CoordinatorActor> coordinatorActorHandle) {
    this->coordinatorActorHandle = coordinatorActorHandle;
}

void QueryCatalogController::handleDelete(std::vector<utility::string_t> path, web::http::http_request message) {

    if (path[1] == "query") {

        message.extract_string(true)
            .then([this, message](utility::string_t body) {
                    try {
                        //Prepare Input query from user string
                        std::string payload(body.begin(), body.end());
                        json::value req = json::value::parse(payload);
                        std::string queryId = req.at("queryId").as_string();

                        //Perform async call for deleting the query using actor
                        //Note: This is an async call and would not know if the deletion has failed
                        CoordinatorActorConfig actorCoordinatorConfig;
                        actorCoordinatorConfig.load<io::middleman>();
                        //Prepare Actor System
                        actor_system actorSystem{actorCoordinatorConfig};
                        scoped_actor self{actorSystem};

                        self->request(coordinatorActorHandle,
                                      task_timeout,
                                      deregister_query_atom::value,
                                      queryId);

                        //Prepare the response
                        json::value result{};
                        successMessageImpl(message, result);
                        return;
                    } catch (...) {
                        std::cout << "Exception occurred while building the query plan for user request.";
                        internalServerErrorImpl(message);
                        return;
                    }
                  }
            )
            .wait();

    }

    resourceNotFoundImpl(message);
}

}
