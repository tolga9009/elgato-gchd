/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* Abstract class used for reading and 
 * writing data in program specific information (PSI) as specified by
 * ISO/IEC 13818-1
 *
 * This file determines the data format of descriptors used in program
 * specific information.
 */

#include <vector>
#include <cstdint>
#include <algorithm>
#include "psi_descriptors.hpp"
#include "psi_exceptions.hpp"
#include "../utility.hpp"

int PSI_Descriptor::calculateSize(void)
{
    return 2+calculateSizeInternal();
}

//override this.
int PSI_Descriptor::calculateSizeInternal(void)
{
    return 0;
}

void PSI_Descriptor::pack(std::vector<uint8_t> &outputData,
                          std::vector<uint8_t>::iterator &offset)
{
    *offset++ = tag_;
    *offset++ = calculateSizeInternal();
    packInternal( outputData, offset );
}

//override this
void PSI_Descriptor::packInternal(std::vector<uint8_t> &outputData,
                                   std::vector<uint8_t>::iterator &offset)
{
}

void PSI_Descriptor::unpack(const std::vector<uint8_t> &inputData,
                            std::vector<uint8_t>::const_iterator &offset,
                            int size )
{
    if ( size < 2 ) {
        throw PSI_FormatException( "PSI Descriptor length mismatch." );
    }
    tag_ = *offset++;
    uint8_t length = *offset++;
    if ((size - 2) < length ) {
        throw PSI_FormatException( "PSI Descriptor length mismatch." );
    }
    this->unpackInternal( inputData, offset, length );
};

//Override this.
void PSI_Descriptor::unpackInternal(const std::vector<uint8_t> &inputData,
                                    std::vector<uint8_t>::const_iterator &offset,
                                    int size)
{
    //Do nothing by default.
}


void PSI_ParseDescriptor::unpackInternal(const std::vector<uint8_t> &inputData,
                                         std::vector<uint8_t>::const_iterator &offset,
                                         int size)
{
    switch( tag_ ) {
        case PROGRAM_DESCRIPTOR:
            this->parsedDescriptor_=std::make_shared<PSI_ProgramDescriptor>();
            break;
        case AVC_VIDEO_DESCRIPTOR:
            this->parsedDescriptor_=std::make_shared<PSI_AVC_VideoDescriptor>();
            break;
        case AVC_TIMING_AND_HRD_DESCRIPTOR:
            this->parsedDescriptor_=std::make_shared<PSI_AVC_TimingAndHRDDescriptor>();
            break;
        default:
            this->parsedDescriptor_=std::make_shared<PSI_UnknownDescriptor>( tag_ );
            break;
    }
    this->parsedDescriptor_->unpackInternal( inputData, offset, size );
}

std::shared_ptr<PSI_Descriptor> PSI_ParseDescriptor::getParsedDescriptor()
{
    return parsedDescriptor_;
}

PSI_UnknownDescriptor::PSI_UnknownDescriptor( uint8_t tag )
{
    this->tag_=tag;
    this->data=std::vector<uint8_t>();   
}

int PSI_UnknownDescriptor::calculateSizeInternal()
{
    return this->data.size();
}

void PSI_UnknownDescriptor::unpackInternal(const std::vector<uint8_t> &inputData,
                                           std::vector<uint8_t>::const_iterator &offset,
                                           int size)
{
    this->data.resize(size);
    std::copy_n( offset, size, this->data.begin() );
}

int PSI_ProgramDescriptor::calculateSizeInternal()
{
    return data_.size();
}

void PSI_ProgramDescriptor::packInternal(std::vector<uint8_t> &outputData,
                                         std::vector<uint8_t>::iterator &offset)
{
    std::copy_n(data_.begin(), data_.size(), offset);
    offset+=data_.size();
}

void PSI_ProgramDescriptor::unpackInternal(const std::vector<uint8_t> &inputData,
                                           std::vector<uint8_t>::const_iterator &offset, 
                                           int size ) 
{
    data_.resize(size);
    std::copy_n( offset, size, data_.begin() );
}

PSI_AVC_VideoDescriptor::PSI_AVC_VideoDescriptor( uint8_t profile_idc, 
                                                  uint8_t level_idc,
                                                  bool constraint_set0_flag,
                                                  bool constraint_set1_flag,
                                                  bool constraint_set2_flag,
                                                  bool constraint_set3_flag,
                                                  bool constraint_set4_flag,
                                                  bool constraint_set5_flag,
                                                  bool AVC_still_present,
                                                  bool AVC_24_hour_picture_flag,
                                                  bool frame_packing_SEI_not_present_flag )
{
    tag_=AVC_VIDEO_DESCRIPTOR;
    profile_idc_=profile_idc;
    constraint_set0_flag_=constraint_set0_flag;
    constraint_set1_flag_=constraint_set1_flag;
    constraint_set2_flag_=constraint_set2_flag;
    constraint_set3_flag_=constraint_set3_flag;
    constraint_set4_flag_=constraint_set4_flag;
    constraint_set5_flag_=constraint_set5_flag;
    AVC_compatible_flags_=0;
    level_idc_=level_idc;
    AVC_still_present_=AVC_still_present;
    AVC_24_hour_picture_flag_=AVC_24_hour_picture_flag;
    frame_packing_SEI_not_present_flag_ =frame_packing_SEI_not_present_flag;
}

int PSI_AVC_VideoDescriptor::calculateSizeInternal()
{
    return 4; //4 bytes to code everything.
}

void PSI_AVC_VideoDescriptor::packInternal(std::vector<uint8_t> &outputData,
                                           std::vector<uint8_t>::iterator &offset)
{
    *offset++=profile_idc_;

    if(AVC_compatible_flags_ >= 1<<2) {
        throw PSI_ValueException( "AVC_compatible_flags in AVC video descriptor must be 2 bits in length." );
    }
    uint8_t constraintFlags= (constraint_set0_flag_<<7) |
                             (constraint_set1_flag_<<6) |
                             (constraint_set2_flag_<<5) |
                             (constraint_set3_flag_<<4) |
                             (constraint_set4_flag_<<3) |
                             (constraint_set5_flag_<<2) |
                             AVC_compatible_flags_;
    *offset++=constraintFlags;

    *offset++=level_idc_;
    uint8_t miscellaneousFlags= (AVC_still_present_<<7) |
                                (AVC_24_hour_picture_flag_<<6) |
                                (frame_packing_SEI_not_present_flag_<<5) |
                                ((1<<5)-1); //Reserved bits

    *offset++=miscellaneousFlags;                        
}

void PSI_AVC_VideoDescriptor::unpackInternal(const std::vector<uint8_t> &inputData,
                                             std::vector<uint8_t>::const_iterator &offset, 
                                             int size ) 
{
    if( size < 4) {
        throw PSI_FormatException( "AVC video descriptor must be 4 bytes long." );
    }
    profile_idc_=*offset++;   
    uint8_t constraintFlags = *offset++;
    constraint_set0_flag_ = (constraintFlags>>7) & 1;
    constraint_set1_flag_ = (constraintFlags>>6) & 1;
    constraint_set2_flag_ = (constraintFlags>>5) & 1;
    constraint_set3_flag_ = (constraintFlags>>4) & 1;
    constraint_set4_flag_ = (constraintFlags>>3) & 1;
    constraint_set5_flag_ = (constraintFlags>>2) & 1;
    AVC_compatible_flags_ = constraintFlags & ((1<<2)-1);
    level_idc_=*offset++;   
    uint8_t miscellaneousFlags = *offset++;
    AVC_still_present_= (miscellaneousFlags>>7) & 1;
    AVC_24_hour_picture_flag_= (miscellaneousFlags>>6) & 1;
    frame_packing_SEI_not_present_flag_= (miscellaneousFlags>>5) & 1;
    uint8_t reserved = miscellaneousFlags & 0x1f;
    if( reserved != 0x1f ) {
        throw PSI_FormatException( "AVC video descriptor reserved bits not set correctly." );
    }
}

    //picture_and_timing_info_not present     
PSI_AVC_TimingAndHRDDescriptor::PSI_AVC_TimingAndHRDDescriptor
    ( bool hrd_management_valid_flag,
      bool fixed_frame_rate_flag,
      bool temporal_poc_flag,
      bool picture_to_display_conversion_flag )
{
    tag_=AVC_TIMING_AND_HRD_DESCRIPTOR;

    hrd_management_valid_flag_=hrd_management_valid_flag;
    fixed_frame_rate_flag_=fixed_frame_rate_flag;
    temporal_poc_flag_=temporal_poc_flag;
    picture_to_display_conversion_flag_=picture_to_display_conversion_flag;

    picture_and_timing_info_present_=false;
    kHz90_flag_=false;
    N_=0;
    K_=0;
    num_units_in_tick_=0;
}

int PSI_AVC_TimingAndHRDDescriptor::calculateSizeInternal()
{
    int length=2;
    if( picture_and_timing_info_present_ ) {
        length+=5;
        if( ! kHz90_flag_ ) {
            length+=8;
        }
    }
    return length;
}

void PSI_AVC_TimingAndHRDDescriptor::packInternal
    ( std::vector<uint8_t> &outputData,
      std::vector<uint8_t>::iterator &offset ) 
{
    uint8_t flags = (hrd_management_valid_flag_ << 7) |
                    (((1<<6)-1) << 1) | //reserved bits.
                    (picture_and_timing_info_present_);
    *offset++=flags;

    if( picture_and_timing_info_present_ ) {
        flags = (kHz90_flag_ << 7) |
                ((1<<7)-1); //reserved bits.

        *offset++=flags;
        if (!kHz90_flag_) {
            Utility::byteify<uint32_t>( &(*offset), N_ );
            offset+=4;
            Utility::byteify<uint32_t>( &(*offset), K_ );
            offset+=4;
        }
        Utility::byteify<uint32_t>( &(*offset), num_units_in_tick_ );
        offset+=4;
    }
    flags = (fixed_frame_rate_flag_ << 7 ) |
            (temporal_poc_flag_ << 6) |
            (picture_to_display_conversion_flag_ << 5) |
            ((1<<5)-1); //Reserved bits.
    *offset++=flags;
}

void PSI_AVC_TimingAndHRDDescriptor::unpackInternal
    ( const std::vector<uint8_t> &inputData,
      std::vector<uint8_t>::const_iterator &offset, 
      int size ) 
{
    if (size < 2) {
        throw PSI_FormatException( "AVC timing and HRD descriptor contents must be at least 2 bytes long." );
    }
    uint8_t flags= *offset++;
    hrd_management_valid_flag_ = (flags >> 7) & 1;
    picture_and_timing_info_present_ = (flags) & 1;
    if (((flags >> 1) & ((1<<6)-1)) != ((1<<6)-1)) {
        throw PSI_FormatException( "AVC timing and HRD descriptor reserved bits not set correctly." );
    }
    
    if( picture_and_timing_info_present_ ) {
        if (size < 7) {
            throw PSI_FormatException( "AVC timing and HRD descriptor contents must be at least 7 bytes long with timing info present." );
        }
        flags= *offset++;
        kHz90_flag_ = (flags >> 7) & 1;
        if ((flags & ((1<<7)-1)) != ((1<<7)-1)) {
            throw PSI_FormatException( "AVC timing and HRD descriptor reserved bits not set correctly." );
        }
        if(!kHz90_flag_) {
            if (size < 15) {
               throw PSI_FormatException( "AVC timing and HRD descriptor must be at least 15 with custom clock configuration." );
            }
            N_ = Utility::debyteify<uint32_t>( &(*offset) );
            offset+=4;  
            K_ = Utility::debyteify<uint32_t>( &(*offset) );
            offset+=4;  
        }
        else {
            N_ = 1;
            K_ = 300;
        }
        num_units_in_tick_ = Utility::debyteify<uint32_t>( &(*offset) );
        offset+=4;  
    }
    else {
        kHz90_flag_=false;
        N_=0;
        K_=0;
        num_units_in_tick_=0;
    }
    flags = *offset++;

    fixed_frame_rate_flag_ = (flags >> 7) & 1;
    temporal_poc_flag_ = (flags >> 6) & 1;
    picture_to_display_conversion_flag_ = (flags >> 5) & 1;
    if ((flags & ((1<<5)-1)) != ((1<<5)-1)) {
        throw PSI_FormatException( "AVC timing and HRD descriptor reserved bits not set correctly." );
    }
}

int PSI_HDMV_CopyControlDescriptor::calculateSizeInternal() 
{
    return 4;
}

void PSI_HDMV_CopyControlDescriptor::packInternal(std::vector<uint8_t> &outputData,
                                                  std::vector<uint8_t>::iterator &offset)
{
    *offset++=0x0f;
    *offset++=0xff;
    *offset++=0xfc;
    *offset++=0xfc;
}

void PSI_HDMV_CopyControlDescriptor::unpackInternal(const std::vector<uint8_t> &inputData,
                                                    std::vector<uint8_t>::const_iterator &offset, 
                                                    int size )
{
    if( size < 4) {
        throw PSI_FormatException( "HDMV copy control descriptor is invalid.");
    }
    uint32_t value=Utility::debyteify<uint32_t>(&(*offset));
    offset+=4;
    if(value != 0x0ffffcfc) {
        throw PSI_FormatException( "HDMV copy control descriptor is not set to default.");
    }
}


