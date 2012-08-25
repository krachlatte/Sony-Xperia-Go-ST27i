#Android makefile to build kernel as a part of Android Build
ifeq ($(TARGET_USE_ST_ERICSSON_KERNEL),true)
ifneq ($(SEMC_PLATFORM),)

CREATE_SIN_HEADER = $(HOST_OUT_EXECUTABLES)/create_sin_header
MKELF             = $(HOST_OUT_EXECUTABLES)/mkelf.py
SEMCSC            = $(HOST_OUT_EXECUTABLES)/semcsc.py

export STERICCSON_WLAN_BUILT_IN=y

ifeq ($(TARGET_PREBUILT_KERNEL),)

# Give other modules a nice, symbolic name to use as a dependent
# Yes, there are modules that cannot build unless the kernel has
# been built. Typical (only?) example: loadable kernel modules.
.phony: build-kernel clean-kernel

PRIVATE_KERNEL_ARGS := -C kernel ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) O=$(KERNEL_OUTPUT)
PRIVATE_OUT := $(abspath $(PRODUCT_OUT)/system)

KERNEL_SOURCE_PATH := $(call my-dir)
KERNEL_SOURCE_CONFIG := $(KERNEL_SOURCE_PATH)/arch/arm/configs/$(KERNEL_DEFCONFIG)
KERNEL_OUT_CONFIG := $(KERNEL_OUTPUT)/.config
TARGET_PREBUILT_KERNEL := $(KERNEL_OUTPUT)/arch/arm/boot/zImage

# Select choosen defconfig
install_defconfig_always:
$(KERNEL_OUT_CONFIG): install_defconfig_always
	@mkdir -p $(KERNEL_OUTPUT)
	$(hide) $(MAKE) -C $(KERNEL_SOURCE_PATH) O=$(KERNEL_OUTPUT)\
            ARCH=arm CROSS_COMPILE=arm-eabi- $(KERNEL_DEFCONFIG)

ifeq ($(TARGET_BUILD_VARIANT),eng)
	kernel/scripts/config --file $(KERNEL_OUTPUT)/.config \
	--enable CONFIG_CW1200_ITP
else
	kernel/scripts/config --file $(KERNEL_OUTPUT)/.config \
	--disable CONFIG_CW1200_ITP
endif

# Build kernel from selected defconfig
$(TARGET_PREBUILT_KERNEL): $(KERNEL_OUT_CONFIG)
	$(hide) $(MAKE) -C $(KERNEL_SOURCE_PATH) O=$(KERNEL_OUTPUT) \
                    ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE)
	$(hide) $(MAKE) -C $(KERNEL_SOURCE_PATH) O=$(KERNEL_OUTPUT) \
                    ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(hide) $(MAKE) -C $(KERNEL_SOURCE_PATH) O=$(KERNEL_OUTPUT) \
                    ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) \
                    INSTALL_MOD_PATH:=$(PRIVATE_OUT) modules_install

# Install selected defconfig, run menuconfig and then copy back
export KCONFIG_NOTIMESTAMP=true
.PHONY: kernelconfig
kernelconfig: $(KERNEL_OUT_CONFIG)
	$(hide) $(MAKE) -C $(KERNEL_SOURCE_PATH)  O=$(KERNEL_OUTPUT) \
	             ARCH=arm CROSS_COMPILE=arm-eabi- menuconfig
	$(hide) cp $(KERNEL_OUT_CONFIG) $(KERNEL_SOURCE_CONFIG)
endif

#
# Rules for packing kernel into elf and sin
#
$(PRODUCT_OUT)/kernel.elf: $(TARGET_PREBUILT_KERNEL) $(PRODUCT_OUT)/ramdisk.img | sin-tools
	$(MKELF) -o $(dir $@)/kernel-unsigned.elf \
	$(TARGET_PREBUILT_KERNEL)@$(BOARD_KERNEL_ADDR) \
	$(PRODUCT_OUT)/ramdisk.img@$(BOARD_RAMDISK_ADDR),ramdisk && \
	$(SEMCSC) -c $(PRODUCT_PARTITION_CONFIG) -p Kernel -t internal \
	 -i $(dir $@)/kernel-unsigned.elf -o $@


$(PRODUCT_OUT)/kernel.si_: $(PRODUCT_OUT)/kernel.elf $(PRODUCT_PARTITION_CONFIG) | sin-tools
	@echo target S1: $(notdir $@)
	$(hide) $(CREATE_SIN_HEADER) Kernel $(PRODUCT_PARTITION_CONFIG) $@
	$(hide) cat $< >> $@

$(PRODUCT_OUT)/kernel.sin: $(PRODUCT_OUT)/kernel.si_ $(PRODUCT_PARTITION_CONFIG) | sin-tools
	@echo target SIN: $(notdir $@)
	$(hide) $(SEMCSC) -c $(PRODUCT_PARTITION_CONFIG) -p Kernel -t external -i $< -o $@

# Configures and runs menuconfig on the kernel based on
# KERNEL_DEFCONFIG given on commandline or in BoardConfig.mk.
# The build after running menuconfig must be run with
# KERNEL_DEFCONFIG=local to not override the configuration modification done.

menuconfig-kernel:
	$(MAKE) $(PRIVATE_KERNEL_ARGS) $(KERNEL_DEFCONFIG)
	$(MAKE) $(PRIVATE_KERNEL_ARGS) menuconfig

# An Android clean removes the files built for the current HW configuration,
# such as u8500,
# while a clobber removes all built files (rm -rf $(OUT_DIR)).
# The kernel only has one build tree, so clean and clobber will be
# the same.
# Android's installclean is used when switching between different build
# variants in the same HW configuration. One of the directories removed
# is $(PRODUCT_OUT)/root - which is where Linux installs its modules.
# Thus, no modifications to the installclean targets are needed here.

#
# Add kernel to system wide PHONY target sin and s1images
#
.PHONY: sin
.PHONY: s1images

sin: $(PRODUCT_OUT)/kernel.sin
s1images: $(PRODUCT_OUT)/kernel.si_

#
# boot.img requires $(INSTALLED_KERNEL_TARGET) to exist
#
$(INSTALLED_KERNEL_TARGET): $(TARGET_PREBUILT_KERNEL) | $(ACP)
	$(transform-prebuilt-to-target)

# Give other modules a nice, symbolic name to use as a dependent
# Yes, there are modules that cannot build unless the kernel has
# been built. Typical (only?) example: loadable kernel modules.
.PHONY: build-kernel
build-kernel: $(TARGET_PREBUILT_KERNEL)

else

#Android makefile to build kernel as a part of Android Build


# Give other modules a nice, symbolic name to use as a dependent
# Yes, there are modules that cannot build unless the kernel has
# been built. Typical (only?) example: loadable kernel modules.
.phony: build-kernel clean-kernel

PRIVATE_KERNEL_ARGS := -C kernel ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) LOCALVERSION=+

PRIVATE_OUT := $(abspath $(PRODUCT_OUT)/system)

PATH := $(PATH):$(BOOT_PATH)/u-boot/tools:$(abspath $(UBOOT_OUTPUT)/tools)
export PATH

# only do this if we are buidling out of tree
ifneq ($(KERNEL_OUTPUT),)
ifneq ($(KERNEL_OUTPUT), $(abspath $(TOP)/kernel))
PRIVATE_KERNEL_ARGS += O=$(KERNEL_OUTPUT)
endif
else
KERNEL_OUTPUT := $(call my-dir)
endif

# Include kernel in the Android build system
include $(CLEAR_VARS)

KERNEL_LIBPATH := $(KERNEL_OUTPUT)/arch/arm/boot
LOCAL_PATH := $(KERNEL_LIBPATH)
ifeq ($(KERNEL_PRODUCT_NAME),riogrande_pdp)
ifeq ($(PDP_STE_FLASH),true)
LOCAL_SRC_FILES := uImage
else
LOCAL_SRC_FILES := zImage
endif
else
LOCAL_SRC_FILES := uImage
endif
LOCAL_MODULE := $(LOCAL_SRC_FILES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

$(KERNEL_LIBPATH)/$(LOCAL_SRC_FILES): build-kernel

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

KERNEL_LIBPATH := $(KERNEL_OUTPUT)
LOCAL_PATH := $(KERNEL_LIBPATH)
LOCAL_SRC_FILES := vmlinux
LOCAL_MODULE := $(LOCAL_SRC_FILES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

$(KERNEL_LIBPATH)/$(LOCAL_SRC_FILES): build-kernel

include $(BUILD_PREBUILT)

# Configures, builds and installs the kernel. KERNEL_DEFCONFIG usually
# comes from the BoardConfig.mk file, but can be overridden on the
# command line or by an environment variable.
# If KERNEL_DEFCONFIG is set to 'local', configuration is skipped.
# This is useful if you want to play with your own, custom configuration.

build-prep:
ifeq ($(KERNEL_DEFCONFIG),local)
	@echo Skipping kernel configuration, KERNEL_DEFCONFIG set to local
else
	$(MAKE) $(PRIVATE_KERNEL_ARGS) $(KERNEL_DEFCONFIG)
endif

ifeq ($(ONE_SHOT_MAKEFILE),)
ifeq ($(KERNEL_PRODUCT_NAME),riogrande_pdp)
ifeq ($(PDP_STE_FLASH),true)
build-kernel: $(UBOOT_OUTPUT)/tools/mkimage build-prep
else
build-kernel: build-prep
endif
else
build-kernel: build-uboot build-prep
endif
else
build-kernel: build-prep
endif

# only do this if we are buidling out of tree
ifneq ($(KERNEL_OUTPUT),)
ifneq ($(KERNEL_OUTPUT), $(abspath $(TOP)/kernel))
	@mkdir -p $(KERNEL_OUTPUT)
endif
endif

ifeq ($(KERNEL_DEFCONFIG),local)
	@echo Skipping kernel configuration, KERNEL_DEFCONFIG set to local
else
	$(MAKE) $(PRIVATE_KERNEL_ARGS) $(KERNEL_DEFCONFIG)
endif

ifeq ($(TARGET_PRODUCT),riogrande_pdp)
ifeq ($(OLD_PDP_RESET),true)
	kernel/scripts/config --file $(KERNEL_OUTPUT)/.config \
		--enable CONFIG_MCDE_DISPLAY_OLD_PDP_RESET
	@rm -f $(KERNEL_OUTPUT)/arch/arm/mach-ux500/board-rio-grande-mcde.o
else
	kernel/scripts/config --file $(KERNEL_OUTPUT)/.config \
		--disable CONFIG_MCDE_DISPLAY_OLD_PDP_RESET
	@rm -f $(KERNEL_OUTPUT)/arch/arm/mach-ux500/board-rio-grande-mcde.o
endif
endif

ifeq ($(TARGET_PRODUCT),riogrande_pdp)
ifeq ($(PDP_STE_FLASH),true)
	$(MAKE) $(PRIVATE_KERNEL_ARGS) uImage
else
	$(MAKE) $(PRIVATE_KERNEL_ARGS) zImage
endif
else
	$(MAKE) $(PRIVATE_KERNEL_ARGS) uImage
endif
ifeq ($(KERNEL_NO_MODULES),)
	$(MAKE) $(PRIVATE_KERNEL_ARGS) modules
	$(MAKE) $(PRIVATE_KERNEL_ARGS) INSTALL_MOD_PATH:=$(PRIVATE_OUT) modules_install
else
	@echo Skipping building of kernel modules, KERNEL_NO_MODULES set
endif

# Configures and runs menuconfig on the kernel based on
# KERNEL_DEFCONFIG given on commandline or in BoardConfig.mk.
# The build after running menuconfig must be run with
# KERNEL_DEFCONFIG=local to not override the configuration modification done.

menuconfig-kernel:
# only do this if we are buidling out of tree
ifneq ($(KERNEL_OUTPUT),)
ifneq ($(KERNEL_OUTPUT), $(abspath $(TOP)/kernel))
	@mkdir -p $(KERNEL_OUTPUT)
endif
endif

	$(MAKE) $(PRIVATE_KERNEL_ARGS) $(KERNEL_DEFCONFIG)
	$(MAKE) $(PRIVATE_KERNEL_ARGS) menuconfig

clean clobber : clean-kernel

clean-kernel:
	$(MAKE) $(PRIVATE_KERNEL_ARGS) clean

endif

endif
