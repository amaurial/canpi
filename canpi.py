__author__ = 'amaurial'

import sys
import os
import canmodule
import tcpmodule
import logging
import time
import signal


logging.basicConfig(level=logging.DEBUG,format='%(asctime)s - %(name)s - %(message)s')

running=True

def receive_signal(signum,stack):
    logging.debug('Signal received. Stopping.')
    bufferReader.stop()
    bufferWriter.stop()
    canThread.stop()
    tcpServer.stop()
    global running
    running = False

signal.signal(signal.SIGINT,receive_signal)
signal.signal(signal.SIGTERM,receive_signal)

device="can0"

logging.debug('Starting CANPI')

canThread = canmodule.CanManager(name="can",threadID=1,device=device)
bufferReader = canmodule.BufferReader(name="bufferReader",threadID=2)
bufferWriter = canmodule.BufferWriter(name="bufferWriter", threadID=3, canManager=canThread)

tcpServer = tcpmodule.TcpServer(host="pihost" ,port=4444, bufwriter=bufferWriter)
bufferReader.register(tcpServer)

bufferReader.start()
bufferWriter.start()
canThread.start()
tcpServer.start()

while running:
    #do nothing
    time.sleep(1)


canThread.stop()
tcpServer.stop()
logging.debug("Finishing %i" %os.getpid())
os.kill(os.getpid(), 9)
#sys.exit()