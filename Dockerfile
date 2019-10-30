# Compile and install qemu_stm32
FROM fedora:latest

RUN dnf install -y \
          arm-none-eabi-gcc\
          arm-none-eabi-newlib\
          findutils\
          gcc\
          git\
          glib2-devel\
          libfdt-devel\
          pixman-devel\
          pkgconf-pkg-config\
          python\
          zlib-devel \
          make \
          g++

WORKDIR /qemu_stm32
ADD . /qemu_stm32
RUN \
  ./configure \
      --extra-cflags="-w" \
      --enable-debug \
      --target-list="arm-softmmu" \
 && make \
 && make install

# Install demos
WORKDIR /
RUN \
    git clone https://github.com/beckus/stm32_p103_demos.git \
 && cd stm32_p103_demos \
 && make
