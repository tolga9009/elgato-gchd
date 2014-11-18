#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

#define ELGATO_VID		0x0fd9
#define GAME_CAPTURE_HD	0x004e

/*
 *  Check for VID & PID
 */
int is_usbdevblock(libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	libusb_get_device_descriptor(dev, &desc);

	if (desc.idVendor == ELGATO_VID && desc.idProduct == GAME_CAPTURE_HD) {
		return 1;
	}

	return 0;
}

int main(void) {
	// discover devices
	libusb_device **list;
	libusb_device *found = NULL;
	libusb_context *ctx = NULL;
	int attached = 0;

	libusb_init(&ctx);
	libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_DEBUG); 
	ssize_t cnt = libusb_get_device_list(ctx, &list);
	ssize_t i = 0;
	int err = 0;
	if (cnt < 0) {
		printf("no usb devices found\n");

		return 1;
	}

	// find our device
	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		if (is_usbdevblock(device)) {
			found = device;
			break;
		}
	}

	if (found) {
		printf("found usb-dev-block!\n");
    		libusb_device_handle *handle;	
		err = libusb_open(found, &handle);
		if (err) {
			printf("Unable to open usb device\n");
			return 1;
		}

		if (libusb_kernel_driver_active(handle,0)) { 
			printf("Device busy...detaching...\n"); 
			libusb_detach_kernel_driver(handle,0); 
			attached = 1;
		} else printf("Device free from kernel\n"); 

		libusb_set_configuration(handle, 1);

		err = libusb_claim_interface(handle, 0);
		if (err) {
			printf("Failed to claim interface.");
			switch (err) {
			case LIBUSB_ERROR_NOT_FOUND:	printf("not found\n");	break;
			case LIBUSB_ERROR_BUSY:		printf("busy\n");		break;
			case LIBUSB_ERROR_NO_DEVICE:	printf("no device\n");	break;
			default:			printf("other\n");		break;
			}

			return 1;
		}

		unsigned char data[2048];
		unsigned char recv[256];

		int transferred = 0;

		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x0094, recv, 4, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x0098, recv, 4, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x0010, recv, 4, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x0014, recv, 4, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x0018, recv, 4, 0);

		libusb_control_transfer(handle, 0x40, 0xbc, 0x0900, 0x0000, recv, 2, 0);

		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0900, 0x0014, recv, 2, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x2008, recv, 2, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0900, 0x0074, recv, 2, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0900, 0x01b0, recv, 2, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x2008, recv, 2, 0);
		libusb_control_transfer(handle, 0xc0, 0xbc, 0x0800, 0x2008, recv, 2, 0);

		libusb_control_transfer(handle, 0x40, 0xbc, 0x0900, 0x0074, recv, 2, 0);
		libusb_control_transfer(handle, 0x40, 0xbc, 0x0900, 0x01b0, recv, 2, 0);

		FILE *idle_bin;

		idle_bin = fopen("firmware/mb86h57_h58_idle.bin", "rb");
		fread(data, sizeof(data), 1, idle_bin);
		libusb_bulk_transfer(handle, 0x02, data, sizeof(data), &transferred, 1000);
		fclose(idle_bin);

		libusb_release_interface(handle, 0);

		//if we detached kernel driver, reattach.
		if (attached == 1) {
			libusb_attach_kernel_driver( handle, 0 );
		}

		libusb_close(handle);
	}

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	return 0;
}
