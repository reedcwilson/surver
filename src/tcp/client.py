#!/usr/bin/env python

import socket

s = socket.socket()
host = socket.gethostname()
port = 5000

s.connect((host, port))
print s.recv(8024)
s.close()
