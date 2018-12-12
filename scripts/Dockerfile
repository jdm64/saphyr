FROM ubuntu:trusty

ENV LANG en_US.UTF-8

RUN locale-gen en_US.UTF-8 && update-locale en_US.UTF-8 \
	&& echo "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-5.0 main" > /etc/apt/sources.list.d/llvm.list \
	&& apt-key adv --fetch-keys http://apt.llvm.org/llvm-snapshot.gpg.key \
	&& apt-get update \
	&& apt-get install -y clang-5.0 g++ llvm-5.0-dev libboost-program-options-dev libboost-filesystem-dev make flexc++ bisonc++ python3 gdb ne \
	&& rm -rf /var/lib/apt/lists/* /var/lib/dpkg/info/*
