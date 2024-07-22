README

The server uses:

    poll_fds: These vector retains the file descriptor (fd) of the socket used
        to communicate with that client. When a client disconnects,
        the corresponding entries are removed from the vector.

    udp_messages: Stores messages received from UDP clients for forwarding to
        active TCP clients who have subscriptions.

    subscribers: Holds the users and keeps track of each user's
        subscriptions, including the topics they are subscribed to,
        whether they are active or not, their id and socket.

Initially, the server starts with three sockets, one for UDP communication,
    one for STDIN and another for listening to new TCP connections.
    Multiplexing is achieved using poll_fds.

    UDP Socket Communication: When data is received on the UDP socket,
        the server constructs a packet (udp_msg) to be sent to all
        active TCP clients based on their subscriptions. This step
        involves building the packet with the correct information and
        forwarding it to the appropriate TCP clients.

    TCP Listening Socket: When data is received on the listening socket, it
        indicates that a new TCP client wants to connect. The server creates
        a new socket for this communication, verifies if the ID is valid, and
        sends back a response indicating whether the client can successfully
        connect to the server.

    STDIN Command: If data is received from the server's
        standard input (STDIN), it signifies a server command. Currently, the
        only recognized command is exit, which stops the server and sends
        shutdown notifications to all connected clients.

    TCP Client Communication: This handles actions from connected TCP clients,
        such as subscribing, unsubscribing, or exiting. If the action is exit,
        the server removes the client from poll_fds vector.

At the end of server operations, the server sends a shutdown notification to
    all connected TCP clients and closes all open sockets.


The subscriber creates a TCP socket to communicate with the server.

The subscriber waits for a server response indicating whether it can connect
    or not.

    Receiving Data from the Server:
        If the server is shutting down, the subscriber should also stop.
        If a message is received from the server
            (originating from a UDP client), the subscriber displays the
            message.
    Reading Commands from the Keyboard: The subscriber reads commands from the
        user. Valid commands are processed to prepare a packet to be sent to
        the server, specifying what action the client wants to perform
        (subscribe, unsubscribe, or exit).

At the end, the subscriber closes its TCP socket and terminates the
    communication with the server.

OTHERS:
    - I started the implementation from the seventh laboratory.
    - I disabled the buffering for stdout.
    - I disabled Nagle's algorithm.
