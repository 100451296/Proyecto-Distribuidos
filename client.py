import subprocess
import sys
import PySimpleGUI as sg
from enum import Enum
import argparse
import socket
from functions import *
import threading

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _quit = 0
    _username = "edu"
    _alias = "edu"
    _date = "1/1/1"
    _server_addres = None
    _connected = True
    _thread = None
   
    OP_REGISTER = 'REGISTER'
    OP_UNREGISTER = 'UNREGISTER'
    OP_CONNECT = 'CONNECT'
    OP_DISCONNECT = 'DISCONNECT'
    OP_SEND = 'SEND'
    OP_CONNECTEDUSERS = 'CONNECTEDUSERS' 

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def register(user, window):
        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Crea el socket
        connection.connect(client._server_addres) # Conectamos el socket al servidor

        # Generamos petición para mandar 
        message = formatPetition(connection, client.OP_REGISTER, client._username, user, client._date)
      
        # result = connection.recv(1024).decode()
        # print('res' ,result)
        
        # connection.close()

        temp_result = connection.recv(1024)
        print('temp_res:',temp_result)
        connection.close()
        result = temp_result.decode("utf-8")
        print('res:',result)


        # Mensaje de resultado de conexion
        if result == "0":
            window['_SERVER_'].print("s> REGISTER "+ client._username + " OK")
        elif result == "1":
            window['_SERVER_'].print("s> REGISTER "+ client._username + " FAIL")
        else:
            window['_SERVER_'].print("s> REGISTER FAIL CONNECTION res: " + result )
        return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 *
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user, window):
        
        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Crea el socket
        connection.connect(client._server_addres) # Conectamos el socket al servidor

        message = formatPetition(connection, client.OP_UNREGISTER, client._alias)

        result = connection.recv(1024).decode("utf-8")
        print(result)

        connection.close()

        if result == "0":
            window['_SERVER_'].print("s> UNREGISTER "+ client._username + " OK")
        elif result == "1":
            window['_SERVER_'].print("s> UNREGISTER "+ client._username + " FAIL")
        else:
            window['_SERVER_'].print("s> UNREGISTER FAIL CONNECTION")
        return client.RC.ERROR

    @staticmethod
    def handleConnection(conn, addr, window):
        print('Conectado por', addr)
        try:
            op = conn.recv(1024).decode("utf-8")
            conn.sendall("OK\0".encode())
            if not op:
                print("error en op")
                raise "Error en op"
            print("Mensaje:", op)
            
            if op == "SEND_MESSAGE":
                
                alias = conn.recv(1024).decode("utf-8")
                conn.sendall("OK\0".encode())
                if not alias:
                    print("error en alias")
                    
                print("Alias:", alias)

                id = conn.recv(1024).decode("utf-8")
                conn.sendall("OK\0".encode())
                if not id:
                    print("error en id")
                    
                print("id:", id)

                content = conn.recv(1024).decode("utf-8")
                if not content:
                    print("error en content")
                    
                print("Content:", content)
                window['_SERVER_'].print(f"s> MESSAGE {id} FROM {alias} {content} END")

        except Exception as e:
            print("Error:", e)
            
        conn.close()

    @staticmethod
    def recvMessages(socket, window):
        socket.settimeout(10)  # tiempo de espera para recibir conexiones
        while client._connected:
            socket.listen(1)
            print("Estoy")
            try:
                conn, addr = socket.accept()
                t = threading.Thread(target=client.handleConnection, args=(conn, addr, window))
                t.start()
            except Exception as e:
                pass  # seguir esperando conexiones
        print("Salgo")
        socket.close()

    # *
    # * @param user - User name to connect to the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred

    # Crea el hilo 
    @staticmethod
    def connect(user, window):
        socketMessages = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        socketMessages.bind(('localhost', 0)) # 0: que busque puerto disponible
        host, port = socketMessages.getsockname()
        print("host:", host)
        print("port:", port)
        
        if client._thread == None:
            client._connected = True
            client._thread = threading.Thread(target=client.recvMessages, args=(socketMessages, window))
            # target=client.recvMessages xq recvMessages es staticmethod
            client._thread.start()

        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Crea el socket
        connection.connect(client._server_addres) # Conectamos el socket al servidor

        message = formatPetition(connection, client.OP_CONNECT, client._alias, str(port))
        resetBuffer(connection)

        result = connection.recv(1024).decode("utf-8")
        print(result)
        connection.close()

        if result == "0":
            window['_SERVER_'].print("s> CONNECT " + client._username + " OK")
        elif result == "1":
            window['_SERVER_'].print("s> CONNECT FAIL, USER DOES NOT EXIST")
        elif result == "2":
            window['_SERVER_'].print("s> USER ALREADY CONNECTED")
        else:
            window['_SERVER_'].print("s> CONNECT FAIL CONNECTION")

        return client.RC.ERROR

    # *
    # * @param user - User name to disconnect from the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def disconnect(user, window):

        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Crea el socket
        connection.connect(client._server_addres) # Conectamos el socket al servidor

        message = formatPetition(connection,client.OP_DISCONNECT, client._alias)
        #connection.sendall(message.encode("utf-8"))
        resetBuffer(connection)

        result = connection.recv(1024).decode("utf-8")
        print(result)
        connection.close()

        if result == "0":
            window['_SERVER_'].print("s> DISCONNECT " + client._username + " OK")
        elif result == "1":
            window['_SERVER_'].print("s> DISCONNECT FAIL, USER DOES NOT EXIST")
        elif result == "2":
            window['_SERVER_'].print("s> USER NOT CONNECTED")
        else:
            window['_SERVER_'].print("s> DISCONNECT FAIL CONNECTION")

        client._connected = False
        client._thread = None
        
        return client.RC.ERROR
    

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def send(user, message, window):
        # ******* Conexión de cliente con servidor  ******* #

        print("Datos", user, message, "user", type(user))
        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Crea el socket
        connection.connect(client._server_addres) # Conectamos el socket al servidor

        # Generamos petición para mandar 
        message = formatPetition(connection, client.OP_SEND, client._alias, user, message)
        resetBuffer(connection)

        result = connection.recv(1024).decode("utf-8")
        print(result)

        connection.sendall("OK\0".encode())
        id = connection.recv(1024).decode("utf-8")
        
        # ******* Retroalimentación a interfaz  ******* #
        # Todo bien
        if result == '0':
            window['_SERVER_'].print(f"s> SEND MESSAGE {id} OK")
        elif result == '1':
            window['_SERVER_'].print("s> USER " + user + " DOES NOT EXIST")
        elif result == '2':
            window['_SERVER_'].print("s> SEND FAIL")

        connection.close()
        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * @param file    - file  to be sent

    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user, message, file, window):
        window['_SERVER_'].print("s> SENDATTACH MESSAGE OK")
        print("SEND ATTACH " + user + " " + message + " " + file)
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  connectedUsers(window):

        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Crea el socket
        connection.connect(client._server_addres) # Conectamos el socket al servidor

        # Formateamos petición y mandamos a servidor 
        message = formatPetition(connection, client.OP_CONNECTEDUSERS)
        #connection.sendall(message.encode("utf-8"))
        resetBuffer(connection)

        temp_result = connection.recv(1024)
        print('temp_res:',temp_result)
        connection.close()
        result = temp_result.decode("utf-8")
        print('res:',result)

        # Todo bien
        if result == '0':
            window['_SERVER_'].print("s> CONNECTED USERS OK")
        elif result == '1':
            window['_SERVER_'].print("s> USER IS NOT CONNECTED")
        elif result == '2':
            window['_SERVER_'].print("s> CONNECTED USERS FAIL")
        else:
            window['_SERVER_'].print("s> CONNECTED FAIL UNKNOWN res"+ result)
        return client.RC.ERROR


    @staticmethod
    def window_register():
        layout_register = [[sg.Text('Ful Name:'),sg.Input('Text',key='_REGISTERNAME_', do_not_clear=True, expand_x=True)],
                            [sg.Text('Alias:'),sg.Input('Text',key='_REGISTERALIAS_', do_not_clear=True, expand_x=True)],
                            [sg.Text('Date of birth:'),sg.Input('',key='_REGISTERDATE_', do_not_clear=True, expand_x=True, disabled=True, use_readonly_for_disable=False),
                            sg.CalendarButton("Select Date",close_when_date_chosen=True, target="_REGISTERDATE_", format='%d-%m-%Y',size=(10,1))],
                            [sg.Button('SUBMIT', button_color=('white', 'blue'))]
                            ]

        layout = [[sg.Column(layout_register, element_justification='center', expand_x=True, expand_y=True)]]

        window = sg.Window("REGISTER USER", layout, modal=True)
        choice = None

        while True:
            event, values = window.read()

            if (event in (sg.WINDOW_CLOSED, "-ESCAPE-")):
                break

            if event == "SUBMIT":
                if(values['_REGISTERNAME_'] == 'Text' or values['_REGISTERNAME_'] == '' or values['_REGISTERALIAS_'] == 'Text' or values['_REGISTERALIAS_'] == '' or values['_REGISTERDATE_'] == ''):
                    sg.Popup('Registration error', title='Please fill in the fields to register.', button_type=5, auto_close=True, auto_close_duration=1)
                    continue

                client._username = values['_REGISTERNAME_']
                client._alias = values['_REGISTERALIAS_']
                client._date = values['_REGISTERDATE_']
                break
        window.Close()


    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;

        client._server = args.s
        client._port = args.p
        client._server_addres = (client._server, client._port) # Guarda la informacion de IP y puerto

        return True


    def main(argv):

        if (not client.parseArguments(argv)):
            client.usage()
            exit()

        lay_col = [[sg.Button('REGISTER',expand_x=True, expand_y=True),
                sg.Button('UNREGISTER',expand_x=True, expand_y=True),
                sg.Button('CONNECT',expand_x=True, expand_y=True),
                sg.Button('DISCONNECT',expand_x=True, expand_y=True),
                sg.Button('CONNECTED USERS',expand_x=True, expand_y=True)],
                [sg.Text('Dest:'),sg.Input('User',key='_INDEST_', do_not_clear=True, expand_x=True),
                sg.Text('Message:'),sg.Input('Text',key='_IN_', do_not_clear=True, expand_x=True),
                sg.Button('SEND',expand_x=True, expand_y=False)],
                [sg.Text('Attached File:'), sg.In(key='_FILE_', do_not_clear=True, expand_x=True), sg.FileBrowse(),
                sg.Button('SENDATTACH',expand_x=True, expand_y=False)],
                [sg.Multiline(key='_CLIENT_', disabled=True, autoscroll=True, size=(60,15), expand_x=True, expand_y=True),
                sg.Multiline(key='_SERVER_', disabled=True, autoscroll=True, size=(60,15), expand_x=True, expand_y=True)],
                [sg.Button('QUIT', button_color=('white', 'red'))]
            ]


        layout = [[sg.Column(lay_col, element_justification='center', expand_x=True, expand_y=True)]]

        window = sg.Window('Messenger', layout, resizable=True, finalize=True, size=(1000,400))
        window.bind("<Escape>", "-ESCAPE-")


        while True:
            event, values = window.Read()

            if (event in (None, 'QUIT')) or (event in (sg.WINDOW_CLOSED, "-ESCAPE-")):
                sg.Popup('Closing Client APP', title='Closing', button_type=5, auto_close=True, auto_close_duration=1)
                break

            #if (values['_IN_'] == '') and (event != 'REGISTER' and event != 'CONNECTED USERS'):
             #   window['_CLIENT_'].print("c> No text inserted")
             #   continue

            if (client._alias == None or client._username == None or client._alias == 'Text' or client._username == 'Text' or client._date == None) and (event != 'REGISTER'):
                sg.Popup('NOT REGISTERED', title='ERROR', button_type=5, auto_close=True, auto_close_duration=1)
                continue

            if (event == 'REGISTER'):
                client.window_register()

                if (client._alias == None or client._username == None or client._alias == 'Text' or client._username == 'Text' or client._date == None):
                    sg.Popup('NOT REGISTERED', title='ERROR', button_type=5, auto_close=True, auto_close_duration=1)
                    continue

                window['_CLIENT_'].print('c> REGISTER ' + client._alias)
                client.register(client._alias, window)

            elif (event == 'UNREGISTER'):
                window['_CLIENT_'].print('c> UNREGISTER ' + client._alias)
                client.unregister(client._alias, window)


            elif (event == 'CONNECT'):
                window['_CLIENT_'].print('c> CONNECT ' + client._alias)
                client.connect(client._alias, window)


            elif (event == 'DISCONNECT'):
                window['_CLIENT_'].print('c> DISCONNECT ' + client._alias)
                client.disconnect(client._alias, window)


            elif (event == 'SEND'):
                window['_CLIENT_'].print('c> SEND ' + values['_INDEST_'] + " " + values['_IN_'])

                if (values['_INDEST_'] != '' and values['_IN_'] != '' and values['_INDEST_'] != 'User' and values['_IN_'] != 'Text') :
                    client.send(values['_INDEST_'], values['_IN_'], window)
                else :
                    window['_CLIENT_'].print("Syntax error. Insert <destUser> <message>")


            elif (event == 'SENDATTACH'):

                window['_CLIENT_'].print('c> SENDATTACH ' + values['_INDEST_'] + " " + values['_IN_'] + " " + values['_FILE_'])

                if (values['_INDEST_'] != '' and values['_IN_'] != '' and values['_FILE_'] != '') :
                    client.sendAttach(values['_INDEST_'], values['_IN_'], values['_FILE_'], window)
                else :
                    window['_CLIENT_'].print("Syntax error. Insert <destUser> <message> <attachedFile>")


            elif (event == 'CONNECTED USERS'):
                window['_CLIENT_'].print("c> CONNECTEDUSERS")
                client.connectedUsers(window)

            window.Refresh()

        window.Close()


if __name__ == '__main__':
    client.main([])
    print("+++ FINISHED +++")
