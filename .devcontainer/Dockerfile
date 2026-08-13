FROM debian:13

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    # ARM bare-metal toolchain
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    \
    # Build tools
    build-essential \
    cmake \
    make \
    \
    # General dev utilities
    git \
    curl \
    wget \
    ca-certificates \
    xxd \
    file \
    patch \
    unzip \
    zip \
    jq \
    less \
    nano \
    bash-completion \
    socat \
	picocom \
	nodejs \
	npm \
    \
    # Dev-container niceties
    sudo \
    openssh-client \
	&& npm install -g @vscode/vsce \
    && rm -rf /var/lib/apt/lists/*

# Non-root user suitable for VS Code/devcontainers
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd \
        --uid ${USER_UID} \
        --gid ${USER_GID} \
        --create-home \
        --shell /bin/bash \
        ${USERNAME} \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" \
        > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}

WORKDIR /workspaces/flasher

COPY misc/flasher ./

RUN mkdir -p build && cd build && cmake .. && make

WORKDIR /workspaces/vscode-extension

COPY .devcontainer/vscode-extension/ ./

RUN npx @vscode/vsce package -o /workspaces/vscode-extension/kernel-tools.vsix --allow-missing-repository

WORKDIR /workspaces/KIV-RTOS

ENV PATH="/workspaces/flasher/build:${PATH}"

CMD ["/bin/bash"]