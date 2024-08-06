FROM boylanduwm/cool-compiler:latest AS aps-build

WORKDIR /usr/stage/aps

COPY . .

RUN apk add --no-cache flex
RUN make && \
    make install && \
    cd base/scala && \
      make aps-library.jar && \
      make install
RUN cp /usr/stage/aps/bin/* /usr/local/bin && \
    cp /usr/stage/aps/lib/* /usr/local/lib

WORKDIR /root
