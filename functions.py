MAX_LINE_LENGHT = 1024
MAX_MESSAGE = 256
DELIM = ":;"

import socket
import time

def formatPetition(socket, *args):
    for i in range(len(args)):
        peticion = args[i] + '\0'
        #print('pet' , peticion)
        res = socket.sendall(peticion.encode('utf-8'))
        confirmacion = socket.recv(1024).decode('utf-8')
        #print("Recibido", confirmacion)
    
    return confirmacion # Se ha enviado todo el contenido

def readString(sock):
    a = ''
    while True:
        msg = sock.recv(1).decode('utf-8')
        print(msg)
        if not msg:
            # Si no se recibió nada, se cerró la conexión
            return None
        a += msg
        if '\0' in msg:
            # Si se recibió el carácter nulo, se terminó de recibir el mensaje
            a = a.split('\0')[0]
            break
        elif '\n' in msg:
            a = a.split('\n')[0]
    return a

def resetBuffer(connection):
    connection.settimeout(100) # Establece un tiempo de espera de 5 segundos
    try:
        while True:
            data = connection.recv(1)
            
            if data == b'\n':
                #print("Reset buffer", data)
                break
            if not data:
                break

    except Exception as e:
        pass # No hay más datos disponibles
