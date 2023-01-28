docker build -t server:base -f Dockerfile_base .

# docker run
# docker run -ti --privileged --rm -v /home/wenjie.yin/github/simplest-web-server:/opt/simplest-web-server server:base bash