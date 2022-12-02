export PICO_TOOL_PATH=$PICO_SDK_PATH/../picotool/build

$PICO_TOOL_PATH/picotool reboot -f -u
sleep 1
$PICO_TOOL_PATH/picotool load probeInterfaceG2.uf2
$PICO_TOOL_PATH/picotool reboot
