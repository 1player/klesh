.EXPORT_ALL_VARIABLES:

# Uncomment this line if you want to enable kernel debugging (bigger and slower kernel)
DEBUG=1

####### DON'T MODIFY HERE BELOW UNLESS YOU KNOW WHAT YOU'RE DOING ###########

# Directories
TOP_DIR = $(shell pwd)
INCLUDE_DIR = "$(TOP_DIR)/Include"

.PHONY: all clean kernel applications install install-kernel install-applications

all: kernel
	
kernel::
	@echo Building kernel...
	@make -C Sources 1>/dev/null
	
install:
	@echo Installing kernel...
	@rm -Rf .tmp-mount
	@mkdir .tmp-mount
	@sudo mount -o loop floppy.img .tmp-mount
	@sudo cp Sources/kernel.bin .tmp-mount/system
	@sudo umount floppy.img
	@rm -Rf .tmp-mount

clean:
	@echo Cleaning up...
	@make -C Sources clean 1>/dev/null
	@rm -Rf bochslog.txt
