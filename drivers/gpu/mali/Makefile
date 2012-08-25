MALI_GIT := $(srctree)/$(src)/mali400ko
MALI_SUBFOLDER := mali400ko/driver/src/devicedrv/mali

ifeq ($(shell [ -d $(MALI_GIT) ] && echo "OK"), OK)
obj-$(CONFIG_GPU_MALI)  += $(MALI_SUBFOLDER)/
else
$(error Cannot find $(MALI_GIT) which is a git in the kernel tree. Clone it to build the kernel)
endif
