make
docker build -t cpp_server .
docker run -p 5000:5000 cpp_server
