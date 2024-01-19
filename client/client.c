#include <stdio.h>

#include "pico/stdlib.h"

#include "pico/binary_info.h"
#include "hardware/spi.h"

#include "btstack.h"
#include "pico/cyw43_arch.h"

#include "hci_dump.h"
#include "hci_dump_embedded_stdout.h"

#include <inttypes.h> // needed?

//#define ENABLE_HCI_DUMP

#define REPORT_ID 0x01

#define BUF_LEN 0x1 // SPI buffer

#define DELAY_MS 1


static uint8_t hid_service_buffer[300];
static uint8_t device_id_sdp_service_buffer[100];
static uint16_t hid_cid; // needed?


static const char hid_device_name[] = "BTstack HID Test client";


static uint8_t hid_boot_device = 0;
static uint16_t host_max_latency = 1600;
static uint16_t host_min_timeout = 3200;
static uint8_t                send_buffer_storage[16];
static btstack_ring_buffer_t  send_buffer;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static enum {
    APP_BOOTING,
    APP_NOT_CONNECTED,
    APP_CONNECTING,
    APP_CONNECTED
} app_state = APP_BOOTING;

// from USB HID Specification 1.1, Appendix B.2
const uint8_t hid_descriptor_mouse_boot_mode[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)

    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)

    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)

    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)

    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

int keycode = 4;
static btstack_timer_source_t timer;

static void timer_handler(btstack_timer_source_t * ts){
//  printf("Restarting timer with keycode %d\n", keycode);

  if(keycode >= 39){ // ensuring that this does not conflict with expected keyboard inputs
    keycode = 4;     // keys a to z (keycode 4 - 29) and 1 to 0 (30 - 39)
  } else {
      keycode++;

  btstack_run_loop_set_timer_handler(&timer, timer_handler);
  btstack_run_loop_set_timer(&timer, DELAY_MS);
  btstack_run_loop_add_timer(&timer);

  hid_device_request_can_send_now_event(hid_cid);
  }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t * packet, uint16_t packet_size){
  uint8_t   event;
  uint8_t   subevent;
  uint8_t   status;

//  printf("Packet type 0x%02x, ", packet_type);

  switch (packet_type) {                                                   // src/bluetooth.h
    case HCI_EVENT_PACKET:                                                 // 0x04
//      printf("HCI_EVENT_PACKET\n");
      event = hci_event_packet_get_type(packet);
//      printf("Event: 0x%02x, ", event);

      switch (event) {                                                    //src/btstack_defines.h
        case HCI_EVENT_CONNECTION_COMPLETE:                               // 0x03
//          printf("HCI_EVENT_CONNECTION_COMPLETE, doing nothing\n");
          break;
        case HCI_EVENT_CONNECTION_REQUEST:                                // 0x04
//          printf("HCI_EVENT_CONNECTION_REQUEST, doing nothing\n");
          break;
        case HCI_EVENT_ENCRYPTION_CHANGE:                                 // 0x08
//          printf("HCI_EVENT_ENCRYPTION_CHANGE, doing nothing\n");
          break;
        case HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE:           // 0x0B
//          printf("HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE, doing nothing\n");
          break;
        case HCI_EVENT_COMMAND_COMPLETE:                                  // 0x0E
//          printf("HCI_EVENT_COMMAND_COMPLETE, doing nothing\n");
          break;
        case HCI_EVENT_COMMAND_STATUS:                                    // 0x0F
//          printf("HCI_EVENT_COMMAND_STATUS, doing nothing\n");
          break;
        case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:                       // 0x13
//          printf("HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS, doing nothing\n");
          break;
        case HCI_EVENT_LINK_KEY_REQUEST:                                  // 0x17
//          printf("HCI_EVENT_LINK_KEY_REQUEST, doing nothing\n");
          break;
        case HCI_EVENT_MAX_SLOTS_CHANGED:                                 // 0x1B
//          printf("HCI_EVENT_MAX_SLOTS_CHANGED, doing nothing\n");
          break;
        case HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE:            // 0x23
//          printf("HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE, doing nothing\n");
          break;
        case HCI_EVENT_IO_CAPABILITY_REQUEST:                             // 0x31
//            print("HCI_EVENT_IO_CAPABILITY_REQUEST, doing nothing\n");
            break;
        case HCI_EVENT_IO_CAPABILITY_RESPONSE:                             // 0x32
//            print("HCI_EVENT_IO_CAPABILITY_RESPONSE, doing nothing\n");
            break;
        case HCI_EVENT_USER_CONFIRMATION_REQUEST:                          // 0x33
//            print("HCI_EVENT_USER_CONFIRMATION_REQUEST, doing nothing\n");
            break;
        case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:                            // 0x36
//            print("HCI_EVENT_SIMPLE_PAIRING_COMPLETE, doing nothing\n");
            break;
        case HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED:                   // 0x38
//            print("HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED, doing nothing\n");
            break;

        case BTSTACK_EVENT_STATE:                                         // 0x60
          uint8_t state = btstack_event_state_get_state(packet);
//          printf("BTSTACK_EVENT_STATE\n");
//          printf("State: 0x%02x, ", state);


          switch (state) {                                                // src/hci_cmd.h
            case HCI_STATE_INITIALIZING:                                  // 0x1
//	            printf("HCI_STATE_INITIALIZING, doing nothing\n");
              return;
	            break;
            case HCI_STATE_WORKING:                                       // 0x2
//	            printf("HCI_STATE_WORKING\n");              
//	            printf("!!! Waiting for connection\n");
              app_state = APP_NOT_CONNECTED;
	            break;
            default:
	            printf("\n---\n!!! Defaulting on state 0x%02x\n---\n", state);
              return;
	            break;
          }

          break;
        case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:                        // 0x61
//          printf("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED, doing nothing\n");
          break;
        case BTSTACK_EVENT_SCAN_MODE_CHANGED:                             // 0x66
//          printf("BTSTACK_EVENT_SCAN_MODE_CHANGED, doing nothing\n");
          break;
        case HCI_EVENT_TRANSPORT_PACKET_SENT:                             // 0x6E
//          printf("HCI_EVENT_TRANSPORT_PACKET_SENT, doing nothing\n");
          break;

        case GAP_EVENT_SECURITY_LEVEL:                                    // 0xD8
//          printf("GAP_EVENT_SECURITY_LEVEL, doing nothing\n");
          break;
        case HCI_EVENT_HID_META:                                          // 0xEF
//          printf("HCI_EVENT_HID_META\n");
          subevent = hci_event_hid_meta_get_subevent_code(packet);
//          printf("Subevent: 0x%02x, ", subevent);

          switch (subevent){
            case HID_SUBEVENT_CONNECTION_OPENED:                          // 0x02
//	            printf("HID_SUBEVENT_CONNECTION_OPENED\n");
              status = hid_subevent_connection_opened_get_status(packet);

              if (status != ERROR_CODE_SUCCESS) {
                printf("Connection failed, status 0x%x\n", status);
                app_state = APP_NOT_CONNECTED;
                hid_cid = 0;
                return;
              }
              app_state = APP_CONNECTED;
              hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
              printf("HID Connected. Starting timer to send new keycode every %d ms\n", DELAY_MS);
              timer_handler(NULL);
			        hid_device_request_can_send_now_event(hid_cid);

              gap_discoverable_control(0); // disabling to reduce latency
              gap_connectable_control(0);  // disabling to reduce latency
	            break;
            case HID_SUBEVENT_CONNECTION_CLOSED:                          // 0x03
//	            printf("HID_SUBEVENT_CONNECTION_CLOSED, doing nothing\n");
              break;
            case HID_SUBEVENT_CAN_SEND_NOW:                               // 0x04
//	            printf("HID_SUBEVENT_CAN_SEND_NOW\n");

              uint8_t report[] = { 0xa1, REPORT_ID, 0, 0, keycode, 0, 0, 0, 0, 0};

//              printf("Sending report:\n");
//              printf_hexdump(report, sizeof(report));
//              printf("Sending keycode %d via BT and starting SPI feedback timer\n", keycode);

			        uint32_t tick_start = time_us_32();
              hid_device_send_interrupt_message(hid_cid, &report[0], sizeof(report));

			        uint8_t in_buf[BUF_LEN];
      			  uint8_t bytes_read = 0;

              spi_read_blocking(spi_default, 0, in_buf, 1);
              if (in_buf[0] == keycode){
                uint32_t tick_completed = time_us_32() - tick_start;
			          printf("number of microseconds: %lu\n", tick_completed);
              } else {
			          printf("!!! Incorrectly received from SPI: %02x, ", in_buf[0]);
              }

	            break;
            case HID_SUBEVENT_SNIFF_SUBRATING_PARAMS:     // 0x0E
//	            printf("HID_SUBEVENT_SNIFF_SUBRATING_PARAMS, doing nothing\n");
	            break;
            default:
	            printf("\n---\n!!! Defaulting on subevent 0x%02x\n---\n", subevent);
	            break;
          }

          break;
        case HCI_EVENT_VENDOR_SPECIFIC:                                   // 0xFF
//          printf("HCI_EVENT_VENDOR_SPECIFIC, doing nothing\n");
          break;
        default:
          printf("\n---\n!!! Defaulting on event type 0x%02x\n---\n", event);
          break;
      }

      break;
    default:
      printf("\n---\n!!! Defaulting on packet type 0x%02x\n---\n", packet_type);
    break;
  }
}



static void hid_device_setup(void){
  // allow to get found by inquiry
  gap_discoverable_control(1);
  // use Limited Discoverable Mode; Peripheral; Pointing Device as CoD
  gap_set_class_of_device(0x2580);
  // set local name to be identified - zeroes will be replaced by actual BD ADDR
  gap_set_local_name("HID Keyboard Demo 00:00:00:00:00:00");
  // allow for role switch in general and sniff mode
  gap_set_default_link_policy_settings( LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE );
  // allow for role switch on outgoing connections - this allow HID Host to become master when we re-connect to it
  gap_set_allow_role_switch(true);

  l2cap_init();

#ifdef ENABLE_BLE // mp needed for pico?
  // Initialize LE Security Manager. Needed for cross-transport key derivation
  sm_init();
#endif
  
  // SDP Server
  sdp_init();
  memset(hid_service_buffer, 0, sizeof(hid_service_buffer));
  
  uint8_t hid_virtual_cable = 0;
  uint8_t hid_remote_wake = 1;
  uint8_t hid_reconnect_initiate = 1;
  uint8_t hid_normally_connectable = 1;

  hid_sdp_record_t hid_params = {
    // hid sevice subclass 2580 Mouse, hid counntry code 33 US
    0x2580, 33, 
    hid_virtual_cable, hid_remote_wake, 
    hid_reconnect_initiate, hid_normally_connectable,
    hid_boot_device,
    host_max_latency, host_min_timeout,
    3200,
    hid_descriptor_mouse_boot_mode,
    sizeof(hid_descriptor_mouse_boot_mode),
    hid_device_name
  };
    
  hid_create_sdp_record(hid_service_buffer, sdp_create_service_record_handle(), &hid_params);
  btstack_assert(de_get_len( hid_service_buffer) <= sizeof(hid_service_buffer));
  sdp_register_service(hid_service_buffer);

  // See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers if you don't have a USB Vendor ID and need a Bluetooth Vendor ID
  // device info: BlueKitchen GmbH, product 1, version 1
  device_id_create_sdp_record(device_id_sdp_service_buffer, sdp_create_service_record_handle(), DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH, BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, 1, 1);
  btstack_assert(de_get_len( device_id_sdp_service_buffer) <= sizeof(device_id_sdp_service_buffer));
  sdp_register_service(device_id_sdp_service_buffer);

  // HID Device
  hid_device_init(hid_boot_device, sizeof(hid_descriptor_mouse_boot_mode), hid_descriptor_mouse_boot_mode);

  // register for HCI events
  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // register for HID events
  hid_device_register_packet_handler(&packet_handler);

  btstack_ring_buffer_init(&send_buffer, send_buffer_storage, sizeof(send_buffer_storage));
}


int main()
{
  stdio_init_all();

  int i = 5;

  while (i > 0) {
    printf("Starting BT client / SPI slave in: %d\n", i);
    i--;
    sleep_ms(1000);
  }

#ifdef ENABLE_HCI_DUMP 
  hci_dump_init(hci_dump_embedded_stdout_get_instance()); 
  hci_dump_enable_packet_log(true); 
#endif

  // Enable SPI 0 at 1 MHz and connect to GPIOs
  spi_init(spi_default, 1000 * 1000);
  spi_set_slave(spi_default, true);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
  // Make the SPI pins available to picotool
  bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI));

  // BT Init
  if (cyw43_arch_init()) {
    printf("failed to initialise cyw43_arch\n");
    return -1;
  }

  hid_device_setup();
  hci_power_control(HCI_POWER_ON);
    
  btstack_run_loop_execute() ;
}
