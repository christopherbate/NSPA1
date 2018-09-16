FROM gcc:latest

WORKDIR /home/app/

COPY . .

RUN make

# This doesn't actually do anything, you must use docker run -p [host]:[container]
EXPOSE 8080/udp

# Run the server listening on port 8080
CMD ["/home/app/bin/server","8080"]