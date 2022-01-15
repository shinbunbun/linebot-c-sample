FROM frolvlad/alpine-glibc:latest
# ENV LANG=C.UTF-8
# RUN wget -q -O /etc/apk/keys/sgerrand.rsa.pub https://alpine-pkgs.sgerrand.com/sgerrand.rsa.pub && wget https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.34-r0/glibc-2.34-r0.apk
# RUN apk add --no-cache gcc glibc-2.34-r0.apk make openssl3 git libpcap-dev libressl-dev
RUN apk add --no-cache gcc libc-dev make openssl3 git libpcap-dev libressl-dev && \
    # wget https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.34-r0/glibc-2.34-r0.apk https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.34-r0/glibc-bin-2.34-r0.apk https://github.com/sgerrand/alpine-pkg-glibc/releases/download/2.34-r0/glibc-i18n-2.34-r0.apk && \
    # apk add --no-cache --allow-untrusted glibc-2.34-r0.apk glibc-bin-2.34-r0.apk glibc-i18n-2.34-r0.apk &&  \
    # (/usr/glibc-compat/bin/localedef --force --inputfile POSIX --charmap UTF-8 C.UTF-8 || true) && \
    # echo "export LANG=$LANG" > /etc/profile.d/locale.sh && \
    # gcc version && \
    wget https://github.com/akheron/jansson/releases/download/v2.14/jansson-2.14.tar.gz && \
    tar -zxvf jansson-2.14.tar.gz && \
    cd jansson-2.14 && ./configure && \
    make && \
    make install
COPY . /app
WORKDIR /app
RUN make build
CMD ["make", "run"]
