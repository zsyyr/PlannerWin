################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libplanner/mrp-application.c \
../libplanner/mrp-assignment.c \
../libplanner/mrp-calendar.c \
../libplanner/mrp-day.c \
../libplanner/mrp-error.c \
../libplanner/mrp-file-module.c \
../libplanner/mrp-group-assignment.c \
../libplanner/mrp-group.c \
../libplanner/mrp-marshal.c \
../libplanner/mrp-object.c \
../libplanner/mrp-old-xml.c \
../libplanner/mrp-parser.c \
../libplanner/mrp-paths-gnome.c \
../libplanner/mrp-project.c \
../libplanner/mrp-property.c \
../libplanner/mrp-relation.c \
../libplanner/mrp-resource.c \
../libplanner/mrp-sql.c \
../libplanner/mrp-storage-module-factory.c \
../libplanner/mrp-storage-module.c \
../libplanner/mrp-storage-mrproject.c \
../libplanner/mrp-storage-sql.c \
../libplanner/mrp-task-manager.c \
../libplanner/mrp-task.c \
../libplanner/mrp-time.c \
../libplanner/mrp-types.c \
../libplanner/mrp-xml.c \
../libplanner/mrp-xsl.c 

OBJS += \
./libplanner/mrp-application.o \
./libplanner/mrp-assignment.o \
./libplanner/mrp-calendar.o \
./libplanner/mrp-day.o \
./libplanner/mrp-error.o \
./libplanner/mrp-file-module.o \
./libplanner/mrp-group-assignment.o \
./libplanner/mrp-group.o \
./libplanner/mrp-marshal.o \
./libplanner/mrp-object.o \
./libplanner/mrp-old-xml.o \
./libplanner/mrp-parser.o \
./libplanner/mrp-paths-gnome.o \
./libplanner/mrp-project.o \
./libplanner/mrp-property.o \
./libplanner/mrp-relation.o \
./libplanner/mrp-resource.o \
./libplanner/mrp-sql.o \
./libplanner/mrp-storage-module-factory.o \
./libplanner/mrp-storage-module.o \
./libplanner/mrp-storage-mrproject.o \
./libplanner/mrp-storage-sql.o \
./libplanner/mrp-task-manager.o \
./libplanner/mrp-task.o \
./libplanner/mrp-time.o \
./libplanner/mrp-types.o \
./libplanner/mrp-xml.o \
./libplanner/mrp-xsl.o 

C_DEPS += \
./libplanner/mrp-application.d \
./libplanner/mrp-assignment.d \
./libplanner/mrp-calendar.d \
./libplanner/mrp-day.d \
./libplanner/mrp-error.d \
./libplanner/mrp-file-module.d \
./libplanner/mrp-group-assignment.d \
./libplanner/mrp-group.d \
./libplanner/mrp-marshal.d \
./libplanner/mrp-object.d \
./libplanner/mrp-old-xml.d \
./libplanner/mrp-parser.d \
./libplanner/mrp-paths-gnome.d \
./libplanner/mrp-project.d \
./libplanner/mrp-property.d \
./libplanner/mrp-relation.d \
./libplanner/mrp-resource.d \
./libplanner/mrp-sql.d \
./libplanner/mrp-storage-module-factory.d \
./libplanner/mrp-storage-module.d \
./libplanner/mrp-storage-mrproject.d \
./libplanner/mrp-storage-sql.d \
./libplanner/mrp-task-manager.d \
./libplanner/mrp-task.d \
./libplanner/mrp-time.d \
./libplanner/mrp-types.d \
./libplanner/mrp-xml.d \
./libplanner/mrp-xsl.d 


# Each subdirectory must supply rules for building sources it contributes
libplanner/%.o: ../libplanner/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/gtk-2.0 -I/usr/lib/i386-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -I/usr/lib/gtk-2.0/include -I/usr/include/libgnomeui-2.0 -I/usr/include/gnome-vfs-2.0 -I/usr/include/libxml2 -I/usr/include -I/usr/include/gtk-unix-print-2.0 -I/usr/include/libglade-2.0 -I/usr/include/libgnomecanvas-2.0 -I/usr/include/glib-2.0 -O0 -g3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk+-2.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


