#Webserv
## Description
This project is a simple web server that can handle multiple clients at the same time. It is able to serve static files and dynamic files. The server is able to handle GET, POST, and DELETE requests

## Features
- [x] GET requests
- [x] POST requests
- [x] DELETE requests
- [x] Multiple clients
- [x] Static files
- [x] File upload
- [x] Non-blocking I/O
- [x] CGI scripts
- [x] Error handling

## Installation
1. Clone the repository
```bash
git clone
```
2. Change directory to the project
```bash
cd webserv
```
3. Run the make command
```bash
make
```

## Usage
1. Create a and edit a configuration file
```bash
touch config.conf && nano config.conf
```
2. Add the following configuration
```bash
server {
    listen 8080;
    server_name localhost;
    location / {
        root /path/to/your/static/files;
    }
    location /upload {
        root /path/to/your/uploaded/files;
        upload on;
    }
    location /cgi-bin {
        cgi on;
    }
}
```
3. Run the server
```bash
./webserv config.conf
```
4. Open a web browser and go to `http://localhost:8080`


