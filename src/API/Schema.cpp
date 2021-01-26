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

#include <iostream>
#include <stdexcept>

#include <API/Schema.hpp>
#include <Common/DataTypes/DataTypeFactory.hpp>
#include <Common/PhysicalTypes/DefaultPhysicalTypeFactory.hpp>
#include <Common/PhysicalTypes/PhysicalType.hpp>
#include <Util/Logger.hpp>
#include <Util/UtilityFunctions.hpp>

namespace NES {

Schema::Schema() {}

SchemaPtr Schema::create() { return std::make_shared<Schema>(); }

uint64_t Schema::getSize() const { return fields.size(); }

Schema::Schema(const SchemaPtr query) { copyFields(query); }

SchemaPtr Schema::copy() const { return std::make_shared<Schema>(*this); }

/* Return size of one row of schema in bytes. */
uint64_t Schema::getSchemaSizeInBytes() const {
    uint64_t size = 0;
    // todo if we introduce a physical schema.
    auto physicalDataTypeFactory = DefaultPhysicalTypeFactory();
    for (auto const& field : fields) {
        size += physicalDataTypeFactory.getPhysicalType(field->getDataType())->size();
    }
    return size;
}

SchemaPtr Schema::copyFields(SchemaPtr otherSchema) {
    for (AttributeFieldPtr attr : otherSchema->fields) {
        fields.push_back(AttributeField::create(attr->name, attr->dataType));
    }
    this->qualifyingName = otherSchema->qualifyingName;
    return copy();
}

SchemaPtr Schema::addField(AttributeFieldPtr field) {
    if (field) {
        fields.push_back(AttributeField::create(field->name, field->dataType));
    }
    return copy();
}

SchemaPtr Schema::addField(const std::string& name, const BasicType& type) {
    return addField(name, DataTypeFactory::createType(type));
}

SchemaPtr Schema::addField(const std::string& name, DataTypePtr data) { return addField(AttributeField::create(name, data)); }

void Schema::removeField(AttributeFieldPtr field) {
    auto it = fields.begin();
    while (it != fields.end()) {
        if (it->get()->name == field->name) {
            fields.erase(it);
            break;
        }
        it++;
    }
}

void Schema::replaceField(const std::string& name, DataTypePtr type) {
    for (auto i = 0; i < fields.size(); i++) {
        if (fields[i]->name == name) {
            fields[i] = AttributeField::create(name, type);
            return;
        }
    }
}

AttributeFieldPtr Schema::get(const std::string& fieldName) {
    for (auto& field : fields) {
        if (field->name == fieldName) {
            return field;
        }
    }
    NES_FATAL_ERROR("Schema: No field in the schema with the identifier " << fieldName);
    throw std::invalid_argument("field " + fieldName + " does not exist");
}

AttributeFieldPtr Schema::get(uint32_t index) {
    if (index < (uint32_t) fields.size()) {
        return fields[index];
    }
    NES_FATAL_ERROR("Schema: No field in the schema with the id " << index);
    throw std::invalid_argument("field id " + std::to_string(index) + " does not exist");
}

bool Schema::equals(SchemaPtr schema, bool considerOrder) {
    if (schema->fields.size() != fields.size()) {
        return false;
    }
    if (considerOrder) {
        for (int i = 0; i < fields.size(); i++) {
            if (!(fields.at(i)->isEqual((schema->fields).at(i)))) {
                return false;
            }
        }
        return true;
    } else {
        for (AttributeFieldPtr attr : fields) {
            if (!(schema->hasFullyQualifiedFieldName(attr->name) && schema->get(attr->name)->isEqual(attr))) {
                return false;
            }
        }
        return true;
    }
}

const std::string Schema::toString() const {
    std::stringstream ss;
    for (auto& f : fields) {
        ss << f->toString() << " ";
    }
    return ss.str();
}

AttributeFieldPtr createField(std::string name, BasicType type) {
    return AttributeField::create(name, DataTypeFactory::createType(type));
};

bool Schema::contains(const std::string& fieldName) {
    for (const auto& field : this->fields) {
        if (UtilityFunctions::startsWith(field->name, fieldName)) {
            return true;
        }
    }
    return false;
}

uint64_t Schema::getIndex(const std::string& fieldName) {
    int i = 0;
    bool found = false;
    for (const auto& field : this->fields) {
        if (UtilityFunctions::startsWith(field->name, fieldName)) {
            found = true;
            break;
        }
        i++;
    }
    if (found) {
        return i;
    } else {
        return -1;
    }
}

bool Schema::hasFieldName(const std::string& fieldName) {
    return std::any_of(fields.begin(), fields.end(), [&](const AttributeFieldPtr& field) {
        std::string& fullyQualifiedFieldName = field->name;
        auto unqualifiedFieldName = fullyQualifiedFieldName.substr(
            fullyQualifiedFieldName.find(qualifyingName) + qualifyingName.length(), fullyQualifiedFieldName.length());
        return unqualifiedFieldName == fieldName;
    });
}

bool Schema::hasFullyQualifiedFieldName(const std::string& fullyQualifiedFieldName) {
    return std::any_of(fields.begin(), fields.end(), [&fullyQualifiedFieldName](const auto& field) {
        return field->name == fullyQualifiedFieldName;
    });
}

}// namespace NES
