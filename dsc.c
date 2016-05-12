/******************************************************************************
 *
 * FILENAME:
 *     dsc.c
 *
 * DESCRIPTION:
 *     Define APIs for datagram socket communication(UDP).
 *
 * REVISION(MM/DD/YYYY):
 *     07/30/2014  Shengkui Leng (lengshengkui@outlook.com)
 *     - Initial version 
 *
 ******************************************************************************/
#include <unistd.h>
#include "dsc.h"


/******************************************************************************
 * NAME:
 *      compute_checksum
 *
 * DESCRIPTION: 
 *      Compute 16-bit One's Complement sum of data. (The algorithm comes from
 *      RFC-1071)
 *      NOTES: Before call this function, please set the checksum field to 0.
 *
 * PARAMETERS:
 *      buf  - The data buffer
 *      len  - The length of data(bytes).
 *
 * RETURN:
 *      Checksum
 ******************************************************************************/
static uint16_t compute_checksum(void *buf, ssize_t len)
{
    uint16_t *word;
    uint8_t *byte;
    ssize_t i;
    unsigned long sum = 0;

    if (!buf) {
        return 0;
    }

    word = (uint16_t *)buf;
    for (i = 0; i < len/2; i++) {
        sum += word[i];
    }

    /* If the length(bytes) of data buffer is an odd number, add the last byte. */
    if (len & 1) {
        byte = (uint8_t *)buf;
        sum += byte[len-1];
    }

    /* Take only 16 bits out of the sum and add up the carries */
    while (sum>>16) {
        sum = (sum>>16) + (sum&0xFFFF);
    }

    return (uint16_t)(~sum);
}


/******************************************************************************
 * NAME:
 *      verify_command_packet
 *
 * DESCRIPTION: 
 *      Verify the data integrity of the command packet.
 *
 * PARAMETERS:
 *      buf  - The data of command packet
 *      len  - The length of data
 *
 * RETURN:
 *      1 - OK, 0 - FAIL
 ******************************************************************************/
static int verify_command_packet(void *buf, size_t len)
{
    dsc_command_t *pkt;

    if (buf == NULL) {
        return 0;
    }
    pkt = (dsc_command_t *)buf;

    if (pkt->signature != DSC_SIGNATURE) {
        printf("Error: invalid signature of packet (0x%08X)\n", pkt->signature);
        return 0;
    }

    if (pkt->data_len + sizeof(dsc_command_t) != len) {
        printf("Error: invalid length of packet (%ld:%ld)\n",
            pkt->data_len + sizeof(dsc_command_t), len);
        return 0;
    }

    if (compute_checksum(buf, len) != 0) {
        printf("Error: invalid checksum of packet\n");
        return 0;
    }

    return 1;
}


/******************************************************************************
 * NAME:
 *      server_init
 *
 * DESCRIPTION: 
 *      Do some initialzation work for server.
 *
 * PARAMETERS:
 *      req_handler - The function pointer of a user-defined request handler.
 *      port        - The port number of server
 *      timeout     - Timeout value(seconds) of recvfrom operation while waiting
 *                    for request from client.
 *
 * RETURN:
 *      A pointer of server info.
 ******************************************************************************/
dsc_server_t *server_init(request_handler_t req_handler, int port, int timeout)
{
    dsc_server_t *s;
    int rc;

    if (req_handler == NULL) {
        printf("Error: invalid parameter!\n");
        return NULL;
    }

    s = (dsc_server_t *)malloc(sizeof(dsc_server_t));
    if (s == NULL) {
        perror("malloc error");
        return NULL;
    }
    memset(s, 0, sizeof(dsc_server_t));

    /* Setup request handler */
    s->request_handler = req_handler;

    memset(&s->addr, 0, sizeof(s->addr));
    s->addr.sin_family = AF_INET;
    s->addr.sin_port = htons(port);
    s->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->sockfd < 0) {
        perror("socket error");
        free(s);
        return NULL;
    }

    /* Set the timeout value of recvfrom operation. */
    if (timeout >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if (setsockopt(s->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("Set recv timeout error");
            close(s->sockfd);
            free(s);
            return NULL;
        }
    }

    /* Avoid "Address already in use" error in bind() */
    int val = 1;
    if (setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR, &val,
        sizeof(val)) == -1) {
        perror("setsockopt error");
        close(s->sockfd);
        free(s);
        return NULL;
    }

    rc = bind(s->sockfd, (struct sockaddr *)&s->addr, sizeof(s->addr));
    if (rc != 0) {
        perror("bind error");
        close(s->sockfd);
        free(s);
        return NULL;
    }

    return s;
}


/******************************************************************************
 * NAME:
 *      server_accept_request
 *
 * DESCRIPTION: 
 *      Accept a request from client and process it.
 *
 * PARAMETERS:
 *      s - A pointer of server info
 *
 * RETURN:
 *      0 - OK, Others - Error
 ******************************************************************************/
int server_accept_request(dsc_server_t *s)
{
    dsc_command_t *req;
    dsc_command_t *resp;
    uint8_t buf[DSC_BUF_SIZE];
    ssize_t bytes, req_len, resp_len;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr);

    if (s == NULL) {
        printf("Error: invalid parameter!\n");
        return -1;
    }

    /* Receive request from client */
    memset(buf, 0, sizeof(buf));
    req_len = recvfrom(s->sockfd, &buf, sizeof(buf), 0,
        (struct sockaddr *)&client_addr, &client_addrlen);
    if (req_len < 0) {
        //perror("recvform error");
        return -1;
    } else if (req_len == 0) {
        return -1;
    }

    /* Check the integrity of the request packet */
    if (!verify_command_packet(buf, req_len)) {
        /* Discard invaid packet */
        return -1;
    }

    /* Process the request */
    req = (dsc_command_t *)buf;
    resp = s->request_handler(req);
    if (resp == NULL) {
        resp = (dsc_command_t *)buf;   /* Use a local buffer */
        resp->status = STATUS_ERROR;
        resp->data_len = 0;
    }

    resp_len = sizeof(dsc_command_t) + resp->data_len;
    resp->signature = req->signature;
    resp->checksum = 0;
    resp->checksum = compute_checksum(resp, resp_len);

    int rc = 0;
    /* Send response */
    bytes = sendto(s->sockfd, resp, resp_len, 0, (struct sockaddr *)&client_addr,
        sizeof(struct sockaddr));
    if (bytes != resp_len) {
        perror("sendto error");
        rc = -1;
    }
    if (resp != (dsc_command_t *)buf) {    /* If NOT local buffer, free it */
        free(resp);
    }

    return rc;
}


/******************************************************************************
 * NAME:
 *      server_close
 *
 * DESCRIPTION: 
 *      Close the socket fd and free memory.
 *
 * PARAMETERS:
 *      s - A pointer of server info
 *
 * RETURN:
 *      None
 ******************************************************************************/
void server_close(dsc_server_t *s)
{
    if (s == NULL) {
        return;
    }

    close(s->sockfd);
    free(s);
}


/******************************************************************************
 * NAME:
 *      client_init
 *
 * DESCRIPTION: 
 *      Do some initialzation work for client.
 *
 * PARAMETERS:
 *      server_ip   - The IP address of server
 *      server_port - The port number of server
 *
 * RETURN:
 *      A pointer of client info.
 ******************************************************************************/
dsc_client_t *client_init(const char *server_ip, int server_port)
{
    dsc_client_t *c;
    int fd;

    c = (dsc_client_t *)malloc(sizeof(dsc_client_t));
    if (c == NULL) {
        perror("malloc error");
        return NULL;
    }
    memset(c, 0, sizeof(dsc_client_t));

    memset(&c->serv_addr, 0, sizeof(c->serv_addr));
    c->serv_addr.sin_family = AF_INET;
    c->serv_addr.sin_port = htons(server_port);
    c->serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket error");
        free(c);
        return NULL;
    }
    c->sockfd = fd;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Set recv timeout Error");
        free(c);
        close(fd);
        return NULL;
    }

    return c;
}


/******************************************************************************
 * NAME:
 *      client_send_request
 *
 * DESCRIPTION: 
 *      Send a request to server, and get the response.
 *
 * PARAMETERS:
 *      c   - A pointer of client info
 *      req - The request to send
 *
 * RETURN:
 *      The response for the request. The caller need to free the memory.
 ******************************************************************************/
dsc_command_t *client_send_request(dsc_client_t *c, dsc_command_t *req)
{
    uint8_t buf[DSC_BUF_SIZE];
    ssize_t bytes, req_len;
    struct sockaddr_in server_addr;
    socklen_t server_addrlen = sizeof(struct sockaddr);
    
    if ((c == NULL) || (req == NULL)) {
        printf("Error: invalid parameter!\n");
        return NULL;
    }

    /* Send request */
    req_len = sizeof(dsc_command_t) + req->data_len;
    req->signature = DSC_SIGNATURE;
    req->checksum = 0;
    req->checksum = compute_checksum(req, req_len);
    bytes = sendto(c->sockfd, req, req_len, 0, (struct sockaddr *)&c->serv_addr,
        sizeof(struct sockaddr));
    if (bytes != req_len) {
        perror("sendto error");
        return NULL;
    }

    /* Get response */
    memset(buf, 0, sizeof(buf));
    bytes = recvfrom(c->sockfd, &buf, sizeof(buf), 0,
        (struct sockaddr *)&server_addr, &server_addrlen);
    if (bytes < 0) {
        perror("recvform error");
        return NULL;
    } else if (bytes == 0) {
        return NULL;
    }

    /* Check the integrity of the response packet */
    if (verify_command_packet(buf, bytes)) {
        dsc_command_t *resp = (dsc_command_t *)malloc(bytes);
        if (resp) {
            memcpy(resp, buf, bytes);
        } else {
            perror("malloc error");
        }
        return resp;
    }

    return NULL;
}


/******************************************************************************
 * NAME:
 *      client_close
 *
 * DESCRIPTION: 
 *      Close the client socket fd and free memory.
 *
 * PARAMETERS:
 *      c - The pointer of client info
 *
 * RETURN:
 *      None
 ******************************************************************************/
void client_close(dsc_client_t *c)
{
    if (c == NULL) {
        return;
    }

    close(c->sockfd);
    free(c);
}

