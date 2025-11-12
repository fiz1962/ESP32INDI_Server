import socket
import time

HOST = '' # Listen on all interfaces
PORT = 7624
FILENAME = "data.txt"

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen(1)
        print(f"Waiting for connection on port {PORT}...")
        
        conn, addr = s.accept()
        print(f"Connected by {addr}")

        with conn, open(FILENAME, "r") as f:
            for line in f:
                # Ensure the line ends with newline
                data = line.rstrip("\n") + "\n"
                conn.sendall(data.encode('utf-8'))
                time.sleep(0.02) # 20 ms

        print("Finished sending file. Closing connection.")

if __name__ == "__main__":
    main()