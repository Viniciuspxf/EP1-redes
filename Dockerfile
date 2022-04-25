FROM gcc as base

COPY . ./

RUN make clean && make

FROM ubuntu

WORKDIR /usr/src/app

ENV PORT=8000

COPY --from=base ./server ./

RUN chmod 777 ./server

EXPOSE $PORT

CMD ./server ${PORT}
