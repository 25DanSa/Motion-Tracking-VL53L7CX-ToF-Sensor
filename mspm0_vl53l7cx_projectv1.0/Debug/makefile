################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS

GEN_OPTS__FLAG := @"./device.opt" 
GEN_CMDS__FLAG := -Wl,-l"./device_linker.cmd" 

ORDERED_OBJS += \
"./ti_msp_dl_config.o" \
"./startup_mspm0l111x_ticlang.o" \
"./VL53L7CX_ULD/api/vl53l7cx_api.o" \
"./VL53L7CX_ULD/api/vl53l7cx_plugin_detection_thresholds.o" \
"./VL53L7CX_ULD/api/vl53l7cx_plugin_motion_indicator.o" \
"./VL53L7CX_ULD/api/vl53l7cx_plugin_xtalk.o" \
"./VL53L7CX_ULD/platform/platform.o" \
"./source/main.o" \
"./source/ti_msp_dl_config.o" \
"./source/vl53l7cx_i2c_if.o" \
"./source/vl53l7cx_uart_if.o" \
$(GEN_CMDS__FLAG) \
-Wl,-ldevice.cmd.genlibs \
-Wl,-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include VL53L7CX_ULD/api/subdir_vars.mk
-include VL53L7CX_ULD/platform/subdir_vars.mk
-include source/subdir_vars.mk
-include subdir_rules.mk
-include VL53L7CX_ULD/api/subdir_rules.mk
-include VL53L7CX_ULD/platform/subdir_rules.mk
-include source/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
mspm0_vl53l7cx_projectv1.0.out 

EXE_OUTPUTS__QUOTED += \
"mspm0_vl53l7cx_projectv1.0.out" 


# All Target
all: $(OBJS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "mspm0_vl53l7cx_projectv1.0.out"

# Tool invocations
mspm0_vl53l7cx_projectv1.0.out: $(OBJS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: Arm Linker'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -gdwarf-3 -Wl,-m"mspm0_vl53l7cx_projectv1.0.map" -Wl,-i"C:/ti/mspm0_sdk_2_07_00_05/source" -Wl,-i"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0" -Wl,-i"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/Debug/syscfg" -Wl,-i"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/lib" -Wl,-i"C:/Users/dsabic/workspace_ccstheia/mspm0_vl53l7cx_projectv1.0/source" -Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--warn_sections -Wl,--xml_link_info="mspm0_vl53l7cx_projectv1.0_linkInfo.xml" -Wl,--rom_model -o "mspm0_vl53l7cx_projectv1.0.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(GEN_MISC_FILES__QUOTED)$(GEN_FILES__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "ti_msp_dl_config.o" "startup_mspm0l111x_ticlang.o" "VL53L7CX_ULD\api\vl53l7cx_api.o" "VL53L7CX_ULD\api\vl53l7cx_plugin_detection_thresholds.o" "VL53L7CX_ULD\api\vl53l7cx_plugin_motion_indicator.o" "VL53L7CX_ULD\api\vl53l7cx_plugin_xtalk.o" "VL53L7CX_ULD\platform\platform.o" "source\main.o" "source\ti_msp_dl_config.o" "source\vl53l7cx_i2c_if.o" "source\vl53l7cx_uart_if.o" 
	-$(RM) "ti_msp_dl_config.d" "startup_mspm0l111x_ticlang.d" "VL53L7CX_ULD\api\vl53l7cx_api.d" "VL53L7CX_ULD\api\vl53l7cx_plugin_detection_thresholds.d" "VL53L7CX_ULD\api\vl53l7cx_plugin_motion_indicator.d" "VL53L7CX_ULD\api\vl53l7cx_plugin_xtalk.d" "VL53L7CX_ULD\platform\platform.d" "source\main.d" "source\ti_msp_dl_config.d" "source\vl53l7cx_i2c_if.d" "source\vl53l7cx_uart_if.d" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-powershell -Command "$map=Get-Content 'Debug\mspm0_vl53l7cx_projectv1.0.map';$flashLine=($map | Select-String 'FLASH');$sramLine=($map | Select-String 'SRAM');if ($flashLine) { $flash=($flashLine -split '\s+')[-4];$flashTotal=0x20000;$flashKB=[math]::Round(([convert]::ToInt32($flash,16))/1024,2);$flashPct=[math]::Round((([convert]::ToInt32($flash,16))/[double]$flashTotal*100),1);Write-Host ('FLASH used: ' + $flashKB + ' KB (' + $flashPct + '%)');} if ($sramLine) { $sram=($sramLine -split '\s+')[-4];$sramTotal=0x4000;$sramKB=[math]::Round(([convert]::ToInt32($sram,16))/1024,2);$sramPct=[math]::Round((([convert]::ToInt32($sram,16))/[double]$sramTotal*100),1);Write-Host ('SRAM  used: ' + $sramKB + ' KB (' + $sramPct + '%)');}"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

