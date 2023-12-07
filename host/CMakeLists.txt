add_executable(host
        host.c
        )

target_include_directories(host PUBLIC
  .
)

target_compile_definitions(host INTERFACE
	WANT_HCI_DUMP=1 # This enables btstack debug
)
	
target_link_libraries(
  host
  pico_stdlib
  pico_cyw43_arch_none
  pico_btstack_cyw43
  pico_btstack_classic
  pico_btstack_ble
  hardware_spi
)

# enable usb output, disable uart output
pico_enable_stdio_usb(host 1)
pico_enable_stdio_uart(host 0)

pico_add_extra_outputs(host)
