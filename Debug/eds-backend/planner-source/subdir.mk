################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../eds-backend/planner-source/planner-source.c 

OBJS += \
./eds-backend/planner-source/planner-source.o 

C_DEPS += \
./eds-backend/planner-source/planner-source.d 


# Each subdirectory must supply rules for building sources it contributes
eds-backend/planner-source/%.o: ../eds-backend/planner-source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/gtk-2.0 -I/usr/lib/i386-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -I/usr/lib/gtk-2.0/include -I/usr/include/libgnomeui-2.0 -I/usr/include/gnome-vfs-2.0 -I/usr/include/libxml2 -I/usr/include -I/usr/include/gtk-unix-print-2.0 -I/usr/include/libglade-2.0 -I/usr/include/libgnomecanvas-2.0 -I/usr/include/glib-2.0 -O0 -g3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


