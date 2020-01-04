#include <Operators/OperatorJsonUtil.hpp>
#include <Operators/Operator.hpp>

namespace iotdb {

OperatorJsonUtil::OperatorJsonUtil() {};

OperatorJsonUtil::~OperatorJsonUtil() {};

json::value OperatorJsonUtil::getBasePlan(InputQueryPtr inputQuery) {

  json::value result{};
  std::vector<json::value> nodes{};
  std::vector<json::value> edges{};
  const OperatorPtr &root = inputQuery->getRoot();

  if (!root) {
    auto node = json::value::object();
    node["id"] = json::value::string("NONE");
    node["title"] = json::value::string("NONE");
    nodes.push_back(node);
  } else {

    auto node = json::value::object();
    node["id"] = json::value::string(
        operatorTypeToString[root->getOperatorType()] + "(OP-" + std::to_string(root->operatorId) + ")");
    node["title"] =
        json::value::string(
            operatorTypeToString[root->getOperatorType()] + +"(OP-" + std::to_string(root->operatorId) + ")");
    if (root->getOperatorType() == OperatorType::SOURCE_OP ||
        root->getOperatorType() == OperatorType::SINK_OP) {
      node["nodeType"] = json::value::string("Source");
    } else {
      node["nodeType"] = json::value::string("Processor");
    }
    nodes.push_back(node);
    getChildren(root, nodes, edges);
  }

  result["nodes"] = json::value::array(nodes);
  result["edges"] = json::value::array(edges);

  return result;
}

void OperatorJsonUtil::getChildren(const OperatorPtr &root, std::vector<json::value> &nodes,
                                   std::vector<json::value> &edges) {

  std::vector<json::value> childrenNode;

  std::vector<OperatorPtr> &children = root->childs;
  if (children.empty()) {
    return;
  }

  for (OperatorPtr child : children) {
    auto node = json::value::object();
    node["id"] =
        json::value::string(
            operatorTypeToString[child->getOperatorType()] + "(OP-" + std::to_string(child->operatorId) + ")");
    node["title"] =
        json::value::string(
            operatorTypeToString[child->getOperatorType()] + "(OP-" + std::to_string(child->operatorId) + ")");

    if (child->getOperatorType() == OperatorType::SOURCE_OP ||
        child->getOperatorType() == OperatorType::SINK_OP) {
      node["nodeType"] = json::value::string("Source");
    } else {
      node["nodeType"] = json::value::string("Processor");
    }

    nodes.push_back(node);

    auto edge = json::value::object();
    edge["source"] =
        json::value::string(
            operatorTypeToString[child->getOperatorType()] + "(OP-" + std::to_string(child->operatorId) + ")");
    edge["target"] =
        json::value::string(
            operatorTypeToString[root->getOperatorType()] + "(OP-" + std::to_string(root->operatorId) + ")");

    edges.push_back(edge);
    getChildren(child, nodes, edges);
  }
}
}