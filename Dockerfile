FROM gcc:latest

WORKDIR /home/app/

COPY . .

RUN make
RUN mkdir /out
RUN cp ./server_bin/server /out/server
RUN cp ./client_bin/client /out/client

# This doesn't actually do anything, you must use docker run -p [host]:[container]
EXPOSE 8080/udp

# Run the server listening on port 8080
CMD ["/home/app/server_bin/server","8080"]