
import logging
import threading
import select
from opc import *
import time
import tcpmodule
import canmodule

#class that deals with the ED messages, basically this class holds the client socket and the major code

SOFT_VERSION = "VN2.0"
ROASTER_INFO = "RL0]|[" #no locos
START_INFO = "VN2.0\n\rRL0]|[\n\rPPA1\n\rPTT]|[\n\rPRT]|[\n\rRCC0\n\rPW12080\n\r"
DELIM_BRACET = "]|["
DELIM_BTLT = "<;>"
DELIM_KEY = "}|{"
EMPTY_LABES = "<;>]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[" #MTS3<.....


class TcpClientHandler(threading.Thread):

    STATES = {'START': 255, 'WAITING_ED_RESP': 0, 'WAITING_SESS_RESP': 1 , 'SESSION_ON': 2 }
    STATE = ''

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


    def stop(self):
        self.running = False

    def run(self):
        logging.info("Serving the tcp client %s" % self.address[0])

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
        logging.debug("Tcp server closing client socket for %(self.address)s " % locals())
        self.client.close()

    #receive all the can message
    def canmessage(self,canid,size,data):

        #put message on the buffer
        #self.canmsg.append(data)
        logging.debug("Tcpclient received can msg: %s" % data)

        logging.debug("Opc: %s" % hex(data[0]))
        opc = data[0]

        if opc == OPC_PLOC[0]:
            logging.debug("OPC: PLOC")
            session = int(data[1])
            loco = int(data[3])
            speedir = int(data[4])
            f1 = data[5]
            f1 = data[6]
            f1 = data[7]
            edsession = EdSession(session, loco, "S")
            self.sessions[session] = edsession
            logging.debug("Aack client session created %d for loco %d" % (session, loco))
            self.client.sendClientMessage("MT+S" + str(loco).decode('ascii') + "<;>\n")

        if opc == OPC_ERR[0]:
            logging.debug("OPC: ERR")
            session = int(data[1])
            loco = int(data[3])
            err = int(data[4])
            if err == 1:
                logging.debug("Can not create session. Reason: stack full")
            elif err==2:
                logging.debug("Err: Loco %d TAKEN" % loco)
            elif err==3:
                logging.debug("Err: No session %d" % loco)
            else:
                logging.debug("Err code: %d" % err)

        #self.client.send(str(canid).encode(encoding='ascii'))
        #self.client.send(b'-')
        #self.client.sendall(data.encode(encoding='ascii'))
        #self.client.send(b'\n')

    def handleEdMessages(self, message):
        logging.debug("Handling the client message :%s" % message)
        #get the name
        if message[0] == 'N':
            self.edname = message[1:]
            logging.debug("ED name: %s" % self.edname)
            return

        #get hardware info
        if message[0:1] == "HU":
            self.hwinfo = message[2:]
            logging.debug("Received Hardware info: %s" % self.hwinfo)
            #TODO
            #send the can data
            logging.debug("Put CAN session set timer in queue")
            self.can.put("*10") #keep alive each 10 seconds
            return

        #get the session request

        messages = message.split("\n")
        logging.debug("Split message: %s" % messages)

        for msg in messages:
            logging.debug("Processing message: %s" % msg)
            logging.debug("Expected:%s" % msg[0:3])
            if msg[0:3] in ["MT+","MS+"]: #multi throttle
                #MT+S3<;>
                logging.debug("MT+ found")
                #create session
                if msg[3] in ["S", "s", "L", "l"]:
                    logging.debug("MT+S found")
                    adtype = message[3]
                    #get loco
                    i = msg.find("<")
                    logging.debug("Extracted loco: %s" % msg[4:i])
                    loco = int(msg[4:i])
                    #send the can data
                    logging.debug("Put CAN session request in the queue for loco %d" % loco)
                    #TODO
                    self.STATE = self.STATES['WAITING_SESS_RESP']
                    self.can.put(OPC_RLOC + b'\x00' + bytes([loco]))
                    return

    def sendClientMessage(self, message):
        self.client.sendall(message)

class EdSession:
    def __init__(self,sessionid,loco,adtype):
        self.sessionid = sessionid
        self.keepalivetime = 0
        self.loco = loco
        self.adtype = adtype #S =short address or L = long adress

        def getSessionId(self):
            return self.sessionid
        def getLoco(self):
            return self.loco
        def getAdType(self):
            return self.adtype

class State:
    def __init__(self,name):
        self.name = name
    def __str__(self):
        return self.name

    def next(self,input):
        logging.debug("state")


