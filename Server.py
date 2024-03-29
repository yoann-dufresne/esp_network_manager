from threading import Thread
from time import sleep, time
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
        # Functions that are called after each connection
        self.handlers = []


    def run(self):
        # Create + bind the socket
        self.socket = socket.create_server(("", self.port))
        '''
        self.socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM | socket.TCP_NODELAY)
        # try to reduce delay (doesn't work)
        self.socket.setblocking(False)
        # self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.socket.bind(("",self.port))
        self.socket.listen(5)
        '''

        print("socket server created on port", self.port)

        
        idx=1
        self.started = True

        print("Socket server main loop")
        while not self.stopped:
            try:
                # Create a socket for the new connection
                clientsocket, address = self.socket.accept()
                clientsocket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                print("New connection from", address)
                client = ESPClient(self, clientsocket)
                # Call the callbacks on connection
                for handler in self.handlers:
                    handler(client)
                # Start the client thread
                client.start()
                self.clients[address] = client
            except OSError:
                continue

        # Stop the socket before the end of the thread
        self.socket.close()


    def register_connection_handler(self, function):
        self.handlers.append(function)


    def stop(self):
        # Close all connections
        for client in self.clients.values():
            if not client.stopped:
                client.stop()

        # Close the server
        self.socket.shutdown(socket.SHUT_RDWR)
        self.stopped = True
        print("Server stop triggered")
        self.join()

        print("Server stopped")


    def esp_connected(self, client):
        pass



class ESPClient(Thread):

    def __init__(self, server, socket):
        super().__init__()
        # Socket objects
        self.server = server
        self.socket = socket
        self.stopped = False
        # Network + physical addresses
        self.ip, self.port = self.socket.getpeername()
        self.mac = None
        # byte stream received
        self.raw_bytes = []
        # Awaiting message list
        self.messages = []
        # Functions called when a message has been fully received
        self.handlers = []


    def send_msg(self, msg):
        """ 
        @param msg A byte array to send. Its size must be <= 254
        """
        self.socket.send(len(msg).to_bytes(length=1, byteorder='big', signed=False) + msg)


    def receive_and_parse_messages(self):
        # --- Receive data ---
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

        if self.stopped:
            return

        # --- parse messages ---

        self.raw_bytes.extend(data)

        msg_size = int.from_bytes([data[0]], "big", signed=False)

        if len(data) < msg_size + 1:
            print("Trunckated message received", file=stderr)
            return

        # self.send_msg("ACK".encode("ascii"))
        self.messages.append(data[1:msg_size+1])
        
        # Update the raw bytes removing extracted messages
        self.raw_bytes = self.raw_bytes[msg_size+1:]


    def run(self):
        # Looks for esp first connection
        while not self.stopped:
            self.receive_and_parse_messages()
            if len(self.messages) == 0:
                sleep(.01)
                continue

            # Awaits for ESP handcheck message "ESP <mac address>"
            msg = self.messages.pop(0)
            head = msg[:4].decode('ascii')
            # print(split)
            if head != "ESP ":
                # Wrong handcheck
                self.stopped = True
                print(f"Client {self.ip}:{self.port} is not an awaited ESP", file=stderr)
            else:
                self.started = True
                self.mac = msg[4:]
                print(f"Connected  to ESP {list(self.mac)}")
                self.server.esp_connected(self)
                break

        # Main loop. Forward messages to registered callbacks
        while not self.stopped:
            # Get messages
            self.receive_and_parse_messages()

            # Transmit the messages
            while len(self.messages) > 0:
                msg = self.messages.pop(0)
                for handler in self.handlers:
                    # WARNING: The handler must be fast to not miss new messages
                    handler(self, msg)

        self.socket.close()


    def stop(self):
        if not self.stopped:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.stopped = True
        self.join()


    def register_message_handler(self, esp_client, function):
        """ Register a callback function that will be called when a message has been received.
            WARNING: The callback must be fast to execute. If not, some messages can be lost.
        """
        self.handlers.append(function)


def connection_callback(esp_client):
    esp_client.register_message_handler(esp_client, msg_callback_test)

def msg_callback_test(esp_client, msg):
    print("received message:", repr(msg))


if __name__ == "__main__":
    server = ESPServer()
    signal(SIGINT, lambda sig, frame : server.stop())
    server.register_connection_handler(connection_callback)
    server.start()