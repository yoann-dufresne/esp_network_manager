# Echo client program
import socket
import time

HOST = '127.0.0.1'    # The remote host
PORT = 4040           # The same port as used by the server
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.send(bytes("ESP fake\n", 'ascii'))
    for i in range(100):
        mystring = f"Yo! {i}\n"
        s.send(bytes(mystring, 'ascii'))
        time.sleep(1)
