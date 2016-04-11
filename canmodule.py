__author__ = 'amaurial'


import socket
import struct
import sys
import threading
import logging
import select
import time

cantimeout = 0.01 #seconds
recbuffer = 16
incan = []

# this component is responsable to read the can messages and put them in the incan buffer

class CanManager(threading.Thread):

    running = True

    # CAN frame packing/unpacking (see `struct can_frame` in <linux/can.h>)
    can_frame_fmt = "=IB3x8s"

    def __init__(self, name, threadID, device):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.device = device
        self.opencan(self.device)

    #major thread function. reads the data and send to the registered tcp servers
    def run(self):
        logging.info("starting canmanager")
        #reads the incomming messages
        while self.running:
            #ready = select.select([self.can],[],[],cantimeout)
            #if ready[0]:
                #read
            cf,addr = self.can.recvfrom(recbuffer)
            incan.append(cf)
            #send data
            #logging.debug("sending")
        self.can.close();

     #open the can socket
    def opencan(self,device):
       self.can = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
       self.can.bind((device, ))

    #send the can data
    def send(self,data):
        logging.debug("sending:" + data)

    #stop the thread
    def stop(self):
        self.running = False


# component that consumes the content of incan buffer and send them to the registered tcpservers
class BufferReader(threading.Thread):

    # CAN frame packing/unpacking (see `struct can_frame` in <linux/can.h>)
    can_frame_fmt = "=IB3x8s"

    def __init__(self, name, threadID):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.running = True
        self.tcpclients = []

    def run(self):
        logging.debug("start reading incomming buffer")
        while self.running:
            if len(incan) > 0:
                cf = incan.pop()
                canid,candlc,data= self.dissect_can_frame(cf)
                datahex=":".join("{:02x}".format(c) for c in data)
                logging.debug('Received: can_id=%x, size=%x, data=%s' %(canid, candlc, datahex) )
                #send to the tcp servers
                if len(self.tcpclients) > 0:
                    for client in self.tcpclients:
                        client.put(canid,candlc,datahex)

    #stop the thread
    def stop(self):
        self.running = False
    #allow a tcp server to register
    def register(self,client):
        self.tcpclients.append(client)
    #unregister a tcp server
    def unregister(self,client):
        for i in range(0,len(self.tcpclients)):
            if self.tcpclients[i].getName() == client.getName():
                del self.tcpclients[i]

    #prepare to send a message
    def build_can_frame(self,can_id, data):
       can_dlc = len(data)
       data = data.ljust(8, b'\x00')
       return struct.pack(self.can_frame_fmt, can_id, can_dlc, data)
    #parse the message
    def dissect_can_frame(self,frame):
       can_id, can_dlc, data = struct.unpack(self.can_frame_fmt, frame)
       return (can_id, can_dlc, data[:can_dlc])


# this component consumes the messages that will be sent to CBUS
class BufferWriter(threading.Thread):
    # CAN frame packing/unpacking (see `struct can_frame` in <linux/can.h>)
    can_frame_fmt = "=IB3x8s"

    def __init__(self, name, threadID, canManager):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.canManager = canManager
        self.outcan = []
        self.running = True

    def run(self):
        logging.debug("start writing outgoing buffer")
        data = "aaa"
        while self.running:
            #if len(self.outcan) > 0:
            #    data = self.outcan.pop()
            self.canManager.send(data)
            time.sleep(1)
    def put(self,data):
        logging.debug("inserting data % in buffer" %data)
        self.outcan.append(data)

    #stop the thread
    def stop(self):
        self.running = False

    def build_can_frame(self,can_id, data):
       can_dlc = len(data)
       data = data.ljust(8, b'\x00')
       return struct.pack(self.can_frame_fmt, can_id, can_dlc, data)

