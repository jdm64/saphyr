FROM ubuntu:xenial

ARG PKGS="locales clang-5.0 g++ llvm-5.0-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev make flexc++ bisonc++ python3 gdb ne"

ENV LANG en_US.UTF-8

RUN apt-get update \
	&& apt-get install -y $PKGS \
	&& apt-get autoremove -y python2.7 libicu55 \
	&& locale-gen en_US.UTF-8 && update-locale en_US.UTF-8 \
	&& rm -rf /var/lib/apt/lists/* /var/lib/dpkg/info/*
