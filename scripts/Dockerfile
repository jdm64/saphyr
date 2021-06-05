FROM ubuntu:bionic

ENV LANG en_US.UTF-8

RUN export PKGS="clang-10 g++ llvm-10-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev make flexc++ bisonc++ python3 gdb ne" \
	&& apt-get update && apt-get install -y --no-install-recommends locales \
	&& echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && locale-gen \
	&& apt-get install -y --no-install-recommends $PKGS \
	&& rm -rf /var/lib/apt/lists/* /var/lib/dpkg/info/*
