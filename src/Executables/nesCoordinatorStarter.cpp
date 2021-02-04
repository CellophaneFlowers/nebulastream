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

/********************************************************
 *     _   _   ______    _____
 *    | \ | | |  ____|  / ____|
 *    |  \| | | |__    | (___
 *    | . ` | |  __|    \___ \     Coordinator
 *    | |\  | | |____   ____) |
 *    |_| \_| |______| |_____/
 *
 ********************************************************/
#include <Components/NesCoordinator.hpp>
#include <Configs/ConfigOption.hpp>
#include <Configs/ConfigOptions/CoordinatorConfig.hpp>
#include <GRPC/CoordinatorRPCServer.hpp>
#include <Util/Logger.hpp>
#include <Util/yaml/Yaml.hh>
#include <Util/yaml/YamlDef.hh>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <iostream>
#include <sys/stat.h>

namespace po = boost::program_options;
using namespace NES;
using namespace std;

const std::string logo = "/********************************************************\n"
                         " *     _   _   ______    _____\n"
                         " *    | \\ | | |  ____|  / ____|\n"
                         " *    |  \\| | | |__    | (___\n"
                         " *    | . ` | |  __|    \\___ \\     Coordinator\n"
                         " *    | |\\  | | |____   ____) |\n"
                         " *    |_| \\_| |______| |_____/\n"
                         " *\n"
                         " ********************************************************/";

int main(int argc, const char* argv[]) {
    std::cout << logo << std::endl;

    // Initializing defaults
    uint16_t restPort = 8081;
    uint16_t rpcPort = 4000;
    uint16_t dataPort = 3001;
    std::string restIp = "127.0.0.1";
    std::string coordinatorIp = "127.0.0.1";
    std::string logLevel = "LOG_DEBUG";
    CoordinatorConfig* coordinatorConfig = new CoordinatorConfig();

    NES::setupLogging("nesCoordinatorStarter.log", NES::getStringAsDebugLevel(logLevel));

    // set the default numberOfSlots to the number of processor
    const auto processorCount = std::thread::hardware_concurrency();
    uint16_t numberOfSlots = processorCount;
    bool enableQueryMerging = false;

    std::string configPath = "";

    po::options_description serverOptions("Nes Coordinator Server Options");

    serverOptions.add_options()("restIp", po::value<std::string>(&restIp)->default_value(restIp),
                                "Set NES ip of the REST server (default: 127.0.0.1).")(
        "coordinatorIp", po::value<std::string>(&coordinatorIp)->default_value(coordinatorIp),
        "Set NES ip for internal communication regarding zmq and rpc (default: 127.0.0.1).")(
        "dataPort", po::value<uint16_t>(&dataPort)->default_value(dataPort), "Set NES data server port (default: 3001).")(
        "restPort", po::value<uint16_t>(&restPort)->default_value(restPort), "Set NES REST server port (default: 8081).")(
        "coordinatorPort", po::value<uint16_t>(&rpcPort)->default_value(rpcPort), "Set NES rpc server port (default: 4000).")(
        "numberOfSlots", po::value<uint16_t>(&numberOfSlots)->default_value(numberOfSlots),
        "Set the computing capacity (default: number of processor).")(
        "enableQueryMerging", po::value<bool>(&enableQueryMerging)->default_value(enableQueryMerging),
        "Enable Query Merging Feature (default: false).")(
        "logLevel", po::value<std::string>(&logLevel)->default_value(logLevel),
        "The log level (LOG_NONE, LOG_WARNING, LOG_DEBUG, LOG_INFO, LOG_TRACE)")(
        "configPath", po::value<std::string>(&configPath)->default_value(configPath),
        "Path to the NES Coordinator Configurations YAML file.")("help", "Display help message");

    /* Parse parameters. */
    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(serverOptions).run(), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        NES_ERROR("NESCOORDINATORSTARTER: Failure while parsing connection parameters!");
        NES_ERROR(e.what());
        return EXIT_FAILURE;
    }

    if (vm.count("help")) {
        NES_INFO("Basic Command Line Parameter ");
        NES_INFO(serverOptions);
        return 0;
    }

    if (!configPath.empty()) {
        NES_INFO("NESWORKERSTARTER: Using config file with path: " << configPath << " .");
        struct stat buffer {};
        if (stat(configPath.c_str(), &buffer) == -1) {
            NES_ERROR("NESWORKERSTARTER: Configuration file not found at: " << configPath);
            return EXIT_FAILURE;
        }

        coordinatorConfig->overwriteConfigWithYAMLFileInput(configPath);
    }

    //NES::setLogLevel(NES::getStringAsDebugLevel(coordinatorConfig->getLogLevel().getValueAsString()));

    NES_INFO("creating coordinator");
    /*NesCoordinatorPtr crd = std::make_shared<NesCoordinator>(coordinatorConfig->getRestIp().getValueAsUint16_t(), coordinatorConfig->getRestPort().getValueAsUint16_t(),
                                                                              coordinatorConfig->getCoordinatorIp().getValueAsString(), coordinatorConfig->getRpcPort().getValueAsUint16_t(),
                                                                              coordinatorConfig->getNumberOfSlots().getValueAsUint16_t(), coordinatorConfig->getEnableQueryMerging().getValueAsBool());*/

    /*NES_INFO("start coordinator with RestIp=" << coordinatorConfig->getRestIp().getValue() << " restPort="
                                                               << coordinatorConfig->getRestPort().getValue()
                                              << " coordinatorIp=" << coordinatorConfig->getCoordinatorIp().getValue() << " with rpc port "
                                              << coordinatorConfig->getRpcPort().getValue()
                                              << " numberOfSlots=" << coordinatorConfig->getNumberOfSlots().getValue());*/
    //    crd->startCoordinator(/**blocking**/ true);//blocking call
    //    crd->stopCoordinator(true);
    NES_INFO("coordinator started");
}
