FROM ubuntu:xenial

WORKDIR /app

COPY . .

RUN apt-get update
RUN apt-get install -y build-essential

RUN make

EXPOSE 8080/tcp

# Run the server listening on port 8080
CMD ["/app/bin/server","8080"]