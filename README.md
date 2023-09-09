<p align="center">
  <img width="160" height="160" src="https://github.com/xxMichalPK/AmberOS/blob/dev/branding/logo.svg">
</p>

<h1 align="center">AmberOS</h1>

AmberOS is a project aimed at developing a fully functional x86_64 Operating System. The primary objectives of this project are to expand my knowledge of OS development and gain a deep understanding of the fundamental principles underlying the interaction between software and hardware components. The core components of "AmberOS," including the kernel and bootloader, are written in a combination of C, C++ and Assembly Language, allowing me to delve into these languages' intricacies.

## Table of Contents ##

- ### [Introduction](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#introduction-1) ###
- ### [Features](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#features-1) ###
    - [Bootloader](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#bootloader-1)
    - [Kernel](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#kernel-1)
- ### [ToDo List](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#todo-list-1) ###
- ### [Building](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#building-1) ###
- ### [License](https://github.com/xxMichalPK/AmberOS/edit/dev/README.md#license-1) ###

## Introduction ##

AmberOS is an ambitious project that explores the intricacies of operating system development on the x86_64 architecture. With a focus on both legacy and UEFI booting, this OS project aims to encompass a wide range of components, from bootloader development to kernel features and eventually user applications.

## Features ##

### Bootloader ###

The bootloader is responsible for initializing the system, loading the kernel, and transfering control to the kernel. AmberOS, with it's "Amber Loader", supports both legacy and UEFI booting.

### Kernel ###

The kernel is the heart of AmberOS, responsible for core operating system functionalities.

## ToDo List ##

- Memory management
- Interrupt handling
- Syscalls
- Multitasking
- Shared libraries support
- Libc implementation
- PS2 keyboard driver
- PCI driver
- ATA and AHCI drivers
- Filesystem driver
- USB driver
- Mouse driver
- Installer
- GUI
- Applications

## Building ##

AmberOS contains a ``configure`` script that automatically installs all the necessary dependencies:

```
sudo chmod 777 configure
sudo ./configure
```

However, the script was only tested on ``Debian`` and partially with the ``pacman`` package manager and it may not work properly with different package managers. In that case you need to manually install the following build dependencies:

```
make nasm gcc g++ gcc-multilib g++-multilib binutils mtools dosfstools e2fsprogs e2tools xorriso gnu-efi
```

(optional)

```
qemu-system bochs
```

After all the dependencies had been installed you can start the build process by running:

```
make ISO
```

(or)

```
make run_(legacy|UEFI)
```

to run AmberOS, in qemu, after the build.

## License ##

```
AmberOS Download and Sharing License

This License grants the following permissions:

1. Download and Sharing: You are granted the right to download and share the unmodified
source code covered by this License. This includes downloading the source code
for personal use and sharing it with others.

You are explicitly NOT permitted to:

1. Modification: You may not modify, adapt, or create derivative works
based on the source code covered by this License, without the original author's explicit,
written approval. Any attempt at unauthorized modification of the code is strictly prohibited.

2. Use in Other Projects: You may not use, incorporate, or integrate this source code,
in whole or in part, into any other software projects, applications, or systems,
without the original author's explicit, written approval. It is strictly forbidden
to use this source code as a component or foundation for other projects,
whether for personal, academic, or commercial purposes, without prior authorization.

Ownership:
All rights, titles, and interests in the source code, including all intellectual property rights,
are and will remain the exclusive property of the original author(s).
This License does not transfer any ownership rights.

No Warranty:
The source code is provided "as is," without any warranties, express or implied,
including but not limited to the warranties of merchantability, fitness for a particular purpose,
and non-infringement. The original author(s) make no representations or warranties
regarding the accuracy or suitability of the code.

Limitation of Liability:
In no event shall the original author(s) be liable for any claim, damages, or other liability,
whether in an action of contract, tort, or otherwise, arising from, out of,
or in connection with the source code or its use. You assume all risks associated with using the code.

Termination:
This License is effective until terminated by the original author(s).
If you fail to comply with any of its terms and conditions, your rights under this License
will be automatically and immediately terminated without notice.

Modifications and Use in Other Projects:
You may request permission from the original author(s) to modify or use the source code
covered by this License in other projects. The original author(s) reserve the right to grant
or deny such requests at their discretion.

By downloading or sharing the source code covered by this License,
you explicitly agree to be bound by the terms and conditions of this License.


Michał Pazurek
michal10.05.pk@gmail.com
09/09/2023
```