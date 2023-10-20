################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1230/ccs/tools/compiler/ti-cgt-armllvm_2.1.3.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/Users/matthewbass/workspace_v12/project_milestone_3" -I"/Users/matthewbass/workspace_v12/project_milestone_3/MCU+Image" -I"/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/source" -I"/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/kernel/nortos" -I"/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/kernel/nortos/posix" -DDeviceFamily_CC3220 -DNORTOS_SUPPORT -gdwarf-3 -march=armv7e-m -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"/Users/matthewbass/workspace_v12/project_milestone_3/MCU+Image/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-212935423: ../gpiointerrupt.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"/Users/matthewbass/ti/sysconfig_1_12_0/sysconfig_cli.sh" -s "/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/.metadata/product.json" --script "/Users/matthewbass/workspace_v12/project_milestone_3/gpiointerrupt.syscfg" -o "syscfg" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_drivers_config.c: build-212935423 ../gpiointerrupt.syscfg
syscfg/ti_drivers_config.h: build-212935423
syscfg/ti_utils_build_linker.cmd.genlibs: build-212935423
syscfg/syscfg_c.rov.xs: build-212935423
syscfg/: build-212935423

syscfg/%.o: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1230/ccs/tools/compiler/ti-cgt-armllvm_2.1.3.LTS/bin/tiarmclang" -c -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=none -mlittle-endian -mthumb -Oz -I"/Users/matthewbass/workspace_v12/project_milestone_3" -I"/Users/matthewbass/workspace_v12/project_milestone_3/MCU+Image" -I"/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/source" -I"/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/kernel/nortos" -I"/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/kernel/nortos/posix" -DDeviceFamily_CC3220 -DNORTOS_SUPPORT -gdwarf-3 -march=armv7e-m -MMD -MP -MF"syscfg/$(basename $(<F)).d_raw" -MT"$(@)" -I"/Users/matthewbass/workspace_v12/project_milestone_3/MCU+Image/syscfg"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1533698193: ../image.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"/Users/matthewbass/ti/sysconfig_1_12_0/sysconfig_cli.sh" -s "/Users/matthewbass/ti/simplelink_cc32xx_sdk_7_10_00_13/.metadata/product.json" --script "/Users/matthewbass/workspace_v12/project_milestone_3/image.syscfg" -o "syscfg" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_drivers_net_wifi_config.json: build-1533698193 ../image.syscfg
syscfg/: build-1533698193


