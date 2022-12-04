//**********************************************************************************
//
//   Monitor the status of the Lan Connection and send resulting MQTT messages
//
//   Author: Christian Joly
//   Date: 08/23/2022
//
//**********************************************************************************
#include "corinmqtt.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <ctime>
#include "bcm2835.h"
#include <chrono>

#define pause 15 // Number of seconds to refresh Lan Connection values 
#define Relay_Ch1 26

using namespace std;

std::shared_ptr<spdlog::logger> logger;
CorinMqtt mq;
bool running  = true;

int shed_start;
int shed_end;
bool   shed_enable = false;
bool   shed_ac2    = true;
int start_mn;
int end_mn;

//
// Control C Signal Handler
//
void sigint_callback_handler(int signum) {
printf("Received Control-C\n");
	SPDLOG_LOGGER_INFO(logger, "Caught INT signal: {}",signum);
	running = false;
}

//
// Get time since Epoch
//
uint64_t timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

//
// Init Logger
//
void initLogging() {

	int max_size = 1048576 * 5;
	int max_files = 3;
  	logger = spdlog::rotating_logger_mt("lan_monitor.log", "/home/pi/logs/power_control/power_control.log", max_size, max_files);
  	spdlog::set_default_logger(logger);
  	spdlog::set_pattern("[%c] [%!:%#] %v");            
	spdlog::flush_every(std::chrono::seconds(3));

}

void create_msg(string event, char *value) {

    	Json::Value obj;
    	uint64_t ms = timeSinceEpochMillisec();
    	obj["TIMESTAMP"]     = to_string(ms);
    	obj["EVENT"]         = event;
    	obj["VALUE"]         = value;
    	Json::FastWriter fastWriter;
    	string msg = fastWriter.write(obj);
    	mq.writeMqtt(msg);

}

void disconnectMqttEvent() {

        mq.connect();
        mq.subscribe("/home/shed/ac2");
        mq.subscribe("/home/shed/enable");
        mq.subscribe("/home/shed/start");
        mq.subscribe("/home/shed/end");

}

void receivedMqttEvent(string event, string value) {

	SPDLOG_LOGGER_INFO(logger, " Event: {} Value: {}",event,value);
	if (event.compare(0,16,"/home/shed/start") == 0) {
		try {
			shed_start = stoi(value);
			time_t epoch_start = shed_start;
			struct tm *tm_start = localtime(&epoch_start);
			start_mn = tm_start->tm_hour *  60 + tm_start->tm_min;
		} catch ( ... ) {
         		SPDLOG_LOGGER_ERROR(logger, "Error Converting to float - Start");
         		return;
      		}
	}
	if (event.compare(0,14,"/home/shed/end") == 0) {
		try {
			shed_end = stoi(value);
			time_t epoch_end = shed_end;
			struct tm *tm_end = localtime(&epoch_end);
			end_mn = tm_end->tm_hour *  60 + tm_end->tm_min;
		} catch ( ... ) {
         		SPDLOG_LOGGER_ERROR(logger, "Error Converting to float - End");
         		return;
      		}
	}
	if (event.compare(0,17,"/home/shed/enable") == 0) {
		if (value.compare(0,4,"true") == 0) {
			shed_enable = true;
		} else {
			shed_enable = false;
		}
	}
	if (event.compare(0,14,"/home/shed/ac2") == 0) {
		if (value.compare(0,4,"true") == 0) {
			shed_ac2 = true;
                	bcm2835_gpio_write(Relay_Ch1,HIGH);
		} else {
			shed_ac2 = false;
                	bcm2835_gpio_write(Relay_Ch1,LOW);
		}
	}

}

int main() {

	using namespace std::chrono;

	initLogging();
  	SPDLOG_LOGGER_INFO(logger, "");
  	SPDLOG_LOGGER_INFO(logger, "**************************************************");
  	SPDLOG_LOGGER_INFO(logger, "*                     POWER Control              *");
  	SPDLOG_LOGGER_INFO(logger, "**************************************************");

	signal(SIGINT,  sigint_callback_handler);

        bcm2835_init();
        bcm2835_gpio_fsel(Relay_Ch1,BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_write(Relay_Ch1,HIGH);

	mq.init("/home/shed/cpu","/home/pi/logs/power_control");
	mq.connect();
   	mq.subscribe("/home/shed/ac2");
   	mq.subscribe("/home/shed/enable");
   	mq.subscribe("/home/shed/start");
   	mq.subscribe("/home/shed/end");
   	mq.receivedMqttEvent.connect(&receivedMqttEvent);
   	mq.disconnectMqttEvent.connect(&disconnectMqttEvent);

	// Main Event Loop 
	bool bat_on = false;
	while(running) {

    		std::time_t t = std::time(0);   // get time now
    		std::tm* now = std::localtime(&t);
		int current_mn = now->tm_hour * 60 + now->tm_min;

		if (shed_enable) {
			if ((current_mn >= start_mn) && (current_mn < end_mn)) {
				if (bat_on == false) {
                        		bcm2835_gpio_write(Relay_Ch1,LOW);
					create_msg("BATTERY_STATUS","ON");
					bat_on = true;
				}
			} else {
				if (bat_on == true) {
                        		bcm2835_gpio_write(Relay_Ch1,HIGH);
					create_msg("BATTERY_STATUS","OFF");
					bat_on = false;
				}
			}
		}
      		std::this_thread::sleep_for(std::chrono::milliseconds(pause*1000));
	}

   	mq.destroy();
        bcm2835_close();
  	SPDLOG_LOGGER_INFO(logger, "");
  	SPDLOG_LOGGER_INFO(logger, "**************************************************");
  	SPDLOG_LOGGER_INFO(logger, "*                    End POWER Control           *");
  	SPDLOG_LOGGER_INFO(logger, "**************************************************");

   	return(0);

}
