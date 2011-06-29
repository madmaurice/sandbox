#!/usr/bin/python2


import socket
import sys

HOST = socket.gethostname()
SERVER_PORT = 5080

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
  sock.connect((HOST, SERVER_PORT))
except socket.error, e:
  print("Failed to connect to %s:%s. Error=%s" % (HOST, SERVER_PORT, e))
  sys.exit(1)

print("Type message to send, send q to exit")
while True:
  msg = raw_input('-->')
  if msg == 'q':
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()
    break
  try:
    sock.sendall(msg)
  except socket.error, e:
    print("Failed to send to %s. Error=%s" % (msg, e))
    sys.exit(1)


