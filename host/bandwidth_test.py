import socket
from threading import Thread, Event
import time
import signal

PORT = 3333
SIZE = 1024


class ParentTsock:

    parent_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connections = []
    stop_event = Event()

    def __init__(self, addr):
        self.parent_sock.bind((addr, PORT))
        print("Socket started on address {} and port {}".format(self.parent_sock.getsockname()[0], PORT))

    def listen_sock(self):
        """
        Listen for socket connections from ESP32s. When a one connects, spawn a new thread and push data to it
        :return:
        """
        self.parent_sock.listen(0)
        print("Listening for connections...")
        while not self.stop_event.is_set():
            (conn, addr) = self.parent_sock.accept()
            print("Got connection from {}".format(addr))
            child_sock = SockThread(conn, addr)
            child_sock.run()
            self.connections.append(child_sock)

        if self.connections:
            for connection in self.connections:
                connection.stop()

    def stop(self):
        """
        Ask the thread and all children threads to stop
        :return:
        """
        self.stop_event.set()


class SockThread(Thread):

    sock = None
    addr = None
    stop_event = Event()

    def __init__(self, conn, addr):
        self.sock = conn
        self.addr = addr

    def run(self):
        self.push_data()

    def push_data(self):
        """
        Push data to an ESP32 and measure the bandwidth
        :return:
        """
        buff = []
        for i in range(0, SIZE):
            buff.append(b"A")
        while not self.stop_event.is_set():
            timestamp = time.time()
            ret = self.sock.sendall(buff)
            if ret is None:
                print("Speed: {} bytes per second on connection to {}".format(SIZE/(time.time() - timestamp), self.addr))
            else:
                print("Connection error: sock.sendall() returned {} instead of None".format(ret))

    def stop(self):
        """
        Stop the thread
        :return:
        """
        self.stop_event.set()


def main():
    sock = ParentTsock("192.168.1.18")
    sock.listen_sock()


if __name__ == '__main__':
    main()