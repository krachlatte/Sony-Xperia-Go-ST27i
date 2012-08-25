#
# Makefile for the linux kernel, U8500 machine.
#

ifeq ($(CONFIG_CRYPTO_DEV_UX500_HASH), m)
	CFLAGS_devices.o += -DCONFIG_CRYPTO_DEV_UX500_HASH
	CFLAGS_board-mop500.o += -DCONFIG_CRYPTO_DEV_UX500_HASH
endif

obj-y					:= clock.o timer-mtu.o timer-db8500-prcmu.o \
					   devices-common.o pins.o dcache.o reboot_reasons.o \
					   uart-db8500.o
obj-$(CONFIG_PM)			+= pm/
obj-$(CONFIG_REGULATOR)			+= regulator-ux500.o

ifeq ($(CONFIG_UX500_SOC_DB8500), y)
obj-$(CONFIG_REGULATOR)			+= regulator-u8500.o
obj-$(CONFIG_MFD_CG2900)		+= devices-cg2900-u8500.o
endif

ifeq ($(CONFIG_UX500_SOC_DB5500), y)
obj-$(CONFIG_REGULATOR)			+= regulator-u5500.o
obj-$(CONFIG_MFD_CG2900)		+= devices-cg2900-u5500.o
endif

obj-$(CONFIG_U8500_REGULATOR_DEBUG)	+= virt-regulator-u8500.o
obj-$(CONFIG_ARCH_U8500)		+= devices.o cpu.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o
obj-$(CONFIG_MACH_B5500)		+= board-u5500.o board-u5500-mcde.o \
					   board-u5500-sdi.o \
					   board-u5500-regulators.o \
					   board-u5500-pins.o \
					   board-u5500-cg2900.o
obj-$(CONFIG_MACH_SVP5500)		+= board-svp5500.o
obj-$(CONFIG_MACH_U8500_MOP)		+= board-mop500.o board-mop500-msp.o board-mop500-uib.o \
					   board-mop500-stuib.o board-mop500-u8500uib.o \
					   board-mop500-pins.o board-mop500-bm.o \
					   board-mop500-regulators.o board-pins-sleep-force.o \
					   board-mop500-wlan.o board-mop500-vibra.o board-mop500-mmio.o \
					   board-mop500-cyttsp.o
obj-$(CONFIG_MACH_U8500_PDP)		+= board-rio-grande.o board-mop500-msp.o \
                                           board-rio-grande-mcde.o \
                                           gpio-pdp.o input-rio-grande-pdp.o board-mop500-bm.o \
                                           board-rio-grande-mmio.o charger-rio-grande.o \
                                           leds-rio-grande-pdp.o board-mop500-regulators.o \
					   board-mop500-wlan.o board-pins-sleep-force.o board-mop500-vibra.o \
					   display-rio-grande.o  simple_remote-rio-grande.o \
					   gpio_keypad-rio-grande.o

obj-$(CONFIG_MACH_U8500_PDP_R2)		+= board-rio-grande.o board-mop500-msp.o \
					   board-rio-grande-mcde.o board-rio-grande-sdi.o \
					   gpio-pdp_r2.o input-rio-grande-pdp.o board-mop500-bm.o \
					   board-rio-grande-mmio.o charger-rio-grande.o \
					   leds-rio-grande-pdp.o board-mop500-regulators.o \
					   board-mop500-wlan.o board-pins-sleep-force.o board-mop500-vibra.o \
					   display-rio-grande_r2.o simple_remote-rio-grande_r2.o

obj-$(CONFIG_MACH_U8500_NYPON)		+= board-rio-grande.o board-mop500-msp.o \
					   board-rio-grande-mcde.o board-rio-grande-sdi.o \
					   gpio-nypon.o board-mop500-bm.o \
					   board-rio-grande-mmio.o charger-nypon.o \
					   leds-nypon.o board-mop500-regulators.o \
					   board-mop500-wlan.o board-pins-sleep-force.o board-mop500-vibra-nypon.o \
					   display-nypon.o simple_remote-rio-grande_r2.o input-nypon.o sim_detect_semc.o

obj-$(CONFIG_MACH_U8500_KUMQUAT)	+= board-rio-grande.o board-mop500-msp.o \
                                           board-rio-grande-mcde.o board-rio-grande-sdi.o \
                                           gpio-kumquat.o board-mop500-bm.o \
                                           board-rio-grande-mmio.o charger-kumquat.o \
                                           leds-kumquat.o board-mop500-regulators.o \
					   board-mop500-wlan.o board-pins-sleep-force.o board-mop500-vibra.o display-rio-grande.o \
					   simple_remote-rio-grande.o input-kumquat.o sim_detect_semc.o

obj-$(CONFIG_MACH_U8500_PEPPER)		+= board-rio-grande.o board-mop500-msp.o \
                                           board-rio-grande-mcde.o board-rio-grande-sdi.o \
                                           gpio-pepper.o board-mop500-bm.o \
                                           board-rio-grande-mmio.o charger-pepper.o \
                                           leds-pepper.o board-mop500-regulators.o \
					   board-mop500-wlan.o board-pins-sleep-force.o board-mop500-vibra-pepper.o display-rio-grande.o \
					   simple_remote-rio-grande.o input-pepper.o sim_detect_semc.o

obj-$(CONFIG_MACH_U8500_LOTUS)		+= board-rio-grande.o board-mop500-msp.o \
                                           board-rio-grande-mcde.o board-rio-grande-sdi.o \
                                           gpio-lotus.o board-mop500-bm.o \
                                           board-rio-grande-mmio.o charger-lotus.o \
                                           leds-lotus.o board-mop500-regulators.o \
					   board-mop500-wlan.o board-pins-sleep-force.o board-mop500-vibra.o display-rio-grande_r2.o \
					   simple_remote-rio-grande.o input-lotus.o sim_detect_semc.o

ifeq ($(CONFIG_MACH_U8500_PDP), y)
obj-$(CONFIG_MOP500_SDI)		+= board-rio-grande-sdi.o
else 
obj-$(CONFIG_MOP500_SDI)		+= board-mop500-sdi.o
endif
obj-$(CONFIG_UX500_SOC_DB5500)		+= devices-db5500.o cpu-db5500.o \
					   dma-db5500.o timer-db8500.o \
					   clock-db5500.o clock-debug.o
obj-$(CONFIG_UX500_SOC_DB8500)		+= devices-db8500.o \
					   cpu-db8500.o dma-db8500.o\
					   clock-db8500.o
obj-$(CONFIG_UX500_SOC_DB8500)		+= timer-db8500.o
obj-$(CONFIG_MACH_U8500_MOP)		+= board-mop500-mcde.o
obj-$(CONFIG_MACH_U5500_SIMULATOR)	+= board-u5500.o
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o
obj-$(CONFIG_U8500_PRCMU)		+= prcmu-db8500.o
obj-$(CONFIG_U5500_PRCMU)		+= prcmu-db5500.o
obj-$(CONFIG_UX500_PRCMU_QOS_POWER)	+= prcmu-qos-power.o
obj-$(CONFIG_UX500_PRCMU_DEBUG)		+= prcmu-debug.o
obj-$(CONFIG_SENSORS1P_MOP)            	+= sensors1p.o
obj-$(CONFIG_MACH_U5500_BB)		+= board-u5500.o
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o
ifeq ($(CONFIG_UX500_SOC_DB5500),y)
obj-$(CONFIG_USB)            		+= musb_db5500.o
else
obj-$(CONFIG_USB)            		+= musb_db8500.o
endif
obj-$(CONFIG_U5500_MLOADER)		+= mloader-db5500.o
obj-$(CONFIG_LOCAL_TIMERS)		+= localtimer.o
obj-$(CONFIG_U5500_MODEM_IRQ)		+= modem_irq.o
obj-$(CONFIG_U5500_PWM)			+= pwm.o
obj-$(CONFIG_MOP500_NUIB)		+= board-mop500-nuib.o
obj-$(CONFIG_DB8500_MLOADER)		+= mloader-db8500.o

obj-$(CONFIG_TEE_UX500)			+= tee_ux500.o product.o
obj-$(CONFIG_TEE_SVP)			+= tee_service_svp.o
obj-$(CONFIG_TEE_SVP)			+= tee_ta_start_modem_svp.o
obj-$(CONFIG_MFD_CG2900)		+= devices-cg2900.o \
					   clock-cg2900.o
obj-$(CONFIG_SIMPLE_REMOTE_PLATFORM)	+= simple_remote_ux500_pf.o

obj-$(CONFIG_UX500_DEBUG_HWREG) 	+= hwreg.o
obj-$(CONFIG_HWMEM)			+= hwmem-int.o
obj-$(CONFIG_UX500_L2X0_PREFETCH_CTRL)	+= l2x0-prefetch.o
obj-$(CONFIG_AB5500_BM)			+= board-u5500-bm.o
