add_executable(client
        client.c
        )

target_include_directories(client PUBLIC
  .
)

target_compile_definitions(client INTERFACE
	WANT_HCI_DUMP=1 # This enables btstack debug
)

target_link_libraries(
  client
  pico_stdlib
  pico_cyw43_arch_none
  pico_btstack_cyw43
  pico_btstack_classic
  pico_btstack_ble
  hardware_spi
)

# disable usb output, enable uart output
pico_enable_stdio_usb(client 0)
pico_enable_stdio_uart(client 1)

pico_add_extra_outputs(client)
