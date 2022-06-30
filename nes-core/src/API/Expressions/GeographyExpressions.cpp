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

#include <API/Expressions/Expressions.hpp>
#include <API/Expressions/GeographyExpressions.hpp>
#include <Nodes/Expressions/ConstantValueExpressionNode.hpp>
#include <Nodes/Expressions/FieldAccessExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/ShapeExpressions/ShapeExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/ShapeExpressions/CircleExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/ShapeExpressions/PointExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/ShapeExpressions/PolygonExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/ShapeExpressions/RectangleExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/STDWithinExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/STKnnExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/STWithinExpressionNode.hpp>
#include <Nodes/Expressions/GeographyExpressions/GeographyFieldsAccessExpressionNode.hpp>


namespace NES {

ShapeExpressionNodePtr CIRCLE(double latitude,
                              double longitude,
                              double distance) {
    auto shapeExpressionNode = CircleExpressionNode::create(latitude, longitude, distance);
    return shapeExpressionNode;
}

ShapeExpressionNodePtr POINT(double latitude,
                             double longitude) {
    auto shapeExpressionNode = PointExpressionNode::create(latitude, longitude);
    return shapeExpressionNode;
}

ShapeExpressionNodePtr RECTANGLE(double latitude_low,
                                 double longitude_low,
                                 double latitude_high,
                                 double longitude_high) {
    auto shapeExpressionNode =
        RectangleExpressionNode::create(latitude_low,
                                        longitude_low,
                                        latitude_high,
                                        longitude_high);
    return shapeExpressionNode;
}

ShapeExpressionNodePtr POLYGON(std::initializer_list<double> coords) {
    auto shapeExpressionNode = PolygonExpressionNode::create(coords);
    return shapeExpressionNode;
}

ExpressionNodePtr ST_WITHIN(const ExpressionItem& latitudeFieldName,
                            const ExpressionItem& longitudeFieldName,
                            const ShapeExpressionNodePtr& shapeExpression) {
    // GeographyFieldsAccessExpressionNode for latitude and longitude fields
    auto latitudeExpression = latitudeFieldName.getExpressionNode();
    if (!latitudeExpression->instanceOf<FieldAccessExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_WITHIN): latitude has to be an FieldAccessExpression but it was a " + latitudeExpression->toString());
    }
    auto latitudeAccess = latitudeExpression->as<FieldAccessExpressionNode>();

    auto longitudeExpression = longitudeFieldName.getExpressionNode();
    if (!longitudeExpression->instanceOf<FieldAccessExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_WITHIN): latitude has to be an FieldAccessExpression but it was a " + longitudeExpression->toString());
    }
    auto longitudeAccess = longitudeExpression->as<FieldAccessExpressionNode>();

    auto geographyFieldsAccess = GeographyFieldsAccessExpressionNode::create(latitudeAccess, longitudeAccess);

    auto shapeType = shapeExpression->getShapeType();
    if(shapeType != ShapeType::Circle && shapeType != ShapeType::Polygon && shapeType != ShapeType::Rectangle) {
        NES_ERROR("Spatial Query(ST_WITHIN): Shape has to be a Circle, Polygon or a Rectangle but it was a " + shapeExpression->toString());
    }

    if(shapeType == ShapeType::Polygon || shapeType == ShapeType::Rectangle){
        return STWithinExpressionNode::create(std::move(geographyFieldsAccess->as<GeographyFieldsAccessExpressionNode>()),
                                              std::move(shapeExpression));
    }

    // in case the shape is a circle create an ST_DWithin expression instead
    auto circle = shapeExpression->as<CircleExpressionNode>();
    return STDWithinExpressionNode::create(std::move(geographyFieldsAccess->as<GeographyFieldsAccessExpressionNode>()),
                                           std::move(circle));
}

ExpressionNodePtr ST_DWITHIN(const ExpressionItem& latitudeFieldName,
                             const ExpressionItem& longitudeFieldName,
                             const ShapeExpressionNodePtr& shapeExpression) {
    // GeographyFieldsAccessExpressionNode for latitude and longitude fields
    auto latitudeExpression = latitudeFieldName.getExpressionNode();
    if (!latitudeExpression->instanceOf<FieldAccessExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_DWITHIN): latitude has to be an FieldAccessExpression but it was a " + latitudeExpression->toString());
    }
    auto latitudeAccess = latitudeExpression->as<FieldAccessExpressionNode>();

    auto longitudeExpression = longitudeFieldName.getExpressionNode();
    if (!longitudeExpression->instanceOf<FieldAccessExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_DWITHIN): latitude has to be an FieldAccessExpression but it was a " + longitudeExpression->toString());
    }
    auto longitudeAccess = longitudeExpression->as<FieldAccessExpressionNode>();

    auto geographyFieldsAccess = GeographyFieldsAccessExpressionNode::create(latitudeAccess, longitudeAccess);

    auto geographyPointAccess = geographyFieldsAccess->as<GeographyFieldsAccessExpressionNode>();

    auto circle = shapeExpression->as<CircleExpressionNode>();

    if (!circle->instanceOf<CircleExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_DWITHIN): Shape has to be a CircleExpression but it was a " + circle->toString());
    }

    return STDWithinExpressionNode::create(std::move(geographyFieldsAccess->as<GeographyFieldsAccessExpressionNode>()),
                                           std::move(circle));
}

ExpressionNodePtr ST_KNN(const ExpressionItem& latitudeFieldName,
                         const ExpressionItem& longitudeFieldName,
                         const ShapeExpressionNodePtr& queryPoint,
                         const ExpressionItem& k) {
    // Throw not implemented exception. ST_KNN requires more deliberate thinking.
    NES_THROW_RUNTIME_ERROR("Spatial Query(ST_KNN): ST_KNN not supported at the moment.");

    // GeographyFieldsAccessExpressionNode for latitude and longitude fields
    auto latitudeExpression = latitudeFieldName.getExpressionNode();
    if (!latitudeExpression->instanceOf<FieldAccessExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_KNN): latitude has to be an FieldAccessExpression but it was a " + latitudeExpression->toString());
    }
    auto latitudeAccess = latitudeExpression->as<FieldAccessExpressionNode>();

    auto longitudeExpression = longitudeFieldName.getExpressionNode();
    if (!longitudeExpression->instanceOf<FieldAccessExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_KNN): latitude has to be an FieldAccessExpression but it was a " + longitudeExpression->toString());
    }
    auto longitudeAccess = longitudeExpression->as<FieldAccessExpressionNode>();

    auto geographyFieldsAccess = GeographyFieldsAccessExpressionNode::create(latitudeAccess, longitudeAccess);

    // ConstantValueExpressionNode for the wkt string
    auto queryPointType = queryPoint->getShapeType();
    if (!queryPoint->instanceOf<PointExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_KNN): the query point has to be PointExpressionNode but it was a " + queryPoint->toString());
    }

    // ConstantValueExpressionNode for the parameter k
    auto kExpression = k.getExpressionNode();
    if (!kExpression->instanceOf<ConstantValueExpressionNode>()) {
        NES_ERROR("Spatial Query(ST_KNN): the parameter k has to be an ConstantValueExpression but it was a " + kExpression->toString());
    }
    auto kConstantValueExpressionNode = kExpression->as<ConstantValueExpressionNode>();

    return STKnnExpressionNode::create(std::move(geographyFieldsAccess->as<GeographyFieldsAccessExpressionNode>()),
                                       std::move(queryPoint),
                                       std::move(kConstantValueExpressionNode));
}

}// namespace NES
