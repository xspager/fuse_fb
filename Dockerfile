FROM ubuntu:23.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential libfuse-dev libsdl2-dev

WORKDIR /root/code
