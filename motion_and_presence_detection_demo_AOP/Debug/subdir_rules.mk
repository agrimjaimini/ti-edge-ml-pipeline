################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-844411343: ../example.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs2020/ccs/utils/sysconfig_1.24.0/sysconfig_cli.bat" --script "C:/Users/prath/workspace_ccstheia/motion_and_presence_detection_demo_AOP/example.syscfg" -o "syscfg" -s "C:/ti/MMWAVE_L_SDK_05_05_03_00/.metadata/product.json" -p "AOP" -r "Default" --context "m4fss0-0" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_dpl_config.c: build-844411343 ../example.syscfg
syscfg/ti_dpl_config.h: build-844411343
syscfg/ti_drivers_config.c: build-844411343
syscfg/ti_drivers_config.h: build-844411343
syscfg/ti_drivers_open_close.c: build-844411343
syscfg/ti_drivers_open_close.h: build-844411343
syscfg/ti_pinmux_config.c: build-844411343
syscfg/ti_power_clock_config.c: build-844411343
syscfg/ti_board_config.c: build-844411343
syscfg/ti_board_config.h: build-844411343
syscfg/ti_board_open_close.c: build-844411343
syscfg/ti_board_open_close.h: build-844411343
syscfg/ti_cli_mpd_demo_config.h: build-844411343
syscfg: build-844411343

syscfg/%.o: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c -mcpu=cortex-m4 -mfloat-abi=hard -mlittle-endian -mthumb -I"C:/Users/prath/workspace_ccstheia/motion_and_presence_detection_demo_AOP" -I"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/include/c" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/FreeRTOS-Kernel/include" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/portable/TI_ARM_CLANG/ARM_CM4F" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/config/xwrL64xx/m4f" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/firmware/mmwave_dfp" -DSOC_XWRL64XX -DGTRACK_3D=1 -DTRACKER_MAX_NUM_TR=10 -DAOP_DEVICE=1 -D_DEBUG_=1 -g -Wall -Wno-gnu-variable-sized-type-not-at-end -Wno-unused-function -mno-unaligned-access -MMD -MP -MF"syscfg/$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/prath/workspace_ccstheia/motion_and_presence_detection_demo_AOP/Debug/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c -mcpu=cortex-m4 -mfloat-abi=hard -mlittle-endian -mthumb -I"C:/Users/prath/workspace_ccstheia/motion_and_presence_detection_demo_AOP" -I"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/include/c" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/FreeRTOS-Kernel/include" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/portable/TI_ARM_CLANG/ARM_CM4F" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/config/xwrL64xx/m4f" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/firmware/mmwave_dfp" -DSOC_XWRL64XX -DGTRACK_3D=1 -DTRACKER_MAX_NUM_TR=10 -DAOP_DEVICE=1 -D_DEBUG_=1 -g -Wall -Wno-gnu-variable-sized-type-not-at-end -Wno-unused-function -mno-unaligned-access -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/prath/workspace_ccstheia/motion_and_presence_detection_demo_AOP/Debug/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


