FROM debian:buster-slim
LABEL maintainer "Hiroki Kumazaki <kumagi@google.com>"

RUN apt-get update && \
    apt-get install -y --no-install-recommends -q \
    ca-certificates \
    wget \
    unzip \
    perl \
    cmake \
    make \
    clang \
    gcc \
    libssl-dev \
    git && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /

ADD . /libsxg
RUN mkdir libsxg/docker_build -p && \
    cd libsxg/docker_build && \
    cmake .. && \
    make sxg

CMD "/bin/bash"
