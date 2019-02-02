import socket
from threading import Thread, Event

PORT = 3333
SIZE = 1024


class ParentTsock:

    parent_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connections = []
    stop_event = Event()

    def __init__(self):
        self.parent_sock.bind((socket.gethostname(), PORT))
        print("Socket started on address {} and port {}".format(self.parent_sock.getsockname(), PORT))

    def listen_sock(self):
        self.parent_sock.listen(0)
        print("Listening for a connections...")
        while not self.stop_event.is_set():
            (conn, addr) = self.parent_sock.accept()
            print("Got connection from {}".format(addr))
            child_sock = SockThread(conn, addr, SIZE)
            child_sock.run()
            self.connections.append(child_sock)


class SockThread(Thread):

    sock = None
    addr = None
    size = None
    stop_event = Event()

    def __init__(self, conn, addr, size):
        self.sock = conn
        self.addr = addr
        self.size = size

    def run(self):
        self.push_data()

    def push_data(self):
        while not self.stop_event.is_set():
            pass

def main():
    sock = ParentTsock()
    sock.listen_sock()
