FROM debian:10

RUN apt-get update && \
    apt-get install -y ruby ruby-dev rubygems build-essential git rpm sudo curl jq

RUN gem install --no-document fpm -v 1.12.0

VOLUME /repo
WORKDIR /repo

ENV PACKAGE="deb"
ENV VERSION=""
ENV ARCH="amd64"
ENV OUTPUT_DIR="/repo/dist/"

CMD /bin/bash
