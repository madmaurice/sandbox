#!/usr/bin/python2

# Print to stdout server program
import SocketServer

HOST = 'localhost'
PORT = 5080

class PrintServerHandler(SocketServer.StreamRequestHandler):
    
  #Handle close the socket at the end of the transmission should not exit before
  #the client disconnect.
  def handle(self):
    while True:
      self.data = self.request.recv(1024).strip()
      if not self.data: break
      #self.data = self.rfile.readline().strip()
      print "%s wrote:" % repr(self.client_address)
      print self.data

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

if __name__ == '__main__':

  server = SocketServer.TCPServer((HOST, PORT), PrintServerHandler)
  #server = ThreadedTCPServer((HOST, PORT), PrintServerHandler)
  server.serve_forever()
