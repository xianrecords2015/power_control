#include <iostream>
#include <cstdlib>
#include <thread> // For sleep
#include <atomic>
#include <chrono>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <fstream>
#include "MQTTClient.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <jsoncpp/json/json.h>
#include <boost/circular_buffer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2.hpp>

using namespace std;
using namespace boost::filesystem;

class CorinMqtt {

private:
  string _address;
  string _port;
  string _username;
  string _password;

  int        _QOS;
  string     _clientID;
  MQTTClient _client;
  bool _isConnected;
  string _topic;

  void initLogging(string logpath);

public:
  std::shared_ptr<spdlog::logger>  _loggermqtt;
  CorinMqtt();
  ~CorinMqtt();
  void init(string topic,string logpath);
  void destroy();
  int connect();
  void disconnect();
  int publish(string topic,string payload);
  int subscribe(string topic);
  void writeMqtt(string message);
  boost::signals2::signal<void (string, string)> receivedMqttEvent;
  boost::signals2::signal<void ()> disconnectMqttEvent;

};
