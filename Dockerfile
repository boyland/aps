FROM alpine:3.17 AS base

LABEL maintainer="boyland@uwm.edu"
LABEL version="1.0.0"

ARG SCALA_VERSION="2.12.13"
ARG BISON_VERSION="3.5.1"
ARG CLASSHOME="/usr/local"

WORKDIR /usr/lib

# Install basics
RUN apk add --no-cache bash make flex gcompat gcc g++ openjdk11 build-base m4 git ncurses \
  && apk add --no-cache --virtual=build-dependencies wget ca-certificates

# Download Scala
RUN wget -q "https://downloads.lightbend.com/scala/${SCALA_VERSION}/scala-${SCALA_VERSION}.tgz" -O - | gunzip | tar x

ENV SCALAHOME="/usr/lib/scala-$SCALA_VERSION"
ENV PATH="$SCALAHOME/bin:$PATH"

# Build and install Bison
RUN wget -q "https://ftp.gnu.org/gnu/bison/bison-${BISON_VERSION}.tar.gz" -O - | gunzip | tar x \
  && cd "bison-${BISON_VERSION}" \
  && ./configure \
  && make \
  && make install

COPY . .

ENV APSHOME="/usr/lib/bin"
ENV PATH="$APSHOME:$PATH"

WORKDIR /usr/local
