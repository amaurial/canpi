__author__ = 'amaurial'


import socket
import threading
import canmodule
import logging
import select
import errno

class TcpServer(threading.Thread):
    def __init__(self, host, port,bufwriter):
        threading.Thread.__init__(self)
        self.host = host
        self.port = port
        self.bufferWriter = bufwriter
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        self.running = True
        self.clients = []

    def stop(self):
        self.running = False;
        logging.debug("Tcp server stopping")
        self.sock.close()

    def run(self):
        logging.debug("starting tcp server on %[self.host]s port = %[self.port]d " % locals())
        self.sock.listen(5)
        while self.running:
            client, address = self.sock.accept()
            client.settimeout(60)
            #self.clients.append(client)
            logging.debug("new tcp client")
            clientHandler = TcpClientHandler(self.bufferWriter,client,address)
            self.clients.append(clientHandler)
            clientHandler.start()
            #threading.Thread(target = self.listenToClient,args = (client,address)).start()
        #close all clients
        logging.debug("Tcp server closing clients")
        for c in self.clients:
            c.stop()
        logging.debug("Tcp server closing main socket")
        self.sock.close()

    def put(self,canid,size,data):
        for c in self.clients:
            # c.send(str(canid).encode(encoding='ascii'))
            # c.send(b'-')
            # c.sendall(data.encode(encoding='ascii'))
            # c.send(b'\n')
            #we can do some filtering if necessary
            c.canmessage(canid,size,data)

    # def listenToClient(self, client, address):
    #     logging.debug("serving the tcp client")
    #     size = 1024
    #     while self.running:
    #         try:
    #             ready = select.select([client],[],[],1)
    #             if ready[0]:
    #                 data = client.recv(size)
    #                 if data:
    #                     response = data
    #                     client.send(response)
    #                 else:
    #                     raise Exception('Client disconnected')
    #         except:
    #             logging.debug("exception")
    #             self.clients.remove(client)
    #             client.close()
    #             raise
    #             return False
    #     logging.debug("Tcp server closing client socket")
    #     client.close()

    def getName(self):
        return self.host + self.port

