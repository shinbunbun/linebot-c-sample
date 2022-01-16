FROM frolvlad/alpine-glibc:latest
RUN apk add --no-cache gcc libc-dev make openssl3 git libpcap-dev libressl-dev && \
    wget https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz && \
    tar -zxvf jansson-2.14.tar.gz && \
    cd jansson-2.14 && ./configure && \
    make && \
    make install
COPY . /app
WORKDIR /app
RUN make build
CMD ["make", "run"]
