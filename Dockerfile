FROM ubuntu:20.04 as build

WORKDIR /app

ENV DEBIAN_FRONTEND=noninteractive  

RUN apt update \
&& apt upgrade -y \
&& apt install libboost-all-dev build-essential -y

COPY . /app

RUN make

FROM ubuntu:20.04 as run

RUN apt update && apt install libgomp1 -y

COPY --from=build /app/exe /usr/bin/run

ENTRYPOINT ["run"]