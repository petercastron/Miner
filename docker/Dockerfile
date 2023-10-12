FROM debian:stretch
MAINTAINER Gordie Jameson <gordie.jameson@gmail.com>
ENTRYPOINT ["/entrypoint.sh"]
ADD ./files.tar.gz /
VOLUME /tmp /data/utgfs
ENV SIGN e2a1037bf46d42faadd3249139cf10b9
EXPOSE 1234 8080
#RUN apt-get update && apt-get install -y net-tools vim iptables procps && apt-get clean && apt-get autoremove
