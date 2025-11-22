# Start directly with the Python image
FROM python:3.11-slim-bookworm

# Set environment variables to prevent interactive prompts during install
ENV DEBIAN_FRONTEND=noninteractive

# 1. Install LLVM and system dependencies
# We install wget/lsb-release to use the official script, 
# OR we can just use apt-get if the repo has a new enough version.
# Below uses the official script to ensure you get Version 18.
RUN apt-get update && apt-get install -y \
    wget \
    lsb-release \
    software-properties-common \
    gnupg2 \
    build-essential \
    git \
    cmake \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM 18
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 20

# More installations done later. Doing here to use previous cached layers.
RUN apt-get update && apt-get install -y \ 
    libzstd-dev \
    zlib1g-dev \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# 2. Set Environment variables so Python (llvmlite) finds LLVM
# LLVM installs into /usr/lib/llvm-20/bin, we need to add that to PATH
ENV PATH="/usr/lib/llvm-20/bin:$PATH"
ENV LLVM_CONFIG="/usr/lib/llvm-20/bin/llvm-config"

# 3. Verify installation (Optional, helps debugging)
RUN llvm-config --version

# 4. Python Setup
WORKDIR /app

COPY requirements.txt .
# If you are using llvmlite, it often needs to compile against LLVM, 
# so we need to ensure the environment variables above are set BEFORE this runs.
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

CMD ["/bin/bash"]