FROM ubuntu:xenial

ENV LANG en_US.UTF-8

RUN apt-get update \
	&& apt-get install -y locales clang-5.0 g++ llvm-5.0-dev libboost-program-options-dev libboost-filesystem-dev make flexc++ bisonc++ python3 gdb ne \
	&& locale-gen en_US.UTF-8 && update-locale en_US.UTF-8 \
	&& rm -rf /var/lib/apt/lists/* /var/lib/dpkg/info/*
