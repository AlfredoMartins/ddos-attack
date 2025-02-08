FROM ubuntu:latest

# Install dependencies
RUN apt-get update && apt-get install -y \
    clang \
    g++ \
    libcurl4-openssl-dev \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Run the server
RUN g++ -std=c++17 -Wall -g -c TaskScheduler.cpp -o TaskScheduler.o && \
    g++ -std=c++17 -Wall -g main.cpp TaskScheduler.o -o main -lcurl

CMD ["./main"]
