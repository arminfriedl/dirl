FROM alpine

RUN adduser web --disabled-password --no-create-home --shell /sbin/nologin && \
    addgroup web web && \
    mkdir -p /var/www/html

COPY dirl /usr/local/bin

ENTRYPOINT ["dirl", "-p", "80", "-u", "web", "-g", "web", "-n", "4096", "-h", "0.0.0.0", "-l", "-d", "/var/www/html"]
