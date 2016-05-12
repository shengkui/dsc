/******************************************************************************
*
* FILENAME:
*     server.c
*
* DESCRIPTION:
*     The example of server using datagram socket(UDP).
*
* REVISION(MM/DD/YYYY):
*     07/30/2014  Shengkui Leng (lengshengkui@outlook.com)
*     - Initial version 
*
******************************************************************************/
#include <unistd.h>
#include <signal.h>
#include "common.h"


volatile sig_atomic_t loop_flag = 1;


/*
 * Return the version of server.
 */
dsc_command_t *cmd_get_version(void)
{
    dsc_response_version_t *ver;

    printf("CMD_GET_VERSION\n");

    ver = (dsc_response_version_t *)malloc(sizeof(dsc_response_version_t));
    if (ver != NULL) {
        ver->common.status = STATUS_SUCCESS;
        ver->common.data_len = sizeof(ver->major) + sizeof(ver->minor);
        ver->major = VERSION_MAJOR;
        ver->minor = VERSION_MINOR;
    }

    return (dsc_command_t *)ver;
}


/*
 * Get a message string from server
 */
dsc_command_t *cmd_get_msg(void)
{
    dsc_response_get_msg_t *res;
    const char *str = "Hello, this is a message from the server.";

    printf("CMD_GET_MESSAGE\n");

    res = (dsc_response_get_msg_t *)malloc(sizeof(dsc_response_get_msg_t));
    if (res != NULL) {
        res->common.status = STATUS_SUCCESS;
        res->common.data_len = strlen(str);
        snprintf(res->data, DSC_GET_MSG_SIZE, "%s", str);
        res->data[DSC_GET_MSG_SIZE-1] = 0;
    }

    return (dsc_command_t *)res;
}


/*
 * Send a message string to server
 */
dsc_command_t *cmd_put_msg(dsc_command_t *req)
{
    dsc_command_t *res;
    dsc_request_put_msg_t *put_msg = (dsc_request_put_msg_t *)req;

    printf("CMD_PUT_MESSAGE\n");

    printf("Message: %s\n", (char *)put_msg->data);

    res = (dsc_command_t *)malloc(sizeof(dsc_command_t));
    if (res != NULL) {
        res->status = STATUS_SUCCESS;
        res->data_len = 0;
    }

    return (dsc_command_t *)res;
}


/*
 * Unknown request type
 */
dsc_command_t *cmd_unknown(dsc_command_t *req)
{
    dsc_command_t *res;

    printf("Unknown request type\n");

    res = (dsc_command_t *)malloc(sizeof(dsc_command_t));
    if (res != NULL) {
        res->status = STATUS_INVALID_COMMAND;
        res->data_len = 0;
    }

    return res;
}


/*
 * The handler to handle all requests from client
 */
dsc_command_t *my_request_handler(dsc_command_t *req)
{
    dsc_command_t *resp = NULL;

    switch (req->command) {
    case CMD_GET_VERSION:
        resp = cmd_get_version();
        break;

    case CMD_GET_MESSAGE:
        resp = cmd_get_msg();
        break;
        
    case CMD_PUT_MESSAGE:
        resp = cmd_put_msg(req);
        break;
        
    default:
        resp = cmd_unknown(req);
        break;
    }

    return resp;
}


/*
 * When user press CTRL+C, quit the server process.
 */
void handler_sigint(int sig)
{
    loop_flag = 0;
}

void install_sig_handler()
{
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_handler = handler_sigint;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
}


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
        "    Server to communicate via datagram socket   \n"
        "                    v%d.%d                      \n"
        "================================================\n"
        "\n"
        "Usage: %s [-p port_number]\n"
        "\n"
        "Options:\n"
        "    -p port_number   The port number of server, default: %d\n"
        "\n"
        "Example:\n"
        "    %s -p 9000\n"
        "\n",
        VERSION_MAJOR, VERSION_MINOR,
        pname, SERVER_PORT, pname
        );
    exit(STATUS_ERROR);
}


int main(int argc, char *argv[])
{
    dsc_server_t *s;
    char *pname = argv[0];
    int serv_port = SERVER_PORT;
    int opt;

    while ((opt = getopt(argc, argv, ":hp:")) != -1) {
        switch (opt) {
        case 'p':
            serv_port = strtol(optarg, NULL, 10);
            if (serv_port <= 0) {
                printf("Error: invalid port number!\n");
                print_usage(pname);
            }
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

    printf("Server listening on port %d\n", serv_port);
    s = server_init(&my_request_handler, serv_port, 2);
    if (s == NULL) {
        printf("Error: server init error\n");
        return STATUS_INIT_ERROR;
    }

    install_sig_handler();

    while (loop_flag) {
        server_accept_request(s);
    }

    server_close(s);
    return STATUS_SUCCESS;
}
