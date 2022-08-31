import socket

host = '127.0.0.1'
port = 8888

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.settimeout(30)
client.connect((host, port));
print("connect success")

client.send(b"hello")
data = client.recv(32);
print(data)