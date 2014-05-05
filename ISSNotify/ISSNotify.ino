/**
 * @file   ISSNotify.ino
 * @author Marc Khouri (marc@khouri.ca)
 * @date   April 2014
 * @brief  Arduino code to activate a light when the International 
 * Space Station is above the current loctation.
 *
 * This is an Arduino sketch that communicates with an ENC28J60 
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
 * @todo JSON request to timezonedb.com is malformed
 * @bug Unexplained behaviour: function ether.browseUrl seems to make its request from the ip identified by ether.dnsLookup(website_name), which is called in my function fill_ip_with_dns, instead of from the address passed to it.
 */

#include <EtherCard.h>
#include "apikey.h" //put your api key from open-notify.org in here as: extern char apikey[]="YOUR_KEY_HERE" 

#define REQUEST_RATE 10000 // milliseconds

int error_led = 13;
static byte mymac[] = {0x74,0x69,0x69,0x2D,0x30,0x31};
prog_char website_name_geoip[] PROGMEM = "freegeoip.net"; //website names must be in PROGMEM for ethercard lib
prog_char website_name_time[] PROGMEM = "api.timezonedb.com";
prog_char website_name_iss[] PROGMEM = "api.open-notify.org";
uint8_t website_ip_geoip[4] = {0,0,0,0};
uint8_t website_ip_time[4] = {0,0,0,0};
uint8_t website_ip_iss[4] = {0,0,0,0};

uint16_t browserCallbackDatapos=0;
byte Ethernet::buffer[500];
char url_buffer[50];
char url_var_part[50];
static uint32_t timer;
void print_p(const prog_char str[]); //print progmem strings

// system states 
typedef enum {
  STATE_GET_DATA = 0,
  STATE_PROCESS_DATA = 1,
  STATE_LIGHT_LAMP = 2
} my_state_t;
typedef enum {
  STATE_GET_DATA_GEOIP_LAT = 0,
  STATE_GET_DATA_GEOIP_LONG = 1,
  STATE_GET_DATA_TIME = 2,
  STATE_GET_DATA_ISS_RISETIME = 3,
  STATE_GET_DATA_ISS_DURATION = 4
} my_data_state_t;
typedef enum {
  STATE_CB_WAIT = 0,
  STATE_CB_READY = 1,
} my_cb_state_t;
my_state_t state = STATE_GET_DATA;
my_data_state_t state_data = STATE_GET_DATA_GEOIP_LAT;
my_cb_state_t state_cb = STATE_CB_WAIT;

static const char keyword_geoiplat[]="latitude\":"; //keyword to use when searching for latitude
#define RETURN_LEN_GEOIPLAT 8 //return can be as long as "-90.0000"
static char current_geoiplat_string[RETURN_LEN_GEOIPLAT]="";

static const char keyword_geoiplong[]="longitude\":"; //keyword to use when searching for longitude
#define RETURN_LEN_GEOIPLONG 9 //return can be as long as "-180.0000"
static char current_geoiplong_string[RETURN_LEN_GEOIPLONG]="";

static const char keyword_time[]="timestamp\":";
#define RETURN_LEN_TIME 10
static char current_time_string[RETURN_LEN_TIME];
//time is contained in int16_t unixtimestamp

static const char keyword_issduration[]="duration\":";
#define RETURN_LEN_ISSDURATION 10
static char current_issduration_string[RETURN_LEN_ISSDURATION];
static int16_t issDuration=0;

static const char keyword_issrisetime[]="risetime\":";
#define RETURN_LEN_ISSRISETIME 10
static char current_issrisetime_string[RETURN_LEN_ISSRISETIME];
static int32_t issRisetime=0;

/**
 * @name    my_result_cb
 * @brief   Called by the ethercard library when the client request is complete.
 *
 * @note   params must be: byte status, word off, word len
 */
static void my_result_cb (byte status, word off, word len) {
  if(status){
    Serial.println("WARNING: web request returned status code != 200");
    Serial.println((const char*) Ethernet::buffer + off);
  }
  Serial.print("<<< reply ");
  Serial.print(millis() - timer);
  Serial.println(" ms");
  browserCallbackDatapos=off;
  Serial.println(state_cb); //XXX
  state_cb = STATE_CB_READY;
  Serial.println(state_cb); //XXX
}

/**
 * @name    init_DHCP
 * @brief   Checks ethernet controller communication and initializes DHCP
 *
 * @note   Outputs the received IP address to the serial terminal.
 * 
 * @retval 0  Success.
 * @retval 1  Failed to access Ethernet controller
 * @retval 2  Failed to acquire DHCP
 */
uint8_t init_DHCP(void){
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0){
    Serial.println( "Failed to access Ethernet controller");
    return 1;
  } 
  else if (!ether.dhcpSetup()){
    Serial.println("DHCP failed");
    return 2;
  }
  else {  
    ether.printIp("My IP: ", ether.myip);
    ether.printIp("GW IP: ", ether.gwip);
    ether.printIp("DNS IP: ", ether.dnsip); 
    return 0;
  }
}

/**
 * @name    fill_ip_with_DNS
 * @brief   Fills the IP address of a given domain into the input website_ip.
 *
 * @note   Outputs the resolved IP address to the serial terminal.
 * 
 * @param [in] website_name  Domain name to find IP of.
 * @param [out] website_ip   Website IP.
 *
 * @retval 1  DNS failed to resolve this domain. 
 * @retval 0  Success. Filled the IP address of the given domain into input point website_ip.
 */
uint8_t fill_ip_with_dns(prog_char * website_name, uint8_t * website_ip){
  if (!ether.dnsLookup(website_name)){
    print_p(PSTR("DNS failed on "));
    print_p(website_name);
    return 1;
  }
  else {
    website_ip = ether.hisip;
    print_p(website_name);
    ether.printIp(" ip: ", website_ip);
    return 0;
  }
}

/**
 *  @name parseData
 *  @brief a function that searches the first 1000 chars of data "input" for a specified string "keyword", then returns up to the next delimiter or "outputlength" characters into "output"
 *  
 */
void parseData(char * inputData, const char * keyword, char * output, uint8_t outputlength)
{
	uint16_t i=1000; // search the first 1000 char
	uint8_t j=0;
	while(i && *inputData) {
		if (j && keyword[j]=='\0') { //found a match
      i=0;
			while(*inputData && (i < outputlength) && 
        (*inputData != '\r') && 
        (*inputData != '\n') && 
        (*inputData != '}') && 
        (*inputData != ',') && 
        (*inputData != ';') && 
        (*inputData != ' ') && 
        (*inputData != '"')){
        output[i]=*inputData;
				inputData++;
				i++;
			}
			output[i]='\0';
			return;
		}
		if (*inputData==keyword[j]) {
			j++;
		} else {
			j=0;
		}
		inputData++;
		i--;
	}
}

void setup () {
  pinMode(error_led, OUTPUT);
  Serial.begin(57600);
  Serial.println("\n[ISS Notification Lamp]");

  while(init_DHCP()){  //continue initializing DHCP until success (retval=0)
    delay(1000);
    digitalWrite(error_led, HIGH); 
  }
  digitalWrite(error_led, LOW);

  uint8_t ip_initialized=1;
  while(ip_initialized){ //loop until all 3 fill_ip_with_dns functions return success (retval=0)
    Serial.println("Requesting site IPs via DNS");
    ip_initialized = 0;
    ip_initialized += fill_ip_with_dns(website_name_geoip, website_ip_geoip);
    ip_initialized += fill_ip_with_dns(website_name_time, website_ip_time);
    ip_initialized += fill_ip_with_dns(website_name_iss, website_ip_iss);
    digitalWrite(error_led, HIGH);
  }
  digitalWrite(error_led, LOW);

  state = STATE_GET_DATA; //ensure state machine starts in the proper state
  state_data = STATE_GET_DATA_GEOIP_LAT;
  timer = - REQUEST_RATE; // start timing out right away
}

void loop () {

  ether.packetLoop(ether.packetReceive());

  switch(state){
    case STATE_GET_DATA:
      switch(state_data){
        case STATE_GET_DATA_GEOIP_LAT:
          //request format http://freegeoip.net/json/
          timer = millis();
          Serial.println("\n>>> REQ GEOIP LAT");
          fill_ip_with_dns(website_name_geoip, website_ip_geoip); //unexplained behaviour: ether.browseUrl makes a request to the domain that was last called by ether.dnsLookup, not the address passed to it as an argument
          ether.browseUrl(PSTR("/json/"), "", website_name_geoip, my_result_cb);
          state_cb = STATE_CB_WAIT;
          state = STATE_PROCESS_DATA;
          break;
        case STATE_GET_DATA_GEOIP_LONG:
          //request format http://freegeoip.net/json/
          timer = millis();
          Serial.println("\n>>> REQ GEOIP LONG");
          fill_ip_with_dns(website_name_geoip, website_ip_geoip); //unexplained behaviour: ether.browseUrl makes a request to the domain that was last called by ether.dnsLookup, not the address passed to it as an argument
          ether.browseUrl(PSTR("/json/"), "", website_name_geoip, my_result_cb);
          state_cb = STATE_CB_WAIT;
          state = STATE_PROCESS_DATA;
          break;
        case STATE_GET_DATA_TIME:
          //request format http://api.timezonedb.com/?format=json&lat=<LAT>&lng=<LONG>&key=<Your_API_Key>
          timer = millis();
          Serial.println("\n>>> REQ TIME");
          fill_ip_with_dns(website_name_time, website_ip_time); //unexplained behaviour: ether.browseUrl makes a request to the domain that was last called by ether.dnsLookup, not the address passed to it as an argument
          url_buffer[0]='\0';
          strcat(url_buffer,"lat=");
          strcat(url_buffer,current_geoiplat_string);
          strcat(url_buffer,"&lng=");
          strcat(url_buffer,current_geoiplong_string);
          strcat(url_buffer,"&key=");
          strcat(url_buffer,apikey);
          Serial.println(url_buffer);
          ether.urlEncode(url_buffer,url_var_part);
          Serial.println(url_var_part);
          ether.browseUrl(PSTR("/?format=json&lat=40.7934&lng=-77.86&key=DJX1F48J602Q"), "", website_name_time, my_result_cb);
          state_cb = STATE_CB_WAIT;
          state = STATE_PROCESS_DATA;
          break;
        case STATE_GET_DATA_ISS_DURATION:
          break;
        case STATE_GET_DATA_ISS_RISETIME:
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
        state = STATE_GET_DATA;
        print_p(PSTR("WARNING: Timed out waiting for reply"));
        break;
      }
      if(state_cb == STATE_CB_READY){
        switch(state_data){
          case STATE_GET_DATA_GEOIP_LAT:
            print_p(PSTR("Got latitude response\n"));
            parseData((char*)&Ethernet::buffer[browserCallbackDatapos], keyword_geoiplat, current_geoiplat_string, RETURN_LEN_GEOIPLAT);
            print_p(PSTR("set current_geoiplat_string="));
            Serial.println(current_geoiplat_string);
            state_data = STATE_GET_DATA_GEOIP_LONG;
            state = STATE_GET_DATA;
            break;
          case STATE_GET_DATA_GEOIP_LONG:
            print_p(PSTR("Got longitude response\n"));
            parseData((char*)&Ethernet::buffer[browserCallbackDatapos], keyword_geoiplong, current_geoiplong_string, RETURN_LEN_GEOIPLONG);
            print_p(PSTR("set current_geoiplong_string="));
            Serial.println(current_geoiplong_string);
            state_data = STATE_GET_DATA_TIME;
            state = STATE_GET_DATA;
            break;
          case STATE_GET_DATA_TIME:
            print_p(PSTR("Got time response\n"));
            parseData((char*)&Ethernet::buffer[browserCallbackDatapos], keyword_time, current_time_string, RETURN_LEN_TIME);
            print_p(PSTR("set current_time_string="));
            Serial.println(current_time_string);
            state_data = STATE_GET_DATA_ISS_DURATION;
            state = STATE_GET_DATA;
            break;
        }
      }
      break;
    case STATE_LIGHT_LAMP:
      //LOGIC FOR LAMP LIGHTING
      //TIMEOUT TO REFRESH DATA (set data state to GEOIP)
      //LOW POWER STATE
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

/**
 * @name    print_p
 * @brief   Prints a string stored in PROGMEM.
 * 
 * @param [in] str[] String stored in PROGMEM.
 */
void print_p(const prog_char str[]){
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)))
    Serial.write(c);
}
