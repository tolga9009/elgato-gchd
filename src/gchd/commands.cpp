/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <cstdio>
#include <vector>

#include "../utility.hpp"
#include "../gchd.hpp"
#include "../gchd_hardware.hpp"

void GCHD::read_config_buffer( uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *buffer, uint16_t readSize) {
    uint16_t wLength=readSize;
	int returnSize=
        libusb_control_transfer(devh_, 0xc0, bRequest, wValue, wIndex, buffer, wLength, 0);

    if (returnSize != wLength)
    {
        throw( usb_error( "libusb_control_transfer did not return all bytes.\n" ) );
    }
}

//This version ignores the result
void GCHD::read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
	std::vector<unsigned char> recv(wLength);

	int returnSize=
        libusb_control_transfer(devh_, 0xc0, bRequest, wValue, wIndex, recv.data(), static_cast<uint16_t>(recv.size()), 0);

    if (returnSize != wLength)
    {   
        throw( usb_error( "libusb_control_transfer did not return all bytes.\n" ) );
    }
}

void GCHD::write_config_buffer( uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *buffer, uint16_t wLength) {

	int returnSize=
        libusb_control_transfer(devh_, 0x40, bRequest, wValue, wIndex, buffer, wLength, 0);

    if (returnSize != wLength)
    {   
        throw( usb_error( "libusb_control_transfer did not write all bytes.\n" ) );
    }
}

void GCHD::interruptPend()
{
	unsigned char input[3];
    int returnSize;
	int status=
        libusb_interrupt_transfer(devh_, 0x83, input, 3, &returnSize, 0); 
    if( status != 0 )
    {
        throw usb_error("USB error when pending on USB interrupt.\n");
    }
}

void GCHD::sendEnableState()
{
    uint16_t status;
    do {
        write_config<uint16_t>( MAIL_SEND_ENABLE_REGISTER_STATE, savedEnableStateRegister_ );
        status = read_config<uint16_t>(MAIL_REQUEST_READY);
        specialDetectMask_ &= status;
    } while ( (status & 1) == 0);
}

/* Doesn't return value to emphasize that something in object is being changed */
void GCHD::readEnableState()
{   
    savedEnableStateRegister_ = read_config<uint16_t>(MAIL_SEND_ENABLE_REGISTER_STATE); 
}

//This enables what I expect are GPIOs in register ENABLE_REGISTER
void GCHD::doEnable( uint16_t setMask, uint16_t valueMask ) //Set mask
                                                            //has all bits that are being configured set.
                                                            //Value mask has their actual value.
{
    if (~setMask & valueMask)
    {
        throw std::logic_error( "Bits in valueMask set that aren't set in setMask.");
    }
    savedEnableStateRegister_ |= setMask;
    savedEnableRegister_ &= ~setMask;
    savedEnableRegister_ |= valueMask;

    write_config<uint16_t>( MAIL_SEND_ENABLE_REGISTER_STATE, savedEnableStateRegister_ );
    write_config<uint16_t>( ENABLE_REGISTER, savedEnableRegister_ );
    savedEnableStateRegister_ = read_config<uint16_t>( MAIL_SEND_ENABLE_REGISTER_STATE );    
    savedEnableRegister_ = read_config<uint16_t>( ENABLE_REGISTER );    
}

void GCHD::enableAnalogInput() 
{
    //Configure input choice from settings.
    //In original windows driver this might have changed on the fly.
    bool analog = settings_->getInputSource() != InputSource::HDMI;
    uint16_t analogInputBit = analog ? EB_ANALOG_INPUT : 0;
    
    //First is mask of what we are configuring, second is values.
     doEnable( EB_ANALOG_INPUT, analogInputBit );
}

//Anything set in clearMask is cleared.
void GCHD::clearEnableStateBits( uint16_t clearMask ) {
    savedEnableStateRegister_ &= ~clearMask;
    sendEnableState();
}

/*
 * Reverse engineered function from official drivers. Used to set device states
 * and modes. scmd() is the short form for system command.
 *
 * @param command there are currently three commands, we have *identified*.
 *  1: SCMD_IDLE
 *     This is used to enter idle state. This also forces our effective
 *     state to SCMD_STATE_STOP in terms of what we read back in the lower 
 *     nybble of SCMD_STATE_READBACK_REGISTER
 *
 *  2: SCMD_RESET
 *     This is believe to reset  the firmware state, and is typically done in two
 *     cases:
 *      a) Bringing up a board that already has its idle flash loaded, after skipping the load.
 *      b) During device uninitialization, it is the last thing done.
 *
 *     The mode parameter is used with this. Mode is set to 1 or 0
 *     probably signifying to what level the state is reset. 
 *       mode=0 is used for case a
 *       mode=1 is used for case b
 *
 *  4: SCMD_INIT
 *     Directly after the transcode settings are configured, we
 *     always send an init. This can be done as long as our state
 *     (as determined by reading SCMD_STATE_READBACK_REGISTER)
 *     is SCMD_STATE_STOP. It doesn't matter if we are currently idle or not.
 *
 *     The mode parameter is used with this. 
 *        mode=0x00 means that this also kicks of an load of the encode firmware
 *            and an interrupt will be generated.
 *        mode=0xa0 means no firmware load is done, and no interrupt
 *            will be generated.
 *
 *  5: SCMD_STATE_CHANGE
 *     This sets the devices encoding state, IE, SCMD_STATE_STOP, SCMD_STATE_START
 *     amd SCMD_STATE_NULL. 
 *      
 * @param mode This is used with SCMD_INIT. 0x0 loads firmware, 0xa0 does
 *    not.  It is also used with SCMD_RESET to set type of reset.
 *
 * @param data applies to send[4] and send[5]. The 16-bit integer data needs to
 *   be split up into two 8-bit integers. It holds data to further specify
 *   the command parameter. In conjunction with STATE_CHANGE, it is used to set
 *   specific encoding states: 1 means SCMD_STOP, 2 means SCMD_START,
 *   and 4 means SCMD_NULL.
 *   Setting STATE_CHANGE to NULL will make the device output an empty data
 *   stream during the encoding process.
 */
void GCHD::scmd(uint8_t command, uint8_t mode, uint16_t data) {
	uint8_t send[6] = {0};
	send[2] = command;
	send[3] = mode;

	// splitting up data to two 8-bit integers by bitshifting and masking
	send[4] = data >> 8;
	send[5] = data & 0xff;

    if(deviceType_ == DeviceType::GameCaptureHD) 
    {
        write_config_buffer(SCMD_REGISTER, send, 6);
    }
    else //GameCaptureHdNew
    {
        write_config_buffer(SCMD_REGISTER, send+2, 4);
        if(( command == SCMD_IDLE ) || ( command == SCMD_STATE_CHANGE ) || (command == SCMD_INIT))
        {
            if((mode & 0xa0) != 0xa0 ) //One of these two bits I think disables interrupts, at
                                        //least for SCMD_INITs after 1st.
            {
                interruptPend();
                uint16_t value;
                do
                {
                    value=read_config<uint16_t>(HDNEW_SCMD_READBACK_REGISTER);
                } while ((value >> 8) != command); //TODO! Probably should set timeout
            }
        }
    }
}

uint16_t GCHD::completeStateChange( uint16_t currentState,
                                    uint16_t nextState,
                                    bool forceStreamEmpty )
{
    bool firstTime=true;
    uint16_t state;
    bool changed=false;
    do
    {
        do
        {

            state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); 

            state &= 0x1f; //Masking off bits we don't ever see. 
            if((state != currentState) && (state != nextState))
            {
                if( firstTime ) {   
                    return state;
                } else {
                    throw std::runtime_error( "Device transitioned to unexpected state.");
                }
            }
            firstTime=false;

            //forceStreamEmpty is a terrible hack that needs to die.
            if( forceStreamEmpty )
            {
                std::array<unsigned char, DATA_BUF> buffer;
                //Streaming happens at top of loop, and after each 0x01b0 command, this seems 
                //proper place to put it.
                for (int i = 0; i < 50; i++)
                {
                    stream(&buffer);
                }
            }
            uint16_t completion=read_config<uint16_t>(SCMD_STATE_CHANGE_COMPLETE);  
            changed = (completion & 0x4)>0; //Check appropriate bit


            read_config<uint16_t>(0xbc, 0x0900, 0x01b0); //Not sure what to do with this if anything
        } while(not changed);
        
        //Double read here for unknown purposes. 
        state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); 
        state=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER); 
        state &= 0x1f;

        //Reset sticky bit and possibly acknowledgement.
        write_config<uint16_t>(SCMD_STATE_CHANGE_COMPLETE, 0x0004);
        write_config<uint16_t>(0xbc, 0x0900, 0x01b0, 0x0000); 
    } while ( state != nextState );
    
    return state;
}

void GCHD::stateConfirmedScmd(uint8_t command,
                              uint8_t mode,
                              uint16_t data,
                              uint16_t currentState )

{
    uint16_t expectedState;
    scmd(command, mode, data);
    
    switch(command) {
        case SCMD_IDLE:
            expectedState=0x11; //IDLE + STOPPED
            break;

        case SCMD_STATE_CHANGE:
            expectedState= data & 0xf; //expected state = mode;
            break;

        case SCMD_RESET:
            expectedState=0x12; //Special state for in explicitly resetted firmware state.
            break;

        default:
            throw std::logic_error( "stateConfirmedScmd used with illegal scmd.");
            break;
    }

    currentState=completeStateChange( currentState, expectedState);
    if(currentState != expectedState) {
        throw std::runtime_error("Failure changing states.");
    }
}

void GCHD::stateConfirmedScmd(uint8_t command,
                              uint8_t mode,
                              uint16_t data)
{
    uint16_t currentState=read_config<uint16_t>(SCMD_STATE_READBACK_REGISTER) & 0x1f;
    stateConfirmedScmd( command, mode, data, currentState );
}

/** 
 * Reverse engineered function used by official drivers.
 * The data is sent out via an 8 byte value.  
 * Ultimately we want:
 *    libusb_control_transfer( dev_h, SETUP_H264_TRANSCODER, address, send, 8, 0);
 * Where buffer is an eight byte field divided into two parts, a 4 byte value, 
 * followed by a 4 byte mask.  The processor on the other side makes sure that only 
 * the bits in the mask get set to the same as the same bits in value.
 * 
 * address is the address of a register, but it must be divisible by 4, such that things
 * are always done on 32 bit word boundary.
 *
 * That said, logically the device appears to use 16 bit registers, so it
 * is nice to have a convenience function:
 *     sparam( address, lsb, bitLength, data );
 *
 * This takes data and writes it to address (which only has to be 16 bit word
 *  aligned). The data is shifted left such that its least significant is at
 * the position specified by lsb. And then we specify the length of the data
 * field in bitLength, so we can generate the mask so only those bits are 
 * modified.
 */
void GCHD::sparam(uint16_t address, uint8_t lsb, uint8_t bits, uint16_t data) {
    uint32_t outputData=(uint32_t)data;
    uint32_t outputMask=(1<<bits)-1; //yes it is that easy to create a right
                                     //aligned mask that is bitLength long.
                                     //Don't ever pass in 0 though. ;)
    //So now outputData and outputMask are set up, but they both need to be
    //shifted into position.
    unsigned int shift=lsb;
    
    //If the bit 1 of address is clear,
    //We will be righting to upper half of 32 bit register.
    if((address & 2)==0)
    {
        shift+=16; 
    }
    uint16_t outputAddress= address & ~3; //Clear lower 2 bits. bit 0 better not be set
                                         //anyways. Now we have 4 byte address.

    //Shift the data and the mask into position.
    outputData<<=shift;
    outputMask<<=shift;


    //Prepare 8 byte send buffer....
    unsigned char send[8];
    //Do big endian conversions of 32 bit into there.
    Utility::byteify<uint32_t>( send, outputData );
    Utility::byteify<uint32_t>( send+4, outputMask );

    
    int returnSize =
	    libusb_control_transfer(devh_, 0x40, SEND_H264_TRANSCODER_BITFIELD, outputAddress, send, 8, 0);

    if(returnSize < 8 )
    {
        throw usb_error("USB error when sending sparam.\n");
    }
}    

//Polymorphic one that takes struct
void GCHD::sparam(const bitfield_t &bitfield, uint16_t data) {
    sparam(bitfield.address,
           bitfield.lsb,
           bitfield.bits, data);
}

/**
 * Reverse engineered function from official drivers. Used to configure the
 * device.
 *
 * @param wIndex offset, where requests are passed to.
 * @param data applies to send[0] and send[1]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds the actual data, like video
 *  vertical size and bitrate.
 */
void GCHD::slsi(uint16_t address, uint16_t data) {
	uint8_t send[2] = {0};

	// splitting up data to two 8-bit integers by bitshifting and masking
    Utility::byteify<uint16_t>(send, data);

    int returnSize =
    	libusb_control_transfer(devh_, 0x40, SEND_H264_TRANSCODER_WORD, address, send, 2, 0);
    
    if(returnSize != 2 )
    {
        throw usb_error("USB error in SLSI.\n");
    }
}

//
//This writes a vector of values out to the 16 bit slsi writable
//registers at address. 
//
//Since we don't know the end range of register blocks
//that use this function, we do no error checking.
//
void GCHD::transcoderTableWrite(uint16_t address, std::vector<uint8_t> &data) 
{
    auto offset=data.begin();
    int leftSize=data.size();
    while( leftSize >= 2 ) {
        uint16_t data=Utility::debyteify<uint16_t>(&(*offset));
        slsi( address, data ); 
        offset+=2;
        leftSize-=2;
        address+=2;
    }
    if( leftSize == 1 ) {
        uint16_t data = *(offset);
        slsi( address, data<<8 );
    }
}

void GCHD::mailReadyWait()
{
    uint16_t status;
    do {
        status=read_config<uint16_t>(MAIL_REQUEST_READY);
        specialDetectMask_ &= status;
    } while( (status & 1) == 0 );
}

void GCHD::mailWrite( uint8_t port, const std::vector<unsigned char> &writeVector )
{
    if( deviceType_ == DeviceType::GameCaptureHD )
    {
        write_config_buffer( HD_MAIL_REGISTER, port << 8, (unsigned char *)writeVector.data(), writeVector.size() );
        sendEnableState();
    }
    else if( deviceType_ == DeviceType::GameCaptureHDNew )
    {
        std::vector<unsigned char> paddedBuffer=writeVector;

        if( writeVector.size() & 1) 
        {
            //Writes are padded at end to even boundary.
            //Pad by one byte.
            paddedBuffer.insert(paddedBuffer.end(), 0);
        }
        write_config_buffer( HDNEW_MAIL_WRITE, paddedBuffer.data(), paddedBuffer.size() );

        unsigned char mailRequestBuffer[4]={0x09, 0x00, 0x0, 0x0};
        mailRequestBuffer[2]=port << 1;
        mailRequestBuffer[3]=writeVector.size(); //Original size before padding.

        write_config_buffer( HDNEW_MAIL_REQUEST_CONFIGURE, mailRequestBuffer, 4 );
        interruptPend();
        
        read_config( HDNEW_INTERRUPT_STATUS, 2 ); //EXPECTED=0x09, 0x00
                                                  //universally
        mailReadyWait();

        read_config( HDNEW_MAIL_READ, 2 ); //Read to nowhere, dummy bytes.
                                           //may have status of write
                                           //possibly, but would have no
                                           //idea what to do with that in reverse
                                           //engineered driver.

        write_config<uint16_t>( MAIL_SEND_ENABLE_REGISTER_STATE, savedEnableStateRegister_ );
        mailReadyWait();
    }
    else
    {
        throw std::logic_error( "Unsupported device.");
    }
}

/* Yes, returning a vector is a bit messy, but what hurts in terms of
 * what is going on under the hood really doesn't matter as much
 * as convenience and readability.
 */
std::vector<unsigned char> GCHD::mailRead( uint8_t port,
                                           uint8_t size )
{
    std::vector<unsigned char> input;
    if( deviceType_ == DeviceType::GameCaptureHD )
    {
        input.resize(size);
        read_config_buffer( HD_MAIL_REGISTER, port, input.data(), size );
    }
    else if( deviceType_ == DeviceType::GameCaptureHDNew )
    {
        unsigned char mailRequestBuffer[4]={0x09, 0x01, 0x0, 0x0};
        mailRequestBuffer[2]=port<<1;
        mailRequestBuffer[3]=size;
        write_config_buffer( HDNEW_MAIL_REQUEST_CONFIGURE, mailRequestBuffer, 4 );       

        interruptPend();
        
        read_config( HDNEW_INTERRUPT_STATUS, 2 ); //EXPECTED=0x09, 0x00
                                                           //universally
        mailReadyWait();

        int readSize=size+2 + (size & 1); //2 extra bytes of 
                                          //leader+ 1 byte padding if odd.

        input.resize(readSize);
        read_config_buffer( HDNEW_MAIL_READ, input.data(), readSize ); //Read to nowhere, dummy bytes.
                                           //may have status of write
                                           //possibly, but would have no
                                           //idea what to do with that in reverse
                                           //engineered driver.
        input.erase( input.begin(), input.begin() +2); //Erase 2 front padding bytes.
        input.resize( size ); //get rid of possible 0 byte at end for odd sized reads.
    }
    else
    {
        throw std::logic_error( "Unsupported device.");
    }
    return input;
}

/**
 * Loads firmware to the device
 *
 * @param file path to binary firmware file
 */
void GCHD::dlfirm(const char *file) {
	int transfer;

	FILE *bin;
	bin = fopen(file, "rb");

	// get filesize
	fseek(bin, 0L, SEEK_END);
	auto filesize = ftell(bin);
	rewind(bin);

    //Buffersizes must be divisble by 16.
    unsigned bufferSize=16384;
    if( deviceType_ == DeviceType::GameCaptureHDNew ) {
        bufferSize=32768; //Must use bigger chunks.
    }
    
	// read firmware from file to buffer and bulk transfer to device
	for (auto i = 0; i <= filesize; i += bufferSize) {
		std::vector<unsigned char> buffer=std::vector<unsigned char>(bufferSize);
		auto bytes_remain = filesize - i;

		if (bytes_remain > bufferSize) {
			bytes_remain = bufferSize;
		}

		fread(buffer.data(), static_cast<unsigned long>(bytes_remain), 1, bin);
		libusb_bulk_transfer(devh_, EP_OUT, buffer.data(), static_cast<int>(bytes_remain), &transfer, 0);
	}
	fclose(bin);
}

//Reads from unknown device. Done so often, made it a subroutine
uint8_t GCHD::readDevice0x9DCD( uint8_t index )
{
    std::vector<unsigned char> input=VC{ 0x9d, 0xcd, 0x00 };
    input[2] = index;
    mailWrite( 0x33, input );
    return mailRead( 0x33, 1 )[0];
}

//Reads from unknown device. Done so often, made it a subroutine
void GCHD::pollOn0x9989ED()
{   
    uint8_t input;
    do
    {
        mailWrite( 0x33, VC{ 0x99, 0x89, 0xed } );
        input=mailRead( 0x33, 1 )[0];
        //input will be 2e, till it changes to 0xea.
    } while( input != 0xea );
}

void GCHD::readFrom0x9989EC( unsigned count )
{
    for(unsigned i=0; i<count; ++i) {
        mailWrite( 0x33, VC{ 0x99, 0x89, 0xec } );
        mailRead( 0x33, 1 ); //Currently don't put result anywhere.
    }
}

//empty buffer is set to true when we want to poll
//and dump bulk transfers which are usually read in separate
//thread.
//TODO--get rid of that flag.
void GCHD::stopStream( bool emptyBuffer ) 
{
	// state change - output null
	scmd(SCMD_STATE_CHANGE, 0x00, SCMD_STATE_NULL);

	/*
	 * usually, this is done by a separate thread, which runs in parallel.
	 * I'm just quickly hacking a way around it, to test some stuff. Taking
	 * a big, random number, which is slightly based on the USB traffic logs
	 * I'm working with. Receive empty data, after setting state change to
	 * null transfer.
	 */
	std::array<unsigned char, DATA_BUF> buffer;

    if( emptyBuffer )
    {
	    for (int i = 0; i < 5000; i++) {
		    stream(&buffer);
	    }
    }
    completeStateChange(  SCMD_STATE_START, SCMD_STATE_NULL );

	// state change - stop encoding
	scmd(SCMD_STATE_CHANGE, 0x00, SCMD_STATE_STOP); 
    completeStateChange(  SCMD_STATE_NULL, SCMD_STATE_STOP );
}

void GCHD::clearEnableState()
{
    clearEnableStateBits(EB_ENCODER_ENABLE);
    clearEnableStateBits(EB_ENCODER_TRIGGER);
    clearEnableStateBits(EB_COMPOSITE_MUX);
    clearEnableStateBits(EB_ANALOG_MUX);

    //This apparently gets processor state........
    //Don't know what to do with it.  Could confirm it.
    //Not sure what the driver does with it.
    mailWrite( 0x33, VC{0xab, 0xa9, 0x0f, 0xa4, 0x55} );
    std::vector<unsigned char> read=mailRead( 0x33, 3 ); //EXPECTED {0x27, 0xf9, 0x7b};

    doEnable(EB_FIRMWARE_PROCESSOR, 0x0); //Shut down processor.

    //And we grab it again after turning it off.
    //This time we poll on it till it is ready, just in case.
    uint32_t result;
    do
    {
        mailWrite( 0x33, VC{0xab, 0xa9, 0x0f, 0xa4, 0x55} );
        read=mailRead( 0x33, 3 ); //EXPECTED {0x33, 0x44, 0x55};
        result=Utility::debyteify<uint32_t>(read.data(),3); 
    } while(result != 0x334455); 
}

//Read firmware version information.
void GCHD::readVersion( std::vector<unsigned char> &version ) 
{
    uint32_t value0=read_config<uint32_t>(HDNEW_VERSION_REGISTER0);
    uint32_t value1=read_config<uint32_t>(HDNEW_VERSION_REGISTER1);
    unsigned char temporary[4];

    version.resize(10);  //V+8 digits + null terminator

    if (( value0 | value1 )==0) //On HD device, not HDNew
    {
	    read_config_buffer(HD_VERSION_REGISTER0, version.data(), 4); 
	    read_config_buffer(HD_VERSION_REGISTER1, version.data()+4, 4); 
	    read_config_buffer(HD_VERSION_REGISTER2, temporary, 4); 
        *(version.data()+4)=temporary[0];
    }
    else
    {
	    read_config_buffer(HDNEW_VERSION_REGISTER0, version.data(), 4); 
	    read_config_buffer(HDNEW_VERSION_REGISTER1, version.data()+4, 4); 
	    read_config_buffer(HDNEW_VERSION_REGISTER2, temporary, 4); 
        *(version.data()+4)=temporary[0];
    }

    //Null terminate.
    version[9]=0;
}


