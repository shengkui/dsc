 Communication with datagram socket
====================================
**Author**: Shengkui Leng

**E-mail**: lengshengkui@outlook.com


Description
-----------
This project is a demo for how to use datagram socket(SOCK_DGRAM) to communicate
between client and server. It defines some APIs and structures for communication.
And it's scalable.

* * *

Build
-----------
(1) Open a terminal.

(2) chdir to the source code directory.

(3) Run "make"


Run
-----------
(1) Start the server:

>    $ ./server

You can specify the port number with argument:

>    $ ./server -p 6666

Notes:
>    The default port number used by server is 6666.

>    Use '-h' option to get more detail usage information of the server.

(2) Start the client to send request to server:

>    $ ./client

You can specify "server ip" and "server port number" with arguments:

>    $ ./client -s 127.0.0.1 -p 6666

Notes:
>    The default server port number is 6666.

>    The default server ip is 127.0.0.1.

>    Use '-h' option to get more detail usage information of the client.

