import os
import select

class Fifo:
    def __init__(self, in_path):
        self.in_path = in_path
        self.infd = None

    def connect(self):
        self.infd = os.open(self.in_path, os.O_WRONLY)
        return self

    def send(self, data):
        if isinstance(data, int):
            data = str(data).encode()
        os.write(self.infd, data+b'\n')
