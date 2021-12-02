- [Preamble](#orga3e5f4c)
  - [Requirements](#org0d9f716)
  - [Features <code>[2/4]</code>:](#org907f78a)
- [Compilation](#orga7b35d4)



<a id="orga3e5f4c"></a>

# Preamble

This is a small C++ broadcast chat.


<a id="org0d9f716"></a>

## Requirements

1.  Each Client MUST be connected to the Server via TCP/IP.
2.  The Server MAY host more than 3 clients.
3.  Each Client connected to the Server MUST be able to send to the Server a character string passed by argument via command line arguments. (Ex: ./send.out "Hi Everyone!").
4.  The Server MUST forward the received messages to all connected Clients.
5.  The Server SHOULD identify the client that has sent the message. (Ex. "Bento said: Hi Everyone!")
6.  Every 5 seconds the Server MUST check if each client is still ONLINE or AFK. (This message exchange SHOULD NOT show up on the chat terminal, is an internal feature).
7.  The Server MAY have a special command to check the client status.
8.  The Client MAY switch his status from ONLINE to AFK if has not sent any new message for more than a minute.
9.  The Client Message Receiver Service MAY run in the background.
10. The Server App, the Client Message Receiver Service, the Client Message Sender Service SHOULD take leverage of POSIX Threads, IPC, and DAEMON for their development.

**Terminology and Definitions**

1.  SHALL indicates an absolute requirement, as does MUST.
2.  SHALL NOT indicates an absolute prohibition, as does MUST NOT.
3.  SHOULD and SHOULD NOT indicate recommendations.
4.  MAY indicates an option


<a id="org907f78a"></a>

## Features <code>[2/4]</code>:

-   [X] Message broadcast between server and clients
-   [X] Check online status
-   [ ] Device drivers for LED
-   [ ] Specific channels for communication with particular clients


<a id="orga7b35d4"></a>

# Compilation

To compile run `make`.
