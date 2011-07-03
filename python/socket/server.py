#!/usr/bin/python2

"""

Basic server largely influenced on SocketServer.py. However this server manage
to handle multiple connections by sending request to handler threads. This is not
really SocketServer.ThreadingMixIn because a thread is not created on each new connection.
Each client connection is interpreted as a BaseClientSession. The server can be
customized easily by defining handlers and sessions.
Server.handleNewConnection() can be overriden to define how the sessions are
created.

"""

import select
import socket
import threading
import Queue


class NotBoundException(Exception):
  """Tried socket operation while not bounded"""
  def __init__(self, msg):
    super(NotBoundException, self).__init__()
    self.msg = msg

  def __str__(self):
    return self.msg
    

class NotListeningException(Exception):
  """Tried to accept connection while not listening"""
  def __init__(self, msg):
    super(NotListeningException, self).__init__()
    self.msg = msg
    
  def __str__(self):
    return self.msg


class ServerException(Exception):
  """Server internal exception"""
  def __init__(self, msg):
    super(ServerException, self).__init__()
    self.msg = msg

  def __str__(self):
    return self.msg
    

class BaseClientSession(object):
  """Session with a remote client"""
  def __init__(self, sock, address, *args, **kwargs):
    super(BaseClientSession, self).__init__()
    self._conn = sock
    self._address = address
    
  conn = property(lambda self: self._conn, doc="Client connection")
  address = property(lambda self: self._address, doc="Client address")

  def __repr__(self):
    return str(self._address)

  def __str__(self):
    return self.__repr__()


class HandlerResponse(object):
  """Base class of HandlerResponse"""

  name = "Base handler response"

  def __init__(self, session, *args, **kwargs):
    super(HandlerResponse, self).__init__()
    self._session = session

  session = property(lambda self: self._session, doc="Client session")


class RequestComplete(HandlerResponse):
  """Mean request completed successfully"""

  name = "Request successfull"

  def __init__(self, *args, **kwargs):
    super(RequestComplete, self).__init__(*args, **kwargs)


class ClientDisconnect(HandlerResponse):
  """Mean remote client has disconnected"""

  name = "Remote client disconnect"

  def __init__(self, *args, **kwargs):
    super(ClientDisconnect, self).__init__(*args, **kwargs)



class SessionServer(object):

  DEFAULT = {
    'NB_HANDLER' : 5,
    'ADDR' : socket.gethostbyname(socket.gethostname()),
    'PORT' : 5080,
    'SOCK_FAMILY' : socket.AF_INET,
    'SOCK_TYPE' : socket.SOCK_STREAM,
    'POLL_TIME' : 0.5,
    #REUSE set sockopt SO_REUSEADDR:
    #Indicates that the rules used in validating addresses supplied in a bind(2)
    #call should allow reuse of local addresses.This socket option tells the 
    #kernel that even if this port is busy (in the TIME_WAIT state), go ahead 
    #and reuse it anyway.  If it is busy, but with another state, error an 
    #address already in use will still happen.
    'REUSE' : False
  }

  def __init__(self, handlerClass, sessionClass, 
               nbHandler=DEFAULT['NB_HANDLER'], address=DEFAULT['ADDR'],
               port=DEFAULT['PORT'], family=DEFAULT['SOCK_FAMILY'],
               type=DEFAULT['SOCK_TYPE'], pollTime=DEFAULT['POLL_TIME'],
               reuse=DEFAULT['REUSE']):
    super(SessionServer, self).__init__()
    self.handlerClass = handlerClass
    self.sessionClass = sessionClass
    self.nbHandler = nbHandler
    self.sock = socket.socket(family, type)
    self.address = address
    self.port = port
    self.pollingTime = pollTime
    self.reuseConn = True
    self.bound = False
    self.listening = False
    self.running = False
    self.verbose = False

    self._clientSession = {} #Contain all active sessions
    self._activeSession = [] #List of all session being currently processed

    self._requestQueue = Queue.Queue(100) #Put request for the handler threads
    self._responseQueue = Queue.Queue(100) #Response from the handler threads
    self._stop = threading.Event() 

    self._handlers = []
    for i in xrange(self.nbHandler):
      self._handlers.append(self.handlerClass(i, self._requestQueue,
                                              self._responseQueue))

  
  def bind(self):
    """Bind server to self.address:self.port"""
    
    if self.reuseConn:
      self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
      self.sock.bind((self.address, self.port))
    except socket.error, e:
      raise ServerException("Failed to bind socket to %s:%s ->%s" % 
                            (self.address, self.port, e))

    self.bound = True

  
  def listen(self, backlog=1):
    self.sock.listen(backlog)
    self.listening = True


  def terminate(self):
    """Stop the server, must be called from another thread as run or it will
    deadlock (stuck while waiting for self._stop.wait())"""

    self.running = False
    #Wait for server loop to stop
    self._stop.wait()
    self.bound = False
    self.listening = False


  def fileno(self):
    """Return socket file descriptor. 
    Required by select function."""
    return self.sock.fileno()

  
  def run(self):
    """Server main loop"""
    
    #Make sure server is bound and listening
    if not self.bound:
      self.bind()

    if not self.listening:
      self.listen()

    #Start all handlers
    for handler in self._handlers:
      handler.start()

    self.running = True
    self._stop.clear()

    #While running, retrieve idle connections (client connection that are not
    #currently processed by the handlers. And then call select on that list.
    while self.running:
      inputs = self._getIdleConnection()
      inputs.append(self)
      readRdy, writeRdy, exList = select.select(inputs, [], inputs,
                                                self.pollingTime)
      for readable in readRdy:
        if readable is self:
          self.acceptConnection()
        else:
          self.handleRequest(readable)

      for excep in exList:
        self._print("Exceptionnal condition on socket %s" % excep.getsockname())
        self.closeConnection(excep)

      #Check if handlers have sent any response.
      self.handleResponse()

    #End server
    self._cleanUp()
    self._stop.set()


  def acceptConnection(self):
    """Call socket.accept and then handleNewConnection"""
    
    if not self.bound:
      raise NotBoundException("Can't accept new connection, not bounded...")
   
    elif not self.listening:
      raise NotListeningException("Can't accept new connection, not "\
                                  "listening...")

    else:
      conn, address = self.sock.accept()
      self._print("New connection with %s" % repr(address))
      self.handleNewConnection(conn, address)


  def handleNewConnection(self, conn, address):
    """ Add new session to the _clientSession """
    self._clientSession[conn] = self.sessionClass(conn, address)


  def handleRequest(self, conn):
    """Handle a client request. conn arg is the socket 
    connected to the client."""

    session = self._clientSession[conn]

    self._print("Handling new request from %s" % session)
    self._requestQueue.put(session)
    if session in self._activeSession:
      #This should never happen... unless something goes really bad!
      raise ServerException("Handle request fail because session %s is "\
                            "already active" % session)
    else:
      self._activeSession.append(session)

  
  def handleResponse(self):
    """Handle response from the handler threads"""
    respList = self._getHandlerResponses()

    for resp in respList:

      if isinstance(resp, RequestComplete):
        pass

      if isinstance(resp, ClientDisconnect):
        self.closeConnection(resp.session)

      if resp.session in self._activeSession:
        #resp.session should always be there, unless we've been tricked...
        self._activeSession.remove(resp.session)


  def closeConnection(self, conn):
    """Close remote client connection. conn arg can either be a socket or a
    session"""

    if isinstance(conn, self.sessionClass):
      clSock = conn.conn
      session = conn

    else:
      clSock = conn
      session = self._clientSession[conn]

    assert isinstance(clSock, socket._socketobject), "Close connection must "\
           "be called on a socket or session."

    self._print("Closing connection with %s" % session)
    clSock.shutdown(socket.SHUT_RDWR)
    clSock.close()
    #Remove session from server list.
    del self._clientSession[clSock]


  def setVerbose(self, verbose):
    """If set to true server will print message to stdout"""
    self.verbose = verbose

  
  def _getIdleConnection(self):
    """Return a list of connection that are not being currently served."""
    return [x.conn for x in self._clientSession.values() if x not in
            self._activeSession]


  def _getHandlerResponses(self):
    """Return all response in the response queue if any"""
    empty = False
    respList = []

    while not empty:
      try:
        resp = self._responseQueue.get_nowait()
        respList.append(resp)
      except Queue.Empty:
        empty = True

    return respList

  
  def _print(self, msg):
    """Print message if verbose is set"""
    if self.verbose:
      print(msg)


  def _cleanUp(self):
    """Clean up the server"""
    self.sock.shutdown(socket.SHUT_RDWR)
    self.sock.close()
    

class BaseHandler(threading.Thread):
  """Base class for handler class"""

  def __init__(self, handlerId, requestQ, responseQ):
    threading.Thread.__init__(self)
    self.daemon = True
    self.handlerId = handlerId
    self.reqQ = requestQ
    self.respQ = responseQ
    self.name = "Base Handler"

  def run(self):
    pass
    

class RequestPrinter(BaseHandler):
  
  def __init__(self, handlerId, requestQ, responseQ):
    #We want to exist if main thread die.
    super(RequestPrinter, self).__init__(handlerId, requestQ, responseQ)
    self.name = "Request printer"


  def run(self):
    """Print all request."""

    while True:

      session = self.reqQ.get()
      data = session.conn.recv(1024).strip()

      if not data:
        self.reqQ.task_done()
        self.respQ.put(ClientDisconnect(session))
        
      else:

        print("%s - %s receive data:" % (self.name, self.handlerId))
        print(data)

        #No more data, notify server that request is done.
        self.reqQ.task_done()
        self.respQ.put(RequestComplete(session))


def runServer(server):
  server.run()
  
    
def main():
  import thread

  server = SessionServer(RequestPrinter, BaseClientSession)
  #server.setVerbose(True)
  thread.start_new_thread(runServer, (server, ))
  print("Type q to close server")

  while True:
    msg = raw_input()
    if msg == 'q':
      break

  #Close server
  server.terminate()
  print("Goodbye!")


if __name__ == '__main__':
  main()
