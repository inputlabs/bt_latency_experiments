/* TODO: Add license */

#include <stdio.h>

#include "pico/stdlib.h"

#include "pico/binary_info.h"
#include "hardware/spi.h"

#include "btstack_config.h"
#include "btstack.h"
#include "pico/cyw43_arch.h"

#include "hci_dump.h"
#include "hci_dump_embedded_stdout.h"

//#define ENABLE_HCI_DUMP

#define BUF_LEN         0x1
#define MAX_ATTRIBUTE_VALUE_SIZE 300


static btstack_packet_callback_registration_t hci_event_callback_registration;
static uint8_t hid_descriptor_storage[MAX_ATTRIBUTE_VALUE_SIZE];
static hid_protocol_mode_t hid_host_report_mode = HID_PROTOCOL_MODE_REPORT_WITH_FALLBACK_TO_BOOT;
static uint16_t hid_host_cid = 0; // connection ID
static bool hid_host_descriptor_available = false;



//static const char * remote_addr_string = "28:CD:C1:0C:D0:3B";
static const char * remote_addr_string = "28:CD:C1:06:C5:D4";

static bd_addr_t remote_addr;

static enum {
    APP_IDLE,
    APP_CONNECTED
} app_state = APP_IDLE;

static void hid_host_handle_interrupt_report(const uint8_t * report, uint16_t report_len){
  printf("handling interrupt\n");

  if (report_len < 1) return;
  if (*report != 0xa1) return; 
    
  report++;
  report_len--;

  printf("descriptor length: %d\n", hid_descriptor_storage_get_descriptor_len(hid_host_cid));
  printf("descriptor data: %x\n", hid_descriptor_storage_get_descriptor_data(hid_host_cid));


  btstack_hid_parser_t parser;
  btstack_hid_parser_init(&parser, 
    hid_descriptor_storage_get_descriptor_data(hid_host_cid), 
    hid_descriptor_storage_get_descriptor_len(hid_host_cid), 
    HID_REPORT_TYPE_INPUT, report, report_len);

  while (btstack_hid_parser_has_more(&parser)){
    uint16_t usage_page;
    uint16_t usage;
    int32_t  value;
    btstack_hid_parser_get_field(&parser, &usage_page, &usage, &value);

    if (usage_page != 0x07) continue;   
    switch (usage){ // https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-usages#usage-page
      case 0xe1:
      case 0xe6:
        continue;
      case 0x00:
        continue;
      default:
        break;
    }

	  printf("received page: %d, usage: %d, value %d\n", usage_page, usage, value);
  }
}


static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  uint8_t   event;
  uint8_t   subevent;
  uint8_t   status;

//  printf("Packet type 0x%02x, ", packet_type);
  
  switch (packet_type) {                                                // src/bluetooth.h
  case HCI_EVENT_PACKET:                                                // 0x04
//    printf("HCI_EVENT_PACKET\n");
    event = hci_event_packet_get_type(packet);
//    printf("Event: 0x%02x, ", event);

    switch (event) {                                                    //src/btstack_defines.h
    case HCI_EVENT_CONNECTION_COMPLETE:                                 // 0x03
//      printf("HCI_EVENT_CONNECTION_COMPLETE, doing nothing\n");
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:                                 // 0x05
//      printf("HCI_EVENT_DISCONNECTION_COMPLETE, doing nothing\n");
      break;
    case HCI_EVENT_AUTHENTICATION_COMPLETE:                             // 0x06
//      printf("HCI_EVENT_AUTHENTICATION_COMPLETE, doing nothing\n");
      break;
    case HCI_EVENT_ENCRYPTION_CHANGE:                                   // 0x08
//      printf("HCI_EVENT_ENCRYPTION_CHANGE, doing nothing\n");
      break;
    case HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE:             // 0x0B
//      printf("HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE, doing nothing\n");
      break;
    case HCI_EVENT_COMMAND_COMPLETE:                                    // 0x0E
//      printf("HCI_EVENT_COMMAND_COMPLETE, doing nothing\n");
      break;
    case HCI_EVENT_COMMAND_STATUS:                                      // 0x0F
//      printf("HCI_EVENT_COMMAND_STATUS, doing nothing\n");
      break;
    case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:                         // 0x13
//      printf("HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS, doing nothing\n");
      break;
    case HCI_EVENT_LINK_KEY_REQUEST:                                    // 0x17
//      printf("HCI_EVENT_LINK_KEY_REQUEST, doing nothing\n");
      break;
    case HCI_EVENT_MAX_SLOTS_CHANGED:                                   // 0x1B
//      printf("HCI_EVENT_MAX_SLOTS_CHANGED, doing nothing\n");
      break;
    case HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE:              // 0x23
//      printf("HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE, doing nothing\n");
      break;
    case BTSTACK_EVENT_STATE:                                           // 0x60
      uint8_t state = btstack_event_state_get_state(packet);
//      printf("BTSTACK_EVENT_STATE\n");
//      printf("State: 0x%02x, ", state);

      switch (state) {                                                  // src/hci_cmd.h
      case HCI_STATE_INITIALIZING:                                      // 0x1
//	      printf("HCI_STATE_INITIALIZING, doing nothing\n");
	      break;
      case HCI_STATE_WORKING:                                           // 0x2
//	      printf("HCI_STATE_WORKING\n");
	      printf("!!! Trying to connect to to %s in HID Host report mode\n", bd_addr_to_str(remote_addr));
	      status = hid_host_connect(remote_addr, hid_host_report_mode, &hid_host_cid);
	      if (status != ERROR_CODE_SUCCESS){
	        printf("HID host connect failed, status 0x%02x.\n", status);
	      }
	      break;
      default:
	      printf("\n---\n!!! Defaulting on state 0x%02x\n---\n", state);
	      break;
      }
      
      break;
    case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:                          // 0x61
//      printf("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED, doing nothing\n");
      break;
    case BTSTACK_EVENT_SCAN_MODE_CHANGED:                               // 0x66
//      printf("BTSTACK_EVENT_SCAN_MODE_CHANGED, doing nothing\n");
      break;
    case GAP_EVENT_SECURITY_LEVEL:                                      // 0xD8
//      printf("GAP_EVENT_SECURITY_LEVEL, doing nothing\n");
      break;
    case HCI_EVENT_TRANSPORT_PACKET_SENT:                               // 0x6E
//      printf("HCI_EVENT_TRANSPORT_PACKET_SENT, doing nothing\n");
      break;
    case HCI_EVENT_HID_META:                                            // 0xEF
//      printf("HCI_EVENT_HID_META\n");
      subevent = hci_event_hid_meta_get_subevent_code(packet);
//      printf("Subevent: 0x%02x, ", subevent);
      
      switch (subevent){
        case HID_SUBEVENT_CONNECTION_OPENED:                            // 0x02
//	        printf("HID_SUBEVENT_CONNECTION_OPENED\n");
          status = hid_subevent_connection_opened_get_status(packet);

          if (status != ERROR_CODE_SUCCESS) {
            printf("Connection failed, status 0x%02x\n", status);
            app_state = APP_IDLE;
            hid_host_cid = 0;
            return;
          }

          app_state = APP_CONNECTED;
          hid_host_descriptor_available = false;
          hid_host_cid = hid_subevent_connection_opened_get_hid_cid(packet);
          printf("HID Host connected.\n");
	        break;
        case HID_SUBEVENT_CONNECTION_CLOSED:                            // 0x03
//	        printf("HID_SUBEVENT_CONNECTION_CLOSED, doing nothing\n");
          break;
        case HID_SUBEVENT_REPORT:                         // 0x0C
//	        printf("HID_SUBEVENT_REPORT: \n");
//          printf_hexdump(hid_subevent_report_get_report(packet), hid_subevent_report_get_report_len(packet));

          if (hid_host_descriptor_available){
            const uint8_t *report = hid_subevent_report_get_report(packet);
            printf("Received keycode %02x via BT\n", report[4]);

            //send via SPI
	          printf("Sending keycode %02x via SPI\n", report[4]);

	          uint8_t out_buf[BUF_LEN];
	  
	          out_buf[0] = report[4];      
	          spi_write_blocking(spi_default, out_buf, BUF_LEN);
          } else {
            printf("No host descriptor\n");
          }

          break;
        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:                         // 0x0D
//	        printf("HID_SUBEVENT_DESCRIPTOR_AVAILABLE, doing nothing\n");
          status = hid_subevent_descriptor_available_get_status(packet);

          if (status == ERROR_CODE_SUCCESS){
            hid_host_descriptor_available = true;
//            printf("HID Descriptor available, please start typing.\n");
          } else {
            printf("Cannot handle input report, HID Descriptor is not available, status 0x%02x\n", status);
          }

	        break;
        case HID_SUBEVENT_SNIFF_SUBRATING_PARAMS:                       // 0x0E
//	        printf("HID_SUBEVENT_SNIFF_SUBRATING_PARAMS, doing nothing\n");
	        break;
        default:
	        printf("\n---\n!!! Defaulting on subevent 0x%02x\n---\n", subevent);
	        break;
      }
      
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

static void hid_host_setup(void){
      l2cap_init();
      
#ifdef ENABLE_BLE // mp needed for pico?
    // Initialize LE Security Manager. Needed for cross-transport key derivation
    sm_init();
#endif

    // Initialize HID Host
    hid_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    hid_host_register_packet_handler(packet_handler);

    // Allow sniff mode requests by HID device and support role switch
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);

    // try to become master on incoming connections
    hci_set_master_slave_policy(HCI_ROLE_MASTER);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Disable stdout buffering //mp needed?
    setvbuf(stdout, NULL, _IONBF, 0);
}

int main()
{
  stdio_init_all();
  
  int i = 5;

  while (i > 0) {
    printf("Starting BT host / SPI master in: %d\n", i);
    i--;
    sleep_ms(1000);
  }

  // if EANBLE_HCI_DUMP is defined, enable debug output as well as packet logger
#ifdef ENABLE_HCI_DUMP
  hci_dump_init(hci_dump_embedded_stdout_get_instance());
  hci_dump_enable_packet_log(true);
#endif

  // Enable SPI 0 at 1 MHz and connect to GPIOs
  spi_init(spi_default, 1000 * 1000);
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
  
  hid_host_setup();
  sscanf_bd_addr(remote_addr_string, remote_addr); // parse human readable Bluetooth address
  hci_power_control(HCI_POWER_ON);
  
  btstack_run_loop_execute() ;
}
