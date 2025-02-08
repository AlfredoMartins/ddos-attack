FROM ubuntu:latest

# Install dependencies
RUN apt-get update && apt-get install -y \
    clang \
    libboost-system-dev \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Compile the application
RUN make

# Expose the port
EXPOSE 5000

# Run the server
CMD ["./main"]
