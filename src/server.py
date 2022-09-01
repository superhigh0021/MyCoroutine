import socket

host = '127.0.0.1'
port = 9999

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.settimeout(30)
server.bind((host, port))
server.listen(5)
c, addr = server.accept()
print("accept success")

data = server.recv(32);
print(data)
server.send(data)