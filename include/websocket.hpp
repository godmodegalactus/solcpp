#pragma once

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;             // from <nlohmann/json.hpp>
using Callback = std::function<void(const json &)>;  // callback function which takes
                                               // the json and returns nothing

/// @brief Class to store the request and function given by user
struct RequestContent {
  // id will be the id for subscription while id+1 will be id for unsubscription
  int id;
  // subscription and unsubscription method
  std::string subscribe_method;
  std::string unsubscribe_method;

  // callback to call in case of notification
  Callback cb;
  // params to be passed with the subscription string
  json params;

  /// @brief constructor
  RequestContent(int id, std::string subscribe_method,
                 std::string unsubscribe_method, Callback cb, json params);

  /// @brief Get the json request to do subscription
  /// @return the json that can be used to make subscription
  json get_subscription_request();

  /// @brief Json data to write to unsubscribe
  /// @param subscription_id the id to subscribe for
  /// @return request json for unsubscription
  json get_unsubscription_request(int subscription_id);
};

// used to create a session to read and write to websocket
class session : public std::enable_shared_from_this<session> {
 public:
  /// @brief resolver and websocket require an io context to do io operations
  /// @param ioc
  explicit session(net::io_context &ioc, int timeout = 30);

  /// @brief Looks up the domain name to make connection to -> calls on_resolve
  /// @param host the host address
  /// @param port port number
  /// @param text ping message
  void run(std::string host, std::string port);

  /// @brief push a function for subscription
  /// @param req the request to call
  void subscribe(std::shared_ptr<RequestContent> &req);

  /// @brief push for unsubscription
  /// @param id the id to unsubscribe on
  void unsubscribe(int id);

  /// @brief disconnect from browser
  void disconnect();

  /// @brief check if connection has been stablished
  /// @return if connection has been established
  bool connection_established();

 private:
  /// @brief log error messages
  /// @param ec the error code recieved from websocket
  /// @param what a text to log with error
  void fail(beast::error_code ec, char const *what);

  /// @brief Used to connect to the provided host -> calls on_connect
  /// @param ec error code in case of the resolve
  /// @param results the result of the resolve
  void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

  /// @brief Initiates handshake for the websocket connection -> calls
  /// on_handshake
  /// @param ec the error code during connect
  /// @param
  void on_connect(beast::error_code ec,
                  tcp::resolver::results_type::endpoint_type);

  /// @brief Send the ping message once the handshake has been complete calls ->
  /// on_read
  /// @param ec error code on handshake
  void on_handshake(beast::error_code ec);

  /// @brief Deals with the message recieved from server, calls itself to keep
  /// listening to more messages
  /// @param ec error code while reading
  /// @param bytes_transferred Amount of byte recieved
  void on_read(beast::error_code ec, std::size_t bytes_transferred);

  /// @brief function to add callback using the respons of subscription
  /// @param data the reponse of subscription from server
  void add_callback(json &data);

  /// @brief function to remove callback when unsubscribing
  /// @param data the data recieved from websocket
  void remove_callback(json &data);

  /// @brief call the specified callback
  /// @param data the data recieved from websocket
  void call_callback(json &data);

  /// @brief close the connection from websocket
  /// @param ec the error code
  void on_close(beast::error_code ec);

  // resolves the host and port provided by user
  tcp::resolver resolver;

  // socket used by client to read and write
  websocket::stream<beast::tcp_stream> ws;

  // buffer to store data
  beast::flat_buffer buffer;

  // host
  std::string host;

  // map of subscription id with callback
  std::unordered_map<int, std::shared_ptr<RequestContent>> callback_map;

  // The RequestContent which was used to just subscribe
  std::shared_ptr<RequestContent> latest_sub = NULL;

  // The id which was used to unsubscribe
  int latest_unsub = -1;

  // denotes if the connection is up
  bool is_connected = false;

  // request content id with subscription map
  std::unordered_map<int, int> id_sub_map;

  // connection timeout
  int connection_timeout = 30;
};