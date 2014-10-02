/**
* @file   Networking.c
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Networking helpers (to use with the EtherCard library)
**/

#include "Networking.h"

/**
* @name    ethercard_callback
* @brief   Called by the ethercard library when the client request is complete.
*
* @note   params must be: byte status, word off, word len
*/
static void ethercard_callback(byte status, word off, word len){
	if(status){
		print_p(PSTR("WARNING: web request returned status code != 200"));
		Serial.println((const char*) Ethernet::buffer + off);
	}
	Serial.print("<<< reply ");
	Serial.print(millis() - timer);
	Serial.println(" ms");
	browserCallbackDatapos=off;
	state_cb = STATE_CB_READY;
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

/**
* @name    get_ip_via_dns
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
uint8_t get_ip_via_dns(prog_char * website_name, uint8_t * website_ip){
	if (!ether.dnsLookup(website_name)){
		print_p(PSTR("WARNING: DNS failed on "));
		print_p(website_name); //website_name is a progmem string
		return 1;
	}
	else {
		website_ip = ether.hisip;
		print_p(website_name); //website_name is a progmem string
		ether.printIp(" ip: ", website_ip);
		return 0;
	}
}
