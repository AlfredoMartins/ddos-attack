FROM ubuntu:latest

# Install dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    libboost-system-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Expose the port
EXPOSE 5001

# Run the server
RUN g++ -std=c++17 -Wall -g -c main.cpp -o main.o && \
    g++ -std=c++17 -Wall -g main.o -o main -pthread

CMD ["./main"]