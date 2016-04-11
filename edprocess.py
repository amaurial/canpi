
import logging
import threading
import select
import time
import tcpmodule
import canmodule

#class that deals with the ED messages, basically this class holds the client socket and the major code

class TcpClientHandler(threading.Thread):

    # client is the tcp client
    # canwriter is the BufferWriter
    def __init__(self,client, address,canwriter):
        threading.Thread.__init__(self)
        self.client = client
        self.can = canwriter
        self.canmsg = []
        self.sessions = [] #array of sessions
        self.running = True

    #receive all the can message
    def canmessage(self,canid,size,data):
        #put message on the buffer
        self.canmsg.append(data)
        self.client.send(str(canid).encode(encoding='ascii'))
        self.client.send(b'-')
        self.client.sendall(data.encode(encoding='ascii'))
        self.client.send(b'\n')

    def stop(self):
        self.running = False

    def run(self):
        logging.debug("serving the tcp client %[self.address]s" %locals())
        size = 1024
        while self.running:
            try:
                ready = select.select([self.client],[],[],1)
                if ready[0]:
                    data = self.client.recv(size)
                    if data:
                        response = data
                        self.client.send(response)
                    else:
                        raise Exception('Client disconnected')
            except:
                logging.debug("exception in client processing")
                self.client.close()
                raise
                return False
        logging.debug("Tcp server closing client socket for %[self.address]s " %locals())
        self.client.close()


class EdSession:
    def __init__(self,sessionid):
        self.sessionid = sessionid
        self.keepalivetime = 0
