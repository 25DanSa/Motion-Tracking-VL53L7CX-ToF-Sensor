################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-2059835889: ../ti_drivers_config.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/sysconfig_1.25.0/sysconfig_cli.bat" --script "C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/ti_drivers_config.syscfg" -o "." -s "C:/ti/mspm0_sdk_2_07_00_05/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-2059835889 ../ti_drivers_config.syscfg
device.opt: build-2059835889
device.cmd.genlibs: build-2059835889
ti_msp_dl_config.c: build-2059835889
ti_msp_dl_config.h: build-2059835889
Event.dot: build-2059835889

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/Debug" -I"C:/ti/mspm0_sdk_2_07_00_05/source/ti/driverlib" -I"C:/ti/mspm0_sdk_2_07_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_07_00_05/source" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/VL53L7CX_ULD/platform" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/VL53L7CX_ULD/api" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/source" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/include" -I"C:/ti/mspm0_sdk_2_07_00_05/source/ti/driverlib/m0p" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0l111x_ticlang.o: C:/ti/mspm0_sdk_2_07_00_05/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0l111x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/Debug" -I"C:/ti/mspm0_sdk_2_07_00_05/source/ti/driverlib" -I"C:/ti/mspm0_sdk_2_07_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_07_00_05/source" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/VL53L7CX_ULD/platform" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/VL53L7CX_ULD/api" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/source" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/include" -I"C:/ti/mspm0_sdk_2_07_00_05/source/ti/driverlib/m0p" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


