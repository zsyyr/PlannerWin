################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tests/calendar-test.c \
../tests/cmd-manager-test.c \
../tests/scheduler-test.c \
../tests/self-check.c \
../tests/task-test.c \
../tests/time-test.c 

OBJS += \
./tests/calendar-test.o \
./tests/cmd-manager-test.o \
./tests/scheduler-test.o \
./tests/self-check.o \
./tests/task-test.o \
./tests/time-test.o 

C_DEPS += \
./tests/calendar-test.d \
./tests/cmd-manager-test.d \
./tests/scheduler-test.d \
./tests/self-check.d \
./tests/task-test.d \
./tests/time-test.d 


# Each subdirectory must supply rules for building sources it contributes
tests/%.o: ../tests/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/gtk-2.0 -I/usr/lib/i386-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -I/usr/lib/gtk-2.0/include -I/usr/include/libgnomeui-2.0 -I/usr/include/gnome-vfs-2.0 -I/usr/include/libxml2 -I/usr/include -I/usr/include/gtk-unix-print-2.0 -I/usr/include/libglade-2.0 -I/usr/include/libgnomecanvas-2.0 -I/usr/include/glib-2.0 -O0 -g3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


