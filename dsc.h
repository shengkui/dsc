/******************************************************************************
*
* FILENAME:
*     dsc.h
*
* DESCRIPTION:
*     Define some structure for datagram socket communication(UDP).
*
* REVISION(MM/DD/YYYY):
*     07/30/2014  Shengkui Leng (lengshengkui@outlook.com)
*     - Initial version 
*
******************************************************************************/
#ifndef _DSC_H_
#define _DSC_H_
#include <stdint.h>
#include <netinet/in.h>


/*--------------------------------------------------------------
 * Definition for both client and server
 *--------------------------------------------------------------*/

/* The read/write buffer size of socket */
#define DSC_BUF_SIZE            4096

/* The signature of the request/response packet */
#define DSC_SIGNATURE           0xDEADBEEF

/* Make a structure 1-byte aligned */
#define BYTE_ALIGNED            __attribute__((packed))


/* Status code, the values used in struct dsc_command_t.status */
#define STATUS_SUCCESS          0   /* Success */
#define STATUS_ERROR            1   /* Generic error */


/* Common header of both request/response packets */
typedef struct dsc_command {
    uint32_t signature;         /* Signature, shall be DSC_SIGNATURE */
    union {
        uint32_t command;       /* Request type */
        uint32_t status;        /* Status code of response, refer dsc_status_code */
    };
    uint32_t data_len;          /* The data length of packet */

    uint16_t checksum;          /* The checksum of the packet */
} BYTE_ALIGNED dsc_command_t;


/*--------------------------------------------------------------
 * Definition for client only
 *--------------------------------------------------------------*/

/* Keep the information of client */
typedef struct dsc_client {
    int sockfd;                     /* Socket fd of the client */
    struct sockaddr_in serv_addr;   /* Server address */
} dsc_client_t;


dsc_client_t *client_init(const char *server_ip, int server_port);
dsc_command_t *client_send_request(dsc_client_t *c, dsc_command_t *req);
void client_close(dsc_client_t *c);


/*--------------------------------------------------------------
 * Definition for server only
 *--------------------------------------------------------------*/

typedef dsc_command_t * (*request_handler_t) (dsc_command_t *);

/* Keep the information of server */
typedef struct dsc_server {
    int sockfd;                         /* Socket fd of the server */
    struct sockaddr_in addr;            /* Server address */
    request_handler_t request_handler;  /* Function pointer of the request handle */
} dsc_server_t;


dsc_server_t *server_init(request_handler_t req_handler, int port, int timeout);
int server_accept_request(dsc_server_t *s);
void server_close(dsc_server_t *s);


#endif /* _DSC_H_ */
