MAX_LINE_LENGHT = 1024

def formatPetition(*args):
    petition = ":;".join(args)
    petition += "\0"
    return petition

def readString(sock):
    a = ''
    while True:
        print("leyendo")
        msg = sock.recv(1).decode("utf-8")
        if not msg:
            # Si no se recibió nada, se cerró la conexión
            print("cerre conexion")
            return None
        a += msg
        if '\0' in msg:
            # Si se recibió el carácter nulo, se terminó de recibir el mensaje
            break
    return a.split('\0')[0]
