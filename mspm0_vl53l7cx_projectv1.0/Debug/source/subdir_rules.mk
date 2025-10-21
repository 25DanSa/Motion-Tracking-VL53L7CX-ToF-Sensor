################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/Debug" -I"C:/ti/mspm0_sdk_2_07_00_05/source/ti/driverlib" -I"C:/ti/mspm0_sdk_2_07_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_07_00_05/source" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/VL53L7CX_ULD/platform" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/VL53L7CX_ULD/api" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/source" -I"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/include" -I"C:/ti/mspm0_sdk_2_07_00_05/source/ti/driverlib/m0p" -gdwarf-3 -MMD -MP -MF"source/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


