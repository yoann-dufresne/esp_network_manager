from threading import Thread
from time import sleep
from signal import signal, SIGINT
from sys import stderr
import socket


class ESPServer(Thread):

    def __init__(self, port=4040):
        super().__init__()
        self.started = False
        self.stopped = False

        self.port = port
        self.socket = None

        self.clients = {}


    def run(self):
        # Create + bind the socket
        self.socket = socket.create_server(("", self.port))
        print("socket server created on port", self.port)
        
        idx=1
        self.started = True

        print("Socket server main loop")
        while not self.stopped:
            try:
                clientsocket, address = self.socket.accept()
                print("New connection from", address)
                client = ESPClient(clientsocket)
                client.start()
                self.clients[address] = client
            except OSError:
                continue

        # Stop the socket before the end of the thread
        self.socket.close()


    def stop(self):
        self.stopped = True

        # Close all connections
        for client in self.clients.values():
            if not client.stopped:
                client.stop()

        # Close the server
        self.socket.shutdown(socket.SHUT_RDWR)
        print("Server stop triggered")
        self.join()

        print("Server stopped")



class ESPClient(Thread):

    def __init__(self, socket):
        super().__init__()
        self.socket = socket
        self.stopped = False
        self.ip, self.port = self.socket.getpeername()
        self.mac = None


    def run(self):
        data = None
        try:
            # Get data to init ESP connection
            data = self.socket.recv(1024)
        except OSError:
            # Brutal disconnection
            self.stopped = True
            print(f"Client {self.ip}:{self.port} disconnected", file=stderr)
            
        if not data:
            # Wrong data
            self.stopped = True
            print(f"Client {self.ip}:{self.port} disconnected", file=stderr)

        if not self.stopped:
            # Awaits for ESP handcheck message "ESP <mac address>"
            split = data.decode('ascii').split(' ')
            print(split)
            if len(split) != 2 or split[0] != "ESP":
                # Wrong handcheck
                self.stopped = True
                print(f"Client {self.ip}:{self.port} is not an awaited ESP", file=stderr)
            else:
                self.started = True
                self.mac = split[1]
                print(f"Connected  to ESP {self.mac}")

        while not self.stopped:
            data = None
            try:
                data = self.socket.recv(1024)
            except OSError:
                continue
            if not data:
                # Connection lost
                self.stopped = True
                print(f"Client {self.ip}:{self.port} disconnected")
                break

            # Use the data
            print("received", repr(data))

        self.socket.close()


    def stop(self):
        if not self.stopped:
            self.stopped = True
            self.socket.shutdown(socket.SHUT_RDWR)
        self.join()


if __name__ == "__main__":
    server = ESPServer()
    signal(SIGINT, lambda sig, frame : server.stop())
    server.start()