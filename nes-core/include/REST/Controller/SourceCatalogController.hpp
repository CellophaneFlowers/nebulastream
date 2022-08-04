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

#ifndef NES_INCLUDE_REST_CONTROLLER_STREAMCATALOGCONTROLLER_HPP_
#define NES_INCLUDE_REST_CONTROLLER_STREAMCATALOGCONTROLLER_HPP_

#include <REST/Controller/BaseController.hpp>
#include <REST/CpprestForwardedRefs.hpp>

namespace NES {

namespace Catalogs::Source {
class SourceCatalog;
using SourceCatalogPtr = std::shared_ptr<SourceCatalog>;
}// namespace Catalogs

class SourceCatalogController : public BaseController {

  public:
    explicit SourceCatalogController(Catalogs::Source::SourceCatalogPtr sourceCatalog);

    void handleGet(const std::vector<utility::string_t>& path, web::http::http_request& request) override;
    void handlePost(const std::vector<utility::string_t>& path, web::http::http_request& message) override;
    void handleDelete(const std::vector<utility::string_t>& path, web::http::http_request& request) override;

  private:
    Catalogs::Source::SourceCatalogPtr sourceCatalog;
};
using SourceCatalogControllerPtr = std::shared_ptr<SourceCatalogController>;

}// namespace NES

#endif// NES_INCLUDE_REST_CONTROLLER_STREAMCATALOGCONTROLLER_HPP_
