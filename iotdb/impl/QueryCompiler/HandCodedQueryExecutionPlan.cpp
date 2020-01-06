/*
 * HandCodedQueryExecutionPlan.cpp
 *
 *  Created on: Dec 19, 2018
 *      Author: zeuchste
 */
#include <boost/serialization/export.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <QueryCompiler/HandCodedQueryExecutionPlan.hpp>
BOOST_CLASS_EXPORT_IMPLEMENT(iotdb::HandCodedQueryExecutionPlan);

#include <QueryCompiler/QueryExecutionPlan.hpp>

namespace iotdb {

HandCodedQueryExecutionPlan::HandCodedQueryExecutionPlan() : QueryExecutionPlan() {}

HandCodedQueryExecutionPlan::~HandCodedQueryExecutionPlan() {}
} // namespace iotdb
