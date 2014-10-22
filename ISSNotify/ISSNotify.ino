/**
* @file   ISSNotify.ino
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Arduino sketch to activate a light when the International 
* Space Station is above the current loctation.
*
* @details This is an Arduino sketch that communicates with an ENC28J60 
* ethernet interface to fetch the current date and time, the approximate 
* location via IP address reverse lookup, and a list of future passes of
* the International Space Station above the current location. When the
* ISS is above, the Arduino activates an output connected to a lamp.
*
* @code   
* ENC    PATA    Arduino
* VCC 	  wire 2  3.3v
* GND 	  wire 1  GND
* SCK 	  wire 5  Pin 13
* SO 	  wire 7  Pin 12
* SI 	  wire 6  Pin 11
* CS 	  wire 4  Pin 8
* @endcode
* 
* @todo refactor to use structs
* @bug Unexplained behaviour: function ether.browseUrl seems to make its request from the ip identified by ether.dnsLookup(website_name), which is called in my function get_ip_via_dns, instead of from the address passed to it.
*/
#include <stdint.h>
#include <avr/pgmspace.h>
#include <EtherCard.h>
#include "DataProcessing.h"
#include "Printing.h"
#include "Networking.h"
#include "States.h"
#include "Definitions.h"

//constants
#define         REQUEST_RATE 20000 // milliseconds
#define         URL_BUFF_SIZE 75 //Size of buffer for variable portion of URLs
#define         ETHERCARD_BUFF_SIZE 750 //Size of buffer for ethercard library. Should be at least 750 to fit all data returned by geo-ip API
//output pins
int             error_led = 7;
int             lamp = 6;
//global variables
static          uint32_t timer;
char            url_var_part[URL_BUFF_SIZE];
uint8_t         Ethernet::buffer[ETHERCARD_BUFF_SIZE];

//system states 
typedef enum {
	STATE_GET_DATA = 0,
	STATE_PROCESS_DATA = 1,
	STATE_CALCULATE = 2,
	STATE_LIGHT_LAMP = 3,
	STATE_SLEEP = 4
} state_t;
typedef enum {
	STATE_GET_DATA_GEOIP_LAT = 0,
	STATE_GET_DATA_GEOIP_LONG = 1,
	STATE_GET_DATA_TIME = 2,
	STATE_GET_DATA_ISS_RISETIME = 3,
	STATE_GET_DATA_ISS_DURATION = 4
} state_data_t;
state_t state = STATE_GET_DATA;
state_data_t state_data = STATE_GET_DATA_GEOIP_LAT;
state_cb_t state_cb = STATE_CB_WAIT; //declared in States.h

//keywords and addresses for data acquisition
prog_char           website_name_geoip[] PROGMEM = "freegeoip.net"; //website names must be in PROGMEM for ethercard lib
prog_char           website_name_iss[] PROGMEM = "api.open-notify.org";
uint8_t             website_ip_geoip[4] = {0,0,0,0};
uint8_t             website_ip_iss[4] = {0,0,0,0};
#define RETURN_LEN_GEOIPLAT 8 //return can be as long as "-90.0000"
static const char   keyword_geoiplat[]="latitude\":"; //keyword to use when searching for latitude
static char         geoiplat_string[RETURN_LEN_GEOIPLAT+1]="";
#define RETURN_LEN_GEOIPLONG 9 //return can be as long as "-180.0000"
static const char   keyword_geoiplong[]="longitude\":"; //keyword to use when searching for longitude
static char         geoiplong_string[RETURN_LEN_GEOIPLONG+1]="";
#define RETURN_LEN_TIME 10 //time in unix timestamp format (10 digits)
static const char   keyword_time[]="datetime\": "; //keyword to use when searching
static char         time_of_request_string[RETURN_LEN_TIME+1];
static uint32_t     time_of_request=0;
#define RETURN_LEN_ISSDURATION 4 
static const char   keyword_issduration[]="duration\": "; //keyword to use when searching
static char         issduration_string[RETURN_LEN_ISSDURATION+1];
static uint32_t     issduration=0;
#define RETURN_LEN_ISSRISETIME 10 //time in unix timestamp format (10 digits)
static const char   keyword_issrisetime[]="risetime\": "; //keyword to use when searching
static char         issrisetime_string[RETURN_LEN_ISSRISETIME+1];
static uint32_t     issrisetime=0;

static uint8_t  mymac2[] = {0x74,0x69,0x69,0x2D,0x30,0x36};
uint8_t init_DHCP2(uint8_t *ethernet_buffer){
	if (ether.begin(sizeof ethernet_buffer, mymac2) == 0){
		print_p(PSTR("Failed to access Ethernet controller\n"));
		return 1;
	} 
	else if (!ether.dhcpSetup()){
		print_p(PSTR("DHCP failed\n"));
		return 2;
	}
	else {  
		ether.printIp("My IP: ", ether.myip);
		ether.printIp("GW IP: ", ether.gwip);
		ether.printIp("DNS IP: ", ether.dnsip); 
		return 0;
	}
}

void setup () {
	pinMode(error_led, OUTPUT);
	pinMode(lamp, OUTPUT);
	Serial.begin(57600);
	print_p(PSTR("\n[ISS Notification Lamp]\n"));
	print_p(PSTR("Marc Khouri - marc.khouri.ca - github.com/mnkhouri\n"));

	
	if (ether.begin(sizeof Ethernet::buffer, mymac2) == 0) 
    Serial.println( "Failed to access Ethernet controller");

  Serial.println("Setting up DHCP");
  if (!ether.dhcpSetup())
    Serial.println( "DHCP failed");
  
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
	
	/*

	while(init_DHCP2(Ethernet::buffer)){  //continue initializing DHCP until success (return value=0)
		delay(1000);
		Serial.print("uh-oh");
		digitalWrite(error_led, HIGH); 
	}
	digitalWrite(error_led, LOW); */

	uint8_t ip_initialized=1;
	while(ip_initialized){ //check that all websites are reacheable by using DNS to get their IP address. When get_ip_via_dns() returns 0, DNS lookup was successful.
		print_p(PSTR("Requesting site IPs via DNS\n"));
		ip_initialized = 0;
		ip_initialized += get_ip_via_dns(website_name_geoip, website_ip_geoip);
		ip_initialized += get_ip_via_dns(website_name_iss, website_ip_iss);
		digitalWrite(error_led, HIGH);
	}
	digitalWrite(error_led, LOW);

	state = STATE_GET_DATA; //ensure state machine starts in the proper state
	state_data = STATE_GET_DATA_GEOIP_LAT;
}

void loop () {
	ether.packetLoop(ether.packetReceive());
	uint32_t millis_at_time_of_request;
	int32_t millis_until_risetime;

	switch(state){
	case STATE_GET_DATA:
		switch(state_data){
		case STATE_GET_DATA_GEOIP_LAT: //same request as GEOIP_LONG
		case STATE_GET_DATA_GEOIP_LONG:
			//request format http://freegeoip.net/json/
			timer = millis();
			print_p(PSTR("\n>>> REQUEST GEOIP DATA\n"));
			get_ip_via_dns(website_name_geoip, website_ip_geoip); //unexplained behaviour: ether.browseUrl makes a request to the domain that was last called by ether.dnsLookup, not the ip address passed to it as an argument
			ether.browseUrl(PSTR("/json/"), "", website_name_geoip, ethercard_callback);
			state_cb = STATE_CB_WAIT;
			state = STATE_PROCESS_DATA;
			break;
		case STATE_GET_DATA_TIME: //same request as ISS_RISETIME
		case STATE_GET_DATA_ISS_DURATION: //same request as ISS_RISETIME
		case STATE_GET_DATA_ISS_RISETIME:
			// request format http://api.open-notify.org/iss-pass.json?lat=<LAT>&lon=<LONG>
			timer = millis();
			print_p(PSTR("\n>>> REQUEST ISS DATA\n"));
			get_ip_via_dns(website_name_iss, website_ip_iss); //unexplained behaviour: ether.browseUrl makes a request to the domain that was last called by ether.dnsLookup, not the address ip passed to it as an argument
			url_var_part[0]='\0';
			strlcat(url_var_part, "lat=", URL_BUFF_SIZE);
			strlcat(url_var_part, geoiplat_string, URL_BUFF_SIZE);
			strlcat(url_var_part, "&lon=", URL_BUFF_SIZE);
			strlcat(url_var_part, geoiplong_string, URL_BUFF_SIZE);
			PRINT_TOKEN(url_var_part);
			ether.browseUrl(PSTR("/iss-pass.json?n=1&"), url_var_part, website_name_iss, ethercard_callback);
			state_cb = STATE_CB_WAIT;
			state = STATE_PROCESS_DATA;
			break;
		default:
			print_p(PSTR("ERROR: Fell through STATE_GET_DATA case"));
			state_data = STATE_GET_DATA_GEOIP_LAT;
			digitalWrite(error_led, HIGH); 
			delay(10000);
			break;
		}
		break;
	case STATE_PROCESS_DATA:
		if (millis() > timer + REQUEST_RATE){
			digitalWrite(error_led, HIGH);
			print_p(PSTR("WARNING: Timed out waiting for reply.\n"));
			PRINT_TOKEN(state);
			PRINT_TOKEN(state_data);
			state = STATE_GET_DATA;
			break;
		}
		if(state_cb == STATE_CB_READY){
			switch(state_data){
			case STATE_GET_DATA_GEOIP_LAT:
				print_p(PSTR("Got latitude response\n"));
				parse_data((char*)&Ethernet::buffer[callback_data_position], keyword_geoiplat, geoiplat_string, RETURN_LEN_GEOIPLAT);
				PRINT_TOKEN(geoiplat_string);
				state_data = STATE_GET_DATA_GEOIP_LONG;
				state = STATE_GET_DATA;
				break;
			case STATE_GET_DATA_GEOIP_LONG:
				print_p(PSTR("Got longitude response\n"));
				parse_data((char*)&Ethernet::buffer[callback_data_position], keyword_geoiplong, geoiplong_string, RETURN_LEN_GEOIPLONG);
				PRINT_TOKEN(geoiplong_string);
				state_data = STATE_GET_DATA_TIME;
				state = STATE_GET_DATA;
				break;
			case STATE_GET_DATA_TIME:
				print_p(PSTR("Got time response\n"));
				parse_data((char*)&Ethernet::buffer[callback_data_position], keyword_time, time_of_request_string, RETURN_LEN_TIME);
				time_of_request = my_atoi(time_of_request_string);
				PRINT_TOKEN(time_of_request_string);
				PRINT_TOKEN(time_of_request);
				millis_at_time_of_request = millis();
				PRINT_TOKEN(millis_at_time_of_request);
				state_data = STATE_GET_DATA_ISS_DURATION;
				state = STATE_GET_DATA;
				break;
			case STATE_GET_DATA_ISS_DURATION:
				print_p(PSTR("Got ISS duration response\n"));
				parse_data((char*)&Ethernet::buffer[callback_data_position], keyword_issduration, issduration_string, RETURN_LEN_ISSDURATION);
				issduration = my_atoi(issduration_string);
				PRINT_TOKEN(issduration_string);
				PRINT_TOKEN(issduration);
				state_data = STATE_GET_DATA_ISS_RISETIME;
				state = STATE_GET_DATA;
				break;
			case STATE_GET_DATA_ISS_RISETIME:
				print_p(PSTR("Got ISS risetime response\n"));
				parse_data((char*)&Ethernet::buffer[callback_data_position], keyword_issrisetime, issrisetime_string, RETURN_LEN_ISSRISETIME);
				issrisetime = my_atoi(issrisetime_string);
				PRINT_TOKEN(issrisetime_string);
				PRINT_TOKEN(issrisetime);
				state = STATE_CALCULATE;
				break;
			}
		}
		break;
	case STATE_CALCULATE:
		print_p(PSTR("\nData acquisition finished.\n"));
		millis_until_risetime = (((issrisetime - time_of_request)*1000) - (millis() - millis_at_time_of_request)); //millis_until_risetime = (millis until risetime) - (millis since request)
		PRINT_TOKEN(issrisetime);
		PRINT_TOKEN(time_of_request);
		print_p(PSTR("millis()-millis_at_time_of_request="));
		Serial.println(millis()-millis_at_time_of_request);
		PRINT_TOKEN(millis_until_risetime);
		if (millis_until_risetime > 0){
			state = STATE_SLEEP;
		}
		else{
			state = STATE_LIGHT_LAMP;
		}
		break;
	case STATE_LIGHT_LAMP:
		print_p(PSTR("The ISS is above! Go look!"));
		PRINT_TOKEN(issduration);
		digitalWrite(lamp, HIGH);
		delay(issduration*1000); //verify that delay command works for this long
		digitalWrite(lamp, LOW);
		state_data = STATE_GET_DATA_TIME;
		state = STATE_GET_DATA;
		break;
	case STATE_SLEEP:
		print_p(PSTR("Going to sleep"));
		//SLEEPING LOGIC GOES HERE
		delay(millis_until_risetime);
		state = STATE_CALCULATE;
		break;
	default:
		print_p(PSTR("ERROR: Fell through top level state machine"));
		digitalWrite(error_led, HIGH); 
		delay(10000);
		state_data = STATE_GET_DATA_GEOIP_LAT;
		state = STATE_GET_DATA;
		break;
	}
}

