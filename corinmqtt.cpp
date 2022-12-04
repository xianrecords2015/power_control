#include "corinmqtt.h"

CorinMqtt::CorinMqtt() {
  cout << "CorinMqtt Object is being created: " << endl;
}

CorinMqtt::~CorinMqtt() {
  cout << "CorinMqtt Object is being deleted" << endl;
}

void conlost(void *context, char *cause) {
    CorinMqtt *m = (CorinMqtt *)context;
    char msg[400];
    sprintf(msg,"Connection Lost - cause:%s",cause);
    SPDLOG_LOGGER_INFO(m->_loggermqtt, "{}", msg);
    m->disconnect();
    m->disconnectMqttEvent();
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {

   CorinMqtt *m = (CorinMqtt *)context;

   char msg[400];
   sprintf(msg,"Topic: %s \t Message: %.*s", topicName, message->payloadlen, (char*) message->payload);
   SPDLOG_LOGGER_INFO(m->_loggermqtt, "{}", msg);

   string t(topicName);
   string p((char *)message->payload);
   m->receivedMqttEvent(t,p);

   MQTTClient_freeMessage(&message);
   MQTTClient_free(topicName);
   return 1;

}

void CorinMqtt::init(string topic,string logpath) {

  initLogging(logpath);

  SPDLOG_LOGGER_INFO(_loggermqtt, "*******************************************************");
  SPDLOG_LOGGER_INFO(_loggermqtt, "*                    CorinMqtt                        *");
  SPDLOG_LOGGER_INFO(_loggermqtt, "*******************************************************");

  // Init Variables
  _QOS = 2;
  _topic = topic;

  // Read Config File
  std::ifstream indata;
  string line;
  indata.open("/home/pi/power_control/mqtt_config.dat", ios::in);
  if(!indata) {
    SPDLOG_LOGGER_ERROR(_loggermqtt, "Error Config File Could not be opened");
  } else {
    while (!std::getline(indata, line).eof()) {
      vector<string> v;
      boost::split(v, line, boost::is_any_of(":"));
      if (v.size() != 2) continue;
        string vv = boost::erase_all_copy(v[1], " ");
        if (v[0].compare(0,7,"ADDRESS") == 0) {
          _address = vv;
        } else if (v[0].compare(0,4,"PORT") == 0) {
          _port = vv;
        } else if (v[0].compare(0,8,"USERNAME") == 0) {
          _username = vv;
        } else if (v[0].compare(0,8,"PASSWORD") == 0) {
          _password = vv;
        }
    }
    indata.close();
  }

  SPDLOG_LOGGER_INFO(_loggermqtt, "Initialized MQTT Object");

} 

void CorinMqtt::destroy() {

  disconnect();
  SPDLOG_LOGGER_INFO(_loggermqtt, "Destroyed MQTT Object");
  SPDLOG_LOGGER_INFO(_loggermqtt, "*******************************************************");
  SPDLOG_LOGGER_INFO(_loggermqtt, "*                  End CorinMqtt                      *");
  SPDLOG_LOGGER_INFO(_loggermqtt, "*******************************************************");

}

//
// Init Logger
//
void CorinMqtt::initLogging(string logpath) {

  int max_size = 1048576 * 5;
  int max_files = 3;
  _loggermqtt = spdlog::rotating_logger_mt("corinmqtt_log", logpath  + "/corinmqtt.log", max_size, max_files);

}

//
// PRIVATE
//

int CorinMqtt::connect() {

    SPDLOG_LOGGER_INFO(_loggermqtt, "Initializing MQTT Server {}", _address);

    string url = _address + ":" + _port;

    // Generate Random Client ID
    unsigned seed = time(0);
    srand(seed);
    int id = rand() % 10000;  
    _clientID = "corinmqtt_" + to_string(id);

    // Create MQTT Client
    int rc;
    if ((rc = MQTTClient_create(&_client, url.c_str(), _clientID.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
      SPDLOG_LOGGER_ERROR(_loggermqtt, "Failed to create MQTT Server,return code: {}", rc);
      return(-1);
    }
    SPDLOG_LOGGER_INFO(_loggermqtt, "MQTT Client {} Created", _clientID);


    // Setup Connection Options
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.username = _username.c_str();
    conn_opts.password = _password.c_str();
    conn_opts.keepAliveInterval = 60;
    MQTTClient_setCallbacks(_client, this, conlost, messageArrived, NULL);

    // Connect to MQTT Server
    if ((rc = MQTTClient_connect(_client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
      SPDLOG_LOGGER_ERROR(_loggermqtt, "Failed to connect to MQTT Server,return code: {}", rc);
      return(-1);
    }

    SPDLOG_LOGGER_INFO(_loggermqtt, "MQTT Client {} Connected", _clientID);

    _isConnected = true;

    return(0);

}

int CorinMqtt::publish(string topic,string payload) {

    if (!_isConnected) return -1;

    // Initialize Message
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    // Create Payload
    pubmsg.payload     = (char *)payload.c_str();
    pubmsg.payloadlen  = payload.length();
    pubmsg.qos         = _QOS;
    pubmsg.retained    = 0;
    MQTTClient_deliveryToken token;

    // Publish Message
    int rc;
    if ((rc = MQTTClient_publishMessage(_client, topic.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
      SPDLOG_LOGGER_ERROR(_loggermqtt, "Failed to publish to MQTT Server,return code: {}", rc);
      return -1;
    }
    MQTTClient_waitForCompletion(_client, token, 1000L);

    return 0;

}


void CorinMqtt::disconnect() {

	// Disconnect
	int timeout = 100;
	MQTTClient_disconnect(_client,timeout);
   	MQTTClient_destroy(&_client);
	_isConnected = false;

}

void CorinMqtt::writeMqtt(string message) {
 	Json::Value root;
     	// Convert it to Json
      	Json::Reader reader;
      	bool parseSuccess = reader.parse(message, root, false);
      	if (!parseSuccess) {
        	SPDLOG_LOGGER_ERROR(_loggermqtt, "MQTT Message not in Json format {}", message);
        	return;
      	}
      	// Publish Message
      	if ( publish(_topic,message) != 0 ) {
        	SPDLOG_LOGGER_ERROR(_loggermqtt, "MQTT Failed to publish Connection Failed");
      	} else {
        	SPDLOG_LOGGER_INFO(_loggermqtt, "Published Topic: {} Message: {}",_topic,message);
      	}
}


int CorinMqtt::subscribe(string topic) {
	if (!_isConnected) return -1;
     	if (MQTTClient_subscribe(_client, topic.c_str(), 0) != MQTTCLIENT_SUCCESS) {
        	SPDLOG_LOGGER_INFO(_loggermqtt,"Failed to subscribe to topic {}", topic);
		return -1;
      	}
	return 0;
}
