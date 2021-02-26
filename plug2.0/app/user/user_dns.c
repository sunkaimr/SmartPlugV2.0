/*
 * user_temp.c
 *
 *  Created on: 2018骞�11鏈�17鏃�
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"

static int DNS_CreateUDPSocket()
{
    struct sockaddr_in saddr = { 0 };
    int sock = -1;
    int err = 0;

    LOG_OUT(LOGOUT_DEBUG, "socket...");
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
    	LOG_OUT(LOGOUT_ERROR, "Failed to create socket. Error %d", errno);
        return -1;
    }

    LOG_OUT(LOGOUT_DEBUG, "socket ok");

    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(53);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    LOG_OUT(LOGOUT_DEBUG, "bind...");
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0)
    {
    	LOG_OUT(LOGOUT_ERROR, "Failed to bind socket. Error %d", errno);
        goto err;
    }
    LOG_OUT(LOGOUT_DEBUG, "bind ok");
    return sock;

err:
    close(sock);
    return -1;
}

static void DNS_DNSServerTask(void *pvParameters)
{
    struct sockaddr_in client = { 0 };
    socklen_t  client_len=sizeof(struct sockaddr_in);
    uint8 data[128];
    uint8 domain[64];
    int len = 0;
    uint8 i = 0, j = 0;

    LOG_OUT(LOGOUT_INFO, "DNS server start...");

    int sock = DNS_CreateUDPSocket();
    if ( sock < 0 )
    {
    	LOG_OUT(LOGOUT_ERROR, "Failed to create IPv4 multicast socket");
    	return;
    }

    LOG_OUT(LOGOUT_INFO, "DNS server started");

    for(;;)
    {
        len = recvfrom( sock, data, 100, 0, (struct sockaddr *)&client, &client_len);
        if((len < 0) || ( len > 100))
        {
        	LOG_OUT(LOGOUT_ERROR, "recvfrom error");
            continue;
        }

        for ( i = 13, j = 0; i < len-5 && j < sizeof(domain)-1; i++, j++ )
        {
        	if ( isalnum(data[i]) )
        	{
        		domain[j] = data[i];
        	}
        	else
        	{
        		domain[j] = '.';
        	}
        }
        domain[j] = 0;

        data[2] |= 0x80;
        data[7] = 1;

        data[len++] =0xc0;
        data[len++] =0x0c;

        data[len++] =0x00;
        data[len++] =0x01;
        data[len++] =0x00;
        data[len++] =0x01;

        data[len++] =0x00;
        data[len++] =0x00;
        data[len++] =0x00;
        data[len++] =0x0A;

        data[len++] =0x00;
        data[len++] =0x04;

        if( strstr( domain, "connect") ||
        	strstr( domain, "smartplug"))
        {
            data[len++] = 192;
            data[len++] = 168;
            data[len++] = 4;
            data[len++] = 1;
            LOG_OUT(LOGOUT_DEBUG, "DNS request: %s -> 192.168.4.1", domain);
        }
        else
        {
            data[len++] = 127;
            data[len++] = 0;
            data[len++] = 0;
            data[len++] = 1;
            LOG_OUT(LOGOUT_DEBUG, "DNS request: %s -> 127.0.0.1", domain);
        }

        sendto(sock, data, len, 0, (struct sockaddr*)&client, client_len);
    }

    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}

void DNS_StartDNSServerTheard()
{
    xTaskCreate(&DNS_DNSServerTask, "DNS_DNSServerTask", 512, NULL, 7, NULL);
}
