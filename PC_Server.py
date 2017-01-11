# -*- coding: utf-8 -*-
"""
Created on Sat Nov 26 17:11:31 2016

@author: nbadami
"""

#Server
import socket

def Main():
    host = ''               #must enter the host IP
    serverport = 5003       #choosing port 5003

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((host, serverport))
    message = 'a'

    while True:
        data, addr = s.recvfrom(1048576)        #lazily recieve data from a client
        if not data:
            break
        print("from connected user: " + str(data))
        data = str(data).upper()
        print("sending: " + str(data))
        s.sendto(str.encode(data), (str(addr[0]), addr[1]))
        break
    
    while message != 'q':                       #kill server with 'q'
        message = input("-> ")
        print("sending: " + str(message))
        s.sendto(str.encode(message), (str(addr[0]), addr[1]))
        
    s.close()

if __name__ == '__main__':
    Main()