#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <memory>
#include <utility>
#include <vector>
#include "tonlib_multiclient/multi_client.h"
#include "tonlib_multiclient/request.h"
#include "td/utils/Status.h"
#include "tonlib/Logging.h"

namespace py = pybind11;

void set_verbosity_level(int verbosity_level) {
  tonlib::Logging::set_verbosity_level(verbosity_level);
}

PYBIND11_MODULE(tonlib_multiclient, m) {
  m.doc() = "tonlib multi client";

  py::class_<multiclient::MultiClientConfig>(m, "MultiClientConfig")
      .def(
          py::init([](std::string global_config_path,
                      std::optional<std::string> key_store_root,
                      std::string blockchain_name,
                      bool reset_key_store,
                      size_t scheduler_threads) {
            return multiclient::MultiClientConfig{
                .global_config_path = std::move(global_config_path),
                .key_store_root = std::move(key_store_root),
                .blockchain_name = std::move(blockchain_name),
                .reset_key_store = reset_key_store,
                .scheduler_threads = scheduler_threads,
            };
          }),
          py::arg("global_config_path"),
          py::arg("key_store_root") = std::nullopt,
          py::arg("blockchain_name") = "mainnet",
          py::arg("reset_key_store") = false,
          py::arg("scheduler_threads") = 1
      )
      .def_readwrite("global_config_path", &multiclient::MultiClientConfig::global_config_path)
      .def_readwrite("key_store_root", &multiclient::MultiClientConfig::key_store_root)
      .def_readwrite("blockchain_name", &multiclient::MultiClientConfig::blockchain_name)
      .def_readwrite("reset_key_store", &multiclient::MultiClientConfig::reset_key_store)
      .def_readwrite("scheduler_threads", &multiclient::MultiClientConfig::scheduler_threads);

  py::enum_<multiclient::RequestMode>(m, "RequestMode")
      .value("Single", multiclient::RequestMode::Single)
      .value("Broadcast", multiclient::RequestMode::Broadcast)
      .value("Multiple", multiclient::RequestMode::Multiple)
      .export_values();

  py::class_<multiclient::RequestParameters>(m, "RequestParameters")
      .def(
          py::init([](multiclient::RequestMode mode,
                      std::optional<std::vector<size_t>> lite_server_indexes,
                      std::optional<size_t> clients_number,
                      bool archival) {
            return multiclient::RequestParameters{
                .mode = mode,
                .lite_server_indexes = std::move(lite_server_indexes),
                .clients_number = clients_number,
                .archival = archival,
            };
          }),
          py::arg("mode") = multiclient::RequestMode::Broadcast,
          py::arg("lite_server_indexes") = std::nullopt,
          py::arg("clients_number") = std::nullopt,
          py::arg("archival") = false
      )
      .def_readwrite("mode", &multiclient::RequestParameters::mode)
      .def_readwrite("lite_server_indexes", &multiclient::RequestParameters::lite_server_indexes)
      .def_readwrite("clients_number", &multiclient::RequestParameters::clients_number)
      .def_readwrite("archival", &multiclient::RequestParameters::archival);

  py::class_<multiclient::RequestJson>(m, "RequestJson")
      .def(
          py::init([](multiclient::RequestParameters parameters, std::string request) {
            return multiclient::RequestJson{
                .parameters = std::move(parameters),
                .request = std::move(request),
            };
          }),
          py::arg("parameters"),
          py::arg("request")
      )
      .def_readwrite("parameters", &multiclient::RequestJson::parameters)
      .def_readwrite("request", &multiclient::RequestJson::request);

  py::class_<td::Status>(m, "Status").def("is_ok", &td::Status::is_ok).def("to_string", &td::Status::to_string);

  py::class_<td::Result<std::string>>(m, "ResultString")
      .def("is_ok", &td::Result<std::string>::is_ok)
      .def("is_error", &td::Result<std::string>::is_error)
      .def("error", &td::Result<std::string>::error)
      .def("move_as_ok", &td::Result<std::string>::move_as_ok);

  py::class_<multiclient::MultiClient, std::shared_ptr<multiclient::MultiClient>>(m, "MultiClient")
      .def(py::init<multiclient::MultiClientConfig>(), py::arg("config"))
      .def("send_json_request", &multiclient::MultiClient::send_request_json);

  m.def("set_verbosity_level", &set_verbosity_level);
}
