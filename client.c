/******************************************************************************
*
* FILENAME:
*     client.c
*
* DESCRIPTION:
*     The example of client using datagram socket(UDP).
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
 *      print_usage
 *
 * DESCRIPTION: 
 *      Print usage information and exit the program.
 *
 * PARAMETERS:
 *      pname - The name of the program.
 *
 * RETURN:
 *      None
 ******************************************************************************/
void print_usage(char *pname)
{
    printf("\n"
        "================================================\n"
        "    Client to communicate via datagram socket   \n"
        "                    v%d.%d                      \n"
        "================================================\n"
        "\n"
        "Usage: %s [-s server_ip] [-p port_number]\n"
        "\n"
        "Options:\n"
        "    -s server_ip     The IP address of server, default: %s\n"
        "    -p port_number   The port number of server, default: %d\n"
        "\n"
        "Example:\n"
        "    %s -p 9000\n"
        "\n",
        VERSION_MAJOR, VERSION_MINOR,
        pname, SERVER_IP, SERVER_PORT,
        pname
        );
    exit(STATUS_ERROR);
}


int main(int argc, char *argv[])
{
    dsc_client_t *clnt;
    char *pname = argv[0];
    const char *server_ip = SERVER_IP;
    int serv_port = SERVER_PORT;
    int opt;

    while ((opt = getopt(argc, argv, ":hp:s:")) != -1) {
        switch (opt) {
        case 'p':
            serv_port = strtol(optarg, NULL, 10);
            if (serv_port <= 0) {
                printf("Error: invalid port number!\n");
                print_usage(pname);
            }
            break;

        case 's':
            server_ip = optarg;
            break;

        case 'h':
            print_usage(pname);
            break;

        case ':':
            printf("Error: option '-%c' needs a value\n", optopt);
            print_usage(pname);
            break;

        case '?':
        default:
            printf("Error: invalid option '-%c'\n", optopt);
            print_usage(pname);
            break;
        }
    }
    if (optind < argc) {
        printf("Error: invalid argument '%s'\n", argv[optind]);
        print_usage(pname);
    }

    printf("Connect server %s:%d\n", server_ip, serv_port);
    clnt = client_init(server_ip, serv_port);
    if (clnt == NULL) {
        printf("Error: client init error\n");
        return STATUS_INIT_ERROR;
    }

    /********************** Get version of server ***********************/
    {
        dsc_command_t req;
        dsc_response_version_t *ver;

        req.command = CMD_GET_VERSION;
        req.data_len = 0;

        printf("Send CMD_GET_VERSION request\n");
        ver = (dsc_response_version_t *)client_send_request(clnt, &req);
        if (ver == NULL) {
            printf("Error: client send request error\n");
            client_close(clnt);
            return STATUS_ERROR;
        }

        if (ver->common.status == STATUS_SUCCESS) {
            printf("Version: %d.%d\n", ver->major, ver->minor);
        } else {
            printf("CMD_GET_VERSION error(%d)\n", ver->common.status);
        }

        free(ver);
    }

    /********************** Get message from server ***********************/
    {
        dsc_command_t req;
        dsc_response_get_msg_t *res;

        req.command = CMD_GET_MESSAGE;
        req.data_len = 0;

        printf("Send CMD_GET_MESSAGE request\n");
        res = (dsc_response_get_msg_t *)client_send_request(clnt, &req);
        if (res == NULL) {
            printf("Error: client send request error\n");
            client_close(clnt);
            return STATUS_ERROR;
        }

        if (res->common.status == STATUS_SUCCESS) {
            printf("Message: %s\n", res->data);
        } else {
            printf("CMD_GET_MESSAGE error(%d)\n", res->common.status);
        }

        free(res);
    }

    /********************** Put message to server ***********************/
    {
        dsc_request_put_msg_t req;
        dsc_command_t *res;
        char str[] = "Hello, this is a message from client.";

        req.common.command = CMD_PUT_MESSAGE;
        req.common.data_len = strlen(str)+1;
        snprintf(req.data, DSC_PUT_MSG_SIZE-1, "%s", str);
        req.data[DSC_PUT_MSG_SIZE-1] = 0;

        printf("Send CMD_PUT_MESSAGE request\n");
        res = client_send_request(clnt, (dsc_command_t *)&req);
        if (res == NULL) {
            printf("Error: client send request error\n");
            client_close(clnt);
            return STATUS_ERROR;
        }

        if (res->status == STATUS_SUCCESS) {
            printf("CMD_PUT_MESSAGE OK\n");
        } else {
            printf("CMD_PUT_MESSAGE error(%d)\n", res->status);
        }

        free(res);
    }

    /********************** Send an unknown request to server ***********************/
    {
        dsc_command_t req;
        dsc_command_t *res;

        req.command = 0xFFFF;
        req.data_len = 0;

        printf("Send an unknown request\n");
        res = (dsc_command_t *)client_send_request(clnt, &req);
        if (res == NULL) {
            printf("Error: client send request error\n");
            client_close(clnt);
            return STATUS_ERROR;
        }
        printf("Response status(%d)\n", res->status);

        free(res);
    }

    client_close(clnt);
    return STATUS_SUCCESS;
}

