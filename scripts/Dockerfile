FROM ubuntu:xenial

ENV LANG en_US.UTF-8

RUN export PKGS="clang-5.0 g++ llvm-5.0-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev make flexc++ bisonc++ python3 gdb ne" \
	&& apt-get update && apt-get install -y --no-install-recommends locales \
	&& locale-gen en_US.UTF-8 && update-locale en_US.UTF-8 \
	&& apt-get install -y --no-install-recommends $PKGS \
	&& rm -rf /var/lib/apt/lists/* /var/lib/dpkg/info/*
