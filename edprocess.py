
import logging
import threading
import select
import time
import tcpmodule
import canmodule

#class that deals with the ED messages, basically this class holds the client socket and the major code

SOFT_VERSION = "VN2.0"
ROASTER_INFO = "RL0]|[" #no locos
START_INFO = "VN2.0\n\rRL0]|[\n\rPPA1\n\rPTT]|[\n\rPRT]|[\n\rRCC0\n\rPW080\n\r"
DELIM_BRACET = "]|["
DELIM_BTLT = "<;>"
DELIM_KEY = "}|{"
EMPTY_LABES = "<;>]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[" #MTS3<.....


class TcpClientHandler(threading.Thread):

    # client is the tcp client
    # canwriter is the BufferWriter
    def __init__(self,client, address,canwriter):
        threading.Thread.__init__(self)
        self.client = client
        self.can = canwriter
        self.address = address
        self.canmsg = []
        self.sessions = {} #dict of sessions; key is the loco number
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
        logging.info("serving the tcp client %s" % self.address[0])

        #send the version software info and the throttle data
        logging.debug("Sending start info :%s" %START_INFO)
        self.sendClientMessage(START_INFO.encode('utf-8'))

        size = 1024
        while self.running:
            try:
                ready = select.select([self.client],[],[],1)
                if ready[0]:
                    data = self.client.recv(size)
                    if data:
                        response = data
                        self.handleEdMessages(data.decode("utf-8"))
                        #self.client.send(response)
                    else:
                        raise Exception('Client disconnected')
            except:
                logging.debug("exception in client processing")
                self.client.close()
                raise
                return False
        logging.debug("Tcp server closing client socket for %(self.address)s " %locals())
        self.client.close()

    def handleEdMessages(self, message):
        logging.debug("handling the client message :%" %message)
        #get the name
        if message[0] == 'A':
            self.edname = message[1:]
            #send the can data
            logging.debug("put CAN session set timer in queue")
            #TODO
            self.can.put("*10") #keep alive each 10 seconds
            return

        #get hardware info
        if message[0:1] == "HU"
            self.hwinfo = message[2:]
            return

        #get the session request
        if message[0:1] == "MT": #multi throttle
            #MT+S3<;>
            if message[2] == '+': #create session
                if message[3] in ["S","s","L","l"]:
                    adtype = message[3]
                    #get loco
                    i = message.find("<")
                    loco = message[4:(i-1)]
                    edsession = EdSession(0,loco,adtype)
                    self.sessions[loco] = edsession
                    #send the can data
                    logging.debug("put CAN session request in the queue")
                    #TODO
                    self.can.put("request session")
                    return





    def sendClientMessage(self, message):
        self.client.sendall(message)

class EdSession:
    def __init__(self,sessionid,loco,adtype):
        self.sessionid = sessionid
        self.keepalivetime = 0
        self.loco = loco
        self.adtype = adtype #S =short address or L = long adress

        def getSessionId:
            return self.sessionid
        def getLoco:
            return self.loco
        def getAdType:
            return self.adtype
