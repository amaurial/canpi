
import logging
import threading
import select
from opc import *
import re
import time

#class that deals with the ED messages, basically this class holds the client socket and the major code

SOFT_VERSION = "VN2.0"
ROASTER_INFO = "RL0]|["  # no locos
START_INFO = "VN2.0\n\rRL0]|[\n\rPPA1\n\rPTT]|[\n\rPRT]|[\n\rRCC0\n\rPW12080\n\r"
DELIM_BRACET = "]|["
DELIM_BTLT = "<;>"
DELIM_KEY = "}|{"
EMPTY_LABELS = "<;>]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[\n" #MTS3<......MTLS1<;>]\[]\[]

RE_SPEED = re.compile('M[TA]+[SL\*]([0-9]+)?<;>V[0-9]+') #regex to identify a set speed message
RE_SESSION = re.compile('M[TA]+\+[SL][0-9]+<;>\\w') #regex to identify a create session message
RE_REL_SESSION = re.compile('M[TA]+\-[SL*]([0-9]+)?<;>\\w') #regex to identify a release session message
RE_DIR = re.compile('M[TA]+[SL\*]([0-9]+)?<;>R[0-1]') #regex to identify a create session message
RE_QRY_SPEED = re.compile('M[TA]+[SL\*]([0-9]+)?<;>qV') #regex to identify a query speed
RE_QRY_DIRECTION = re.compile('M[TA]+[SL\*]([0-9]+)?<;>qR') #regex to identify a query direction
RE_FUNC = re.compile('M[TA]+[SL\*]([0-9]+)?<;>F[0-9]+') #regex to identify a query direction

CBUS_KEEP_ALIVE = 5000
ED_KEEP_ALIVE = 10000
BS = 128
ST = 0.003

class TcpClientHandler(threading.Thread):

    STATES = {'START': 255, 'WAITING_ED_RESP': 0, 'WAITING_SESS_RESP': 1, 'SESSION_ON': 2}
    STATE = ''

    # client is the tcp client
    # canwriter is the BufferWriter
    def __init__(self, client, address, canwriter, server, id):
        threading.Thread.__init__(self)
        self.client = client
        self.can = canwriter
        self.address = address
        self.canmsg = []
        self.sessions = {} #dict of sessions; key is the loco number
        self.running = True
        self.edsession = EdSession(0, "S")
        self.edname = ""
        self.hwinfo = ""
        self.server = server
        self.id = id

    def stop(self):
        self.running = False

    #main loop thread function
    def run(self):
        logging.info("Serving the tcp client %s" % self.address[0])

        #send the version software info and the throttle data
        logging.debug("Sending start info :%s" %START_INFO)
        self.sendClientMessage(START_INFO)

        size = 1024
        while self.running:
            try:
                ready = select.select([self.client], [], [], 1)
                if ready[0]:
                    data = self.client.recv(size)
                    if data:
                        response = data
                        self.handleEdMessages(data.decode("utf-8"))
                        #self.client.send(response)
                    else:
                        #raise Exception('Client disconnected')
                        logging.debug("Exception in client processing. Closing connection")
                        self.running = False
                #send keep alive in both directions
                self.sendKeepAlive()
            except:
                logging.debug("Exception in client processing")
                self.running = False
                #del self.sessions
                self.server.removeClient(self.id)
                self.client.close()
                self.stop()

        logging.debug("Tcp server closing client socket for %s " % self.address[0])
        #del self.sessions
        self.server.removeClient(self.id)
        self.client.close()
        self.stop()


    def sendKeepAlive(self):
        for k,s in self.sessions.items():
            if s:
                #10 seconds for tcp
                #2 seconds for cbus
                millis = int(round(time.time() * 1000))
                if (millis - s.getClientTime()) > ED_KEEP_ALIVE:
                    #send enter
                    logging.debug("Sending keep alive to ED for loco %d" % s.getLoco())
                    self.sendClientMessage("\n")
                    m = int(round(time.time() * 1000))
                    self.sessions.get(s.getLoco()).setClientTime(m)

                if (millis - s.getCbusTime()) > CBUS_KEEP_ALIVE:
                    #send keep alive
                    logging.debug("Sending keep alive to Cbus for loco %d" % s.getLoco())
                    self.can.put(OPC_DKEEP + bytes([s.getSessionID()]))
                    m = int(round(time.time() * 1000))
                    self.sessions.get(s.getLoco()).setCbusTime(m)

    #receive all the can message
    def canmessage(self, canid, size, data):

        logging.debug("Tcpclient received can msg: %s" % data)
        logging.debug("Opc: %s" % hex(data[0]))
        opc = data[0]

        if opc == OPC_PLOC[0]:

            logging.debug("OPC: PLOC %s %s" % (hex(data[2]) , hex(data[3])))

            session = int(data[1])
            Hb = data[2] & 0x3f
            Lb = data[3]

            loco = int.from_bytes(bytes([Hb, Lb]),byteorder='big')
            speedir = int(data[4])
            f1 = data[5]
            f2 = data[6]
            f3 = data[7]

            speed = speedir & 0x7F #0111 1111
            direction = 0
            if (speedir & 0x80) > 127:
                direction = 1
            #put session in array
            self.edsession.setDirection(direction)
            self.edsession.setSpeed(speed)
            self.edsession.setSessionID(session)
            logging.debug("Adding loco %d to sessions" % loco)
            self.sessions[loco] = self.edsession

            message = "MT+" + self.edsession.getAdType() + str(loco) + DELIM_BTLT + "\n"
            logging.debug("Ack client session created %d for loco %d :%s" % (session, loco, message))
            self.STATE = self.STATES['SESSION_ON']
            self.sendClientMessage(message)

            #set speed mode 128 to can
            self.can.put(OPC_STMOD + bytes([session]) + b'\x00')

            #send the labels to client
            labels = "MTLS" + str(loco) + EMPTY_LABELS + self.generateFunctionsLabel("S" + str(loco))
            logging.debug("Sending labels: %s " % labels)

            self.sendClientMessage(labels)

        if opc == OPC_ERR[0]:
            logging.debug("OPC: ERR")
            loco = int(data[2])
            err = int(data[3])
            if err == 1:
                logging.debug("Can not create session. Reason: stack full")
            elif err == 2:
                logging.debug("Err: Loco %d TAKEN" % loco)
            elif err == 3:
                logging.debug("Err: No session %d" % loco)
            else:
                logging.debug("Err code: %d" % err)

        #self.client.send(str(canid).encode(encoding='ascii'))
        #self.client.send(b'-')
        #self.client.sendall(data.encode(encoding='ascii'))
        #self.client.send(b'\n')

    def handleEdMessages(self, message):
        logging.debug("Handling the client message :%s" % message)

        messages = message.split("\n")
        logging.debug("Split message: %s" % messages)

        for msg in messages:
            logging.debug("Processing message: %s" % msg)

            if len(msg) == 0:
                continue

            #get the name
            if msg[0] == 'N':
                self.edname = msg[1:]
                logging.debug("ED name: %s" % self.edname)
                self.sendClientMessage("\n")
                continue

            #get hardware info
            if msg[0:1] == "HU":
                self.hwinfo = msg[2:]
                logging.debug("Received Hardware info: %s" % self.hwinfo)
                logging.debug("Put session set timer in queue")
                self.sendClientMessage("*10\n") #keep alive each 10 seconds
                #TODO wait for confirmation: expected 0xa
                continue

            #create session

            s = RE_SESSION.match(msg)
            if s:
                logging.debug("Create session %s" % msg)
                self.handleCreateSession(msg)
                # wait until session is created
                logging.debug("Waiting 2 secs for session to be created.")
                time.sleep(2)
                if len(self.sessions) > 0:
                    logging.debug("Session created after 2 secs.")
                    logging.debug(self.sessions)

            #set speed
            v = RE_SPEED.match(msg)
            if v:
                self.handleSpeedDir(msg)
                #time.sleep(ST)

            d = RE_DIR.match(msg)
            if d:
                self.handleDirection(msg)

            q = RE_QRY_SPEED.match(msg)
            if q:
                self.handleQuerySpeed(msg)

            q = RE_QRY_DIRECTION.match(msg)
            if q:
                self.handleQueryDirection(msg)

            r = RE_REL_SESSION.match(msg)
            if r:
                self.handleReleaseSession(msg)

            f = RE_FUNC.match(msg)
            if f:
                self.handleSetFunction(msg)

    def getLoco(self,msg):
        i = msg.find("<")
        logging.debug("Extracted loco: %s" % msg[4:i])
        loco = int(msg[4:i])
        return loco

    def handleCreateSession(self, msg):
        logging.debug("Handle create session. MT+ found")
        #create session
        if msg[3] in ["S", "s", "L", "l"]:
            logging.debug("MT+S found")
            adtype = msg[3]
            logging.debug("Address type: %s" % adtype)
            #get loco
            loco = self.getLoco(msg)
            self.edsession.setLoco(loco)
            self.edsession.setAdType(adtype)
            #send the can data
            logging.debug("Put CAN session request in the queue for loco %d" % loco)
            self.STATE = self.STATES['WAITING_SESS_RESP']

            Hb = 0
            Lb = 0
            if loco > 127:
                Hb = loco.to_bytes(2,byteorder='big')[0] | 0xC0
                Lb = loco.to_bytes(2,byteorder='big')[1]
            else:
                Lb = loco.to_bytes(2,byteorder='big')[1]

            self.can.put(OPC_RLOC + bytes([Hb]) + bytes([Lb]))
            return

    def handleReleaseSession(self, msg):
        logging.debug("Handle release session. MT- found")
        #release session

        i = msg.find("*")
        relsessions = []
        #all sessions
        if i > 0:
            logging.debug("Releasing all sessions")
            for k , s in self.sessions.items():
                if s:
                     #send the can data
                    logging.debug("Releasing session for loco %d" % s.getLoco())
                    self.can.put(OPC_KLOC + bytes([s.getSessionID()]))
                    relsessions.append(k)
            #clear sessions
            for k in relsessions:
                del self.sessions[k]
            return

        loco = self.getLoco(msg)

        session = self.sessions.get(loco)
        if session:
            #send the can data
            logging.debug("Releasing session for loco %d" % loco)
            self.can.put(OPC_KLOC + bytes([session.getSessionID()]))
            #TODO check if it works
            del self.sessions[loco]
        else:
            logging.debug("No session found for loco %d" % loco)

    #TODO
    def handleSpeedDir(self, msg):
        logging.debug("Handle speed request")
        self.sendClientMessage("\n")

        i = msg.find(">V")
        logging.debug("Extracted speed: %s" % msg[i+2:])
        speed = int(msg[i+2:])

        i = msg.find("*")
        #all sessions
        if i > 0:
            logging.debug("Set speed for all sessions")
            for k, s in self.sessions.items():
                if s:
                    logging.debug("Set speed %d for loco %d" % (speed, s.getLoco()))
                    self.sessions.get(s.getLoco()).setSpeed(speed)
                    sdir = s.getDirection() * BS + speed
                    self.can.put(OPC_DSPD + bytes([s.getSessionID()]) + bytes([sdir]))
            return

        #one session
        loco = self.getLoco(msg)
        session = self.sessions.get(loco)

        if session:
            #send the can data
            logging.debug("Set speed %d for loco %d" % (speed, loco))
            self.sessions.get(loco).setSpeed(speed)
            sdir = session.getDirection() * BS + speed
            self.can.put(OPC_DSPD + bytes([session.getSessionID()]) + bytes([sdir]))
        else:
            logging.debug("No session found for loco %d" % loco)

    def handleDirection(self, msg):
        logging.debug("Handle Direction request")
        self.sendClientMessage("\n")


        #get the direction
        i = msg.find(">R")
        logging.debug("Extracted direction: %s" % msg[i + 2:])
        d = int(msg[i + 2:])

        i = msg.find("*")
        #all sessions
        if i > 0:
            logging.debug("Set direction for all sessions")
            for k,s in self.sessions.items():
                if s:
                    if d != s.getDirection():
                        #send the can data
                        self.sessions.get(s.getLoco()).setDirection(d)
                        logging.debug("Set direction %d for loco %d" % (d, s.getLoco()))
                        sdir = d * BS + s.getSpeed()
                        self.can.put(OPC_DSPD + bytes([s.getSessionID()]) + bytes([sdir]))
            return

        #one session
        loco = self.getLoco(msg)
        session = self.sessions.get(loco)
        if session:
            if d != session.getDirection():
                #send the can data
                self.sessions.get(loco).setDirection(d)
                logging.debug("Set direction %d for loco %d" % (d, loco))
                sdir = d * BS + session.getSpeed()
                self.can.put(OPC_DSPD + bytes([session.getSessionID()]) + bytes([sdir]))
        else:
            logging.debug("No session found for loco %d" % loco)

    def handleQueryDirection(self, msg):
        logging.debug("Query Direction found")
        i = msg.find("*")
        if i > 0:
            logging.debug("Query direction for all locos")
            for k,s in self.sessions.items():
                if s:
                    self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "R" + str(s.getDirection()) + "\n")
            return
        #get specific loco
        #TODO
        #get the direction
        loco = self.getLoco(msg)
        s = self.sessions.get(loco)
        if s:
            self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "R" + str(s.getDirection()) + "\n")

    def handleQuerySpeed(self, msg):
        logging.debug("Query speed found")
        i = msg.find("*")
        #all sessions
        if i > 0:
            logging.debug("Query direction for all locos")
            for k,s in self.sessions.items():
                if s:
                    self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "V" + str(s.getSpeed()) + "\n")
            return

        #get specific loco
        loco = self.getLoco(msg)

        s = self.sessions.get(loco)
        if s:
            self.sendClientMessage("MTA" + s.getAdType() + str(s.getLoco()) + DELIM_BTLT + "R" + str(s.getSpeed()) + "\n")

    def handleSetFunction(self, msg):
        logging.debug("Set function request found")

        #get the function
        i = msg.find(">F")
        logging.debug("Extracted on/off: %s func: %s" % (msg[i + 2:i + 3], msg[i + 3:]))
        onoff = int(msg[i + 2:i + 3])
        fn = int(msg[i + 3:])

        i = msg.find("*")
        #all sessions
        if i > 0:
            for k,s in self.sessions.items():
                if s:
                    #send the can data
                    logging.debug("Set function %d for loco %d" % (fn, s.getLoco()))
                    if onoff == 0:
                        self.can.put(OPC_DFNOF + bytes([s.getSessionID()]) + bytes([fn]))
                    else:
                        self.can.put(OPC_DFNON + bytes([s.getSessionID()]) + bytes([fn]))
            return

        #one session
        loco = self.getLoco(msg)
        session = self.sessions.get(loco)
        if session:
            #send the can data
                logging.debug("Set function %d for loco %d" % (fn, session.getLoco()))
                if onoff == 0:
                    self.can.put(OPC_DFNOF + bytes([session.getSessionID()]) + bytes([fn]))
                else:
                    self.can.put(OPC_DFNON + bytes([session.getSessionID()]) + bytes([fn]))
        else:
            logging.debug("No session found for loco %d" % loco)

    def sendClientMessage(self, message):
        self.client.sendall(message.encode('utf-8'))

    @staticmethod
    def generateFunctionsLabel(locoaddr):
        s = "MTA" + locoaddr + DELIM_BTLT #MTS1<;>
        a = ""
        for f in range(0, 29):
            a = a + s + "F0" + str(f) + "\n"
        a = a + s + "V0\n" + s + "R1\n" + s + "s0\n"
        return a
class EdSession:
    def __init__(self, loco, adtype):
        self.sessionid = 0
        self.keepalivetime = 0
        self.loco = loco
        self.adtype = adtype #S =short address or L = long adress
        self.speed = 0
        self.direction = 1 #forward
        self.clientTime = 0
        self.cbusTime = 0

    def getSessionID(self):
        return self.sessionid

    def setSessionID(self, sessionid):
        self.sessionid = sessionid

    def getLoco(self):
        return self.loco

    def setLoco(self, loco):
        self.loco = loco

    def getAdType(self):
        return self.adtype

    def setAdType(self, adtype):
        self.adtype = adtype

    def setSpeed(self, speed):
        self.speed = speed

    def getSpeed(self):
        return self.speed

    def setDirection(self, direction):
        self.direction = direction

    def getDirection(self):
        return self.direction

    def setClientTime(self, millis):
        self.clientTime = millis

    def getClientTime(self):
        return self.clientTime

    def setCbusTime(self, millis):
        self.cbusTime = millis

    def getCbusTime(self):
        return self.cbusTime

class State:
    def __init__(self,name):
        self.name = name
    def __str__(self):
        return self.name

    def next(self,input):
        logging.debug("state")


