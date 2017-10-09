# Chat-program
Simple chat program written in C. It uses TCP and basic socket connection.
Chat can be joined either with the client or netcat / telnet.


## Build
You should have ncurses installed on your computer to build the client.
Server can be built with the standard libraries.

## Threads
This chat program is very buggy and uses threads for client socket fd handling.
Of course it should be using select() instead. I might (or may not) update this in future
