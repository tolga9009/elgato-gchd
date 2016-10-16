/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef GCHD_H
#define GCHD_H

#include <array>
#include <cstdint>
#include <string>
#include <exception>
#include <vector>

#include <libusb-1.0/libusb.h>

#include "gchd/settings.hpp"
#include "process.hpp"
#include "gchd_hardware.hpp"
#include "utility.hpp"

// constants
#define DATA_BUF	0x4000 //Has to big enough for HDNew firmware transfers.
#define TIMEOUT		5000 // TODO measure and set more reasonable timeout

using std::runtime_error;
class usb_error: public runtime_error
{
		using runtime_error::runtime_error; //This inherits all constructors.
};

//TODO this is too complicated, needs to be factored into multiple
//classes.
class GCHD {
		friend class GCHDStateMachine;
	public:
		int checkDevice();
		int init();
		void stream(std::array<unsigned char, DATA_BUF> *buffer);
		GCHD(Process *process, Settings *settings);
		~GCHD();

	private:
		int libusb_;
		bool isInitialized_;
		std::string firmwareIdle_;
		std::string firmwareEnc_;
		struct libusb_device_handle *devh_;

		uint16_t savedEnableRegister_;
		uint16_t savedEnableStateRegister_;

		int checkFirmware();
		int openDevice(); //At USB level
		void closeDevice(); //At USB level
		int getInterface();
		void setupConfiguration();
		void configureDevice(); //At Device level
		void uninitDevice();    //At Device level
		void configureHDMI();
		void configureComponent();
		void configureColorSpace();

		void configureSetupSubblock();
		void configureCommonBlockA();
		void configureCommonBlockB1(bool mysteryFlag);
		void configureCommonBlockB2();
		void configureCommonBlockC();
		void readHdmiSignalInformation(unsigned &sum6463, unsigned &countSum6464,
					       unsigned &sum6665, unsigned &countSum6665,
					       bool &colorBit);
		void readComponentSignalInformation(unsigned &sum6867, unsigned &countSum6867,
						    unsigned &sum6665, unsigned &countSum6665);

		void read_config_buffer(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *buffer, uint16_t wLength);

		//This is a beautiful template, that returns a read of whatever integer value type you want...
		//You can do:
		//   myValue=read_config<uint32_t>( bRequest, wValue, wIndex );
		//and it will read 4 bytes in big endian order into the return value.
		//
		//You can also specify the length explicitly, independent of the data type
		//being stored. IE:
		//   myValue=read_config<uint32_t>( bRequest, wValue, wIndex, 3 );
		//would yield those 3 bytes in the bottom 3 bytes of myValue.
		//   myValue=read_config<uint32_t>( bRequest, wValue, wIndex, 100 );
		//Will still read 100 bytes over the usb, but only the first 4 will
		//be stored in myValue.
		//
		//It is recommended that you specify the type with <> when you use this
		//version. If you do not care about the return value, there is another
		//version where T is effectively void.
		template<typename T>
		T read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength=sizeof(T))
		{
			std::vector<unsigned char> recv=std::vector<unsigned char>(wLength);

			int returnSize=
					libusb_control_transfer(devh_, 0xc0, bRequest, wValue, wIndex, recv.data(), recv.size(), 0);

			if  (returnSize != wLength)
			{
				throw usb_error( "libusb_control_transfer did not return all bytes.\n" );
			}

			size_t size=std::min((size_t)wLength, sizeof(T));
			/* Big endian reconstruct */
			T value=Utility::debyteify<T>(recv.data(), size);
			return value;
		}

		//Other version, that reads vacuously to nowhere..
		void read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);

		void write_config_buffer( uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *buffer, uint16_t wLength);

		//This is a beautiful template, that does a write of whatever integer value type you want...
		//You can do:
		//   write_config<uint32_t>( bRequest, wValue, wIndex, data32 );
		//
		//This will write 4 bytes in big endian order into the return value.
		//data32 need not be a uint32_t if you make sure to use the <uint32_t> template
		//mark. It will get cast.
		//
		//You can also specify the length explicitly, independent of the data type
		//being stored. IE:
		//   write_config<uint32_t>( bRequest, wValue, wIndex, data32, 3 );
		//
		//This will write the 3 bytes stored in the bottom 3 bytes of data32.
		//the most significant byte would be chopped off.
		//
		//write_config<uint32_t>( bRequest, wValue, wIndex, data32, 100 );
		//Will write the 4 bytes of data32 out, and zero pad the buffer.
		template<typename T>
		void write_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, T value, uint16_t wLength=sizeof(T)) {
			std::vector<unsigned char> send=std::vector<unsigned char>(wLength);
			Utility::byteify<T>( send.data(), value, wLength );

			int returnSize=
					libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, send.data(), wLength, 0);

			if  (returnSize != wLength)
			{
				throw( usb_error( "libusb_control_transfer did not send all bytes.\n" ) );
			}
		}

		void interruptPend();
		void sendEnableState(); //saves and sends enable state to other device/processor.
		//Write savedEnableStateRegister_ to SEND_ENABLE_REGISTER_STATE
		// and waits for MAIL_REQUEST_READY

		void readEnableState(); //Reads SEND_ENABLE_EGISTER_STATE and saves it to
		//saveEnableStateRegister_

		void doEnable( uint16_t setMask, uint16_t valueMask ); //Set mask
		//has all bits that are being configured set.
		//Value mask has their actual value.

		void enableAnalogInput();
		void clearEnableStateBits( uint16_t clearMask );

		void scmd(uint8_t command, uint8_t mode, uint16_t data);

		void stateConfirmedScmd(uint8_t command, uint8_t mode, uint16_t data);
		void stateConfirmedScmd(uint8_t command, uint8_t mode, uint16_t data, uint16_t currentState);

		uint16_t completeStateChange( uint16_t currentState,
					      uint16_t nextState,
					      bool forceStreamEmpty=false );

		void sparam(uint16_t address, uint8_t lsb, uint8_t bits, uint16_t data);
		void sparam(const bitfield_t &bitField, uint16_t data);

		void slsi(uint16_t wIndex, uint16_t data);
		void transcoderTableWrite(uint16_t address, std::vector<uint8_t> &data);

		void mailReadyWait();

		void mailWrite( uint8_t port, const std::vector<unsigned char> &writeVector );

		std::vector<unsigned char> mailRead( uint8_t port, uint8_t size );

		void dlfirm(const char *file);
		uint8_t readDevice0x9DCD( uint8_t index );
		void pollOn0x9989ED();
		void readFrom0x9989EC( unsigned count );


		void stopStream( bool emptyBuffer );

		void clearEnableState();
		void readVersion( std::vector<unsigned char> &version );

		void transcoderWriteVideoAndAudioPids();
		void transcoderDefaultsInitialize();
		void transcoderSetup();
		void transcoderFinalConfigure();
		void transcoderOutputEnable(bool enable);

		DeviceType deviceType_;
		Process *process_;
		Settings *settings_;

		uint16_t specialDetectMask_;

		bool interlaced_;  //true or false
		unsigned refreshRate_; //50 or 60hz

};

#endif
