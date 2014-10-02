/**
* @file   Networking.h
* @author Marc Khouri (marc@khouri.ca)
* @date   April 2014
* @brief  Networking helpers (to use with the EtherCard library)
**/

#ifndef __NETWORKING_H_
#define __NETWORKING_H_

/**
* @name    ethercard_callback
* @brief   Called by the ethercard library when the client request is complete.
*
* @note   params must be: byte status, word off, word len
*
* @param [out]  off  The offset 
*/
static void ethercard_callback(byte status, word off, word len);

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
uint8_t init_DHCP(void);

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
uint8_t get_ip_via_dns(prog_char * website_name, uint8_t * website_ip);

#endif