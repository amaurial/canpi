
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
EMPTY_LABELS = "<;>]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[\n" #MTS3<......MTLS1<;>]\[]\[]
EMPTY_F = "<;>F00\nMTAS1<;>F01\nMTAS1<;>F02\nMTAS1<;>F03\nMTAS1<;>F04\nMTAS1<;>F05\nMTAS1<;>F06\nMTAS1<;>F07\nMTAS1<;>F08\nMTAS1<;>F09\nMTAS1<;>F010" \
          "\nMTAS1<;>F011\nMTAS1<;>F012\nMTAS1<;>F013\nMTAS1<;>F014\nMTAS1<;>F015\nMTAS1<;>F016\nMTAS1<;>F017\nMTAS1<;>F018\nMTAS1<;>F019\nMTAS1<;>F020" \
          "\nMTAS1<;>F021\nMTAS1<;>F022\nMTAS1<;>F023\nMTAS1<;>F024\nMTAS1<;>F025\nMTAS1<;>F026\nMTAS1<;>F027\nMTAS1<;>F028" \
          "\nMTAS1<;>V0\nMTAS1<;>R1\nMTAS1<;>s0\n"


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
        self.edsession = EdSession(0, "S")


    def stop(self):
        self.running = False

    def run(self):
        logging.info("Serving the tcp client %s" % self.address[0])

        #send the version software info and the throttle data
        logging.debug("Sending start info :%s" %START_INFO)
        self.sendClientMessage(START_INFO)

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

            #put session in array
            self.edsession.setSessionID(session)
            self.sessions[session] = self.edsession

            message = "MT+" + self.edsession.getAdType() + str(loco) + "<;>\n"
            logging.debug("Ack client session created %d for loco %d :%s" % (session, loco, message))
            self.sendClientMessage(message)

            #set speed mode 128
            self.can.put(OPC_STMOD + bytes([session]) + b'\x00')

            #send the labels
            labels = "MTLS" + str(loco) + EMPTY_LABELS + self.generateFunctionsLabel("S" + str(loco))
            logging.debug("Sending labels: %s " % labels )

            self.sendClientMessage(labels)

        if opc == OPC_ERR[0]:
            logging.debug("OPC: ERR")
            loco = int(data[2])
            err = int(data[3])
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
            logging.debug("Put session set timer in queue")
            self.sendClientMessage("*10") #keep alive each 10 seconds
            #TODO wait for confirmation: expected 0xa
            return

        #get the session request

        messages = message.split("\n")
        logging.debug("Split message: %s" % messages)

        for msg in messages:
            logging.debug("Processing message: %s" % msg)
            logging.debug("Expected:%s" % msg[0:3])
            #create session
            if msg[0:3] in ["MT+","MS+"]:
                self.handleCreateSession(msg)
            #query the loco
            if msg[0:3] in ["MTA","MSA"]:
                self.handleSpeedDir(msg)

    def handleCreateSession(self,msg):
        #MT+S3<;>
        logging.debug("MT+ found")
        #create session
        if msg[3] in ["S", "s", "L", "l"]:
            logging.debug("MT+S found")
            adtype = msg[3]
            logging.debug("Address type: %s" % adtype)
            #get loco
            i = msg.find("<")
            logging.debug("Extracted loco: %s" % msg[4:i])
            loco = int(msg[4:i])
            self.edsession.setLoco(loco)
            self.edsession.setAdType(adtype)
            #send the can data
            logging.debug("Put CAN session request in the queue for loco %d" % loco)
            self.STATE = self.STATES['WAITING_SESS_RESP']
            self.can.put(OPC_RLOC + b'\x00' + bytes([loco]))
            return

    #TODO
    def handleSpeedDir(self,msg):
        #MT+S3<;>
        logging.debug("MTA found")
        #create session        
        i = msg.find("<")
        logging.debug("Extracted loco: %s" % msg[3:i])
        loco = int(msg[4:i])
        #send the can data
        logging.debug("Put CAN session request in the queue for loco %d" % loco)
        #TODO
        self.STATE = self.STATES['WAITING_SESS_RESP']
        self.can.put(OPC_RLOC + b'\x00' + bytes([loco]))
        return

    def sendClientMessage(self, message):
        self.client.sendall(message.encode('utf-8'))

    def generateFunctionsLabel(self,locoaddr):
        s = "MTA" + locoaddr + "<;>" #MTS1<;>
        a = ""
        for f in range(0,29):
            a = a + s + "F0" + str(f) + "\n"
        a = a + s + "V0\n" + s + "R1\n" + s +  "s0\n"
        return a
class EdSession:
    def __init__(self,loco,adtype):
        self.sessionid = 0
        self.keepalivetime = 0
        self.loco = loco
        self.adtype = adtype #S =short address or L = long adress
        self.speedDir = 0

    def getSessionId(self):
        return self.sessionid
    def setSessionID(self,sessionid):
        self.sessionid = sessionid
    def getLoco(self):
        return self.loco
    def setLoco(self,loco):
        self.loco = loco
    def getAdType(self):
        return self.adtype
    def setAdType(self,adtype):
        self.adtype = adtype
    def setSpeedDir(self,speedDir):
        self.speedDir = speedDir
    def getSpeedDir(self):
        return self.speedDir

class State:
    def __init__(self,name):
        self.name = name
    def __str__(self):
        return self.name

    def next(self,input):
        logging.debug("state")


