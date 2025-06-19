################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c -mcpu=cortex-m4 -mfloat-abi=hard -mlittle-endian -mthumb -I"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/include/c" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/dpc" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/test" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/mmwave_control" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/power_management" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/calibrations" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/utils" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/FreeRTOS-Kernel/include" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/portable/TI_ARM_CLANG/ARM_CM4F" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/config/xwrL64xx/m4f" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/firmware/mmwave_dfp" -DSOC_XWRL64XX -DGTRACK_3D=1 -DTRACKER_MAX_NUM_TR=10 -D_DEBUG_=1 -g -Wall -Wno-gnu-variable-sized-type-not-at-end -Wno-unused-function -mno-unaligned-access -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/prath/workspace_ccstheia/FallDetection/Debug/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-197470367: ../example.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs2020/ccs/utils/sysconfig_1.24.0/sysconfig_cli.bat" --script "C:/Users/prath/workspace_ccstheia/FallDetection/example.syscfg" -o "syscfg" -s "C:/ti/MMWAVE_L_SDK_05_05_03_00/.metadata/product.json" -p "AOP" -r "Default" --context "m4fss0-0" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_dpl_config.c: build-197470367 ../example.syscfg
syscfg/ti_dpl_config.h: build-197470367
syscfg/ti_drivers_config.c: build-197470367
syscfg/ti_drivers_config.h: build-197470367
syscfg/ti_drivers_open_close.c: build-197470367
syscfg/ti_drivers_open_close.h: build-197470367
syscfg/ti_pinmux_config.c: build-197470367
syscfg/ti_power_clock_config.c: build-197470367
syscfg/ti_board_config.c: build-197470367
syscfg/ti_board_config.h: build-197470367
syscfg/ti_board_open_close.c: build-197470367
syscfg/ti_board_open_close.h: build-197470367
syscfg: build-197470367

syscfg/%.o: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c -mcpu=cortex-m4 -mfloat-abi=hard -mlittle-endian -mthumb -I"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/include/c" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/dpc" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/test" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/mmwave_control" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/power_management" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/calibrations" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/examples/mmw_demo/motion_and_presence_detection/source/utils" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/FreeRTOS-Kernel/include" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/portable/TI_ARM_CLANG/ARM_CM4F" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/source/kernel/freertos/config/xwrL64xx/m4f" -I"C:/ti/MMWAVE_L_SDK_05_05_03_00/firmware/mmwave_dfp" -DSOC_XWRL64XX -DGTRACK_3D=1 -DTRACKER_MAX_NUM_TR=10 -D_DEBUG_=1 -g -Wall -Wno-gnu-variable-sized-type-not-at-end -Wno-unused-function -mno-unaligned-access -MMD -MP -MF"syscfg/$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/Users/prath/workspace_ccstheia/FallDetection/Debug/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


