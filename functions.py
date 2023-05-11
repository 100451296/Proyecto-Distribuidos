MAX_LINE_LENGHT = 1024
MAX_MESSAGE = 256
DELIM = ":;"

import socket

def formatPetition(socket, *args):
    for i in args:
        peticion = i + "\0"
        res = socket.sendall(peticion.encode("utf-8"))
        if res  < len(i):
            return -1 # No se ha enviado todo el contenido de i
        elif res == len(i):
            return 0 # Se ha enviado todo el contenido

def readString(sock):
    a = ''
    while True:
        msg = sock.recv(1).decode("utf-8")
        if not msg:
            # Si no se recibió nada, se cerró la conexión
            return None
        a += msg
        if '\0' in msg:
            # Si se recibió el carácter nulo, se terminó de recibir el mensaje
            break
    return a.split('\0')[0]


