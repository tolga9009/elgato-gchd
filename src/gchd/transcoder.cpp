/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <vector>
#include <algorithm> //For min
#include "../gchd.hpp"
#include "psi_sit.hpp"
#include "psi_pat.hpp"
#include "psi_pmt.hpp"


//Define some implementation specific constants.
//In some remote future these may change (doubtful)
namespace Transcoder
{
	//Assign packet ID numbers
	const uint16_t videoPID=0x1011;
	const uint16_t audioPID=0x10f;

	const uint16_t pmtPID=0x110;
	const uint16_t sitPID=0x1f;
	const uint16_t pcrPID=0x100;
	const uint16_t thumbnailPID=0x1111;

	//Assign Stream ID numbers.
	//https://en.wikipedia.org/wiki/Packetized_elementary_stream
	const uint16_t videoSID=0xE0; //MPEG Video stream #0
	const uint16_t audioSID=0xC0; //MPEG Audio stream #0
}

using namespace Transcoder;
void GCHD::transcoderWriteVideoAndAudioPids()
{
	sparam( v_pid_out, videoPID );
	sparam( v_pid_out_ts_out2, videoPID );
	sparam( a_pid_out, audioPID );
	sparam( a_pid_out_ts_out2, audioPID );
}

//Here we set up parameters to their defaults. This is done the same for all encodings.
using namespace Transcoder;
void GCHD::transcoderDefaultsInitialize()
{
	write_config<uint16_t>(BANKSEL, 0x0000);
	sparam( v_vinpelclk, 0 );
	sparam( tbc_mode, 1 );
	sparam( stout_mode, 0 );
	sparam( extra_info_bit, 0 );
	sparam( tsout_packet_size, 0 );
	sparam( stoutclk_hl, 0 );
	sparam( dma_sel_out, 1 );
	sparam( stoutclk_io, 0 );
	sparam( stoutclk, 4 );
	sparam( stout2_mode, 0 ); //8 bit parallel
	sparam( stout2clk_hl, 0 );
	sparam( stout2clk_io, 0 );
	sparam( pcr_first, 0 );
	sparam( pcr_first_ts_out2, 0 );
	sparam( auto_null, 2 );
	sparam( auto_null_ts_out2, 2 );
	sparam( system_rate, 25000 );
	sparam( system_rate_ts_out2, 8000 );
	sparam( system_min_rate, 0 );
	sparam( system_min_rate_ts_out2, 0 );
	sparam( initial_time_stamp_h, 0 );
	sparam( initial_time_stamp_l, 0 );
	sparam( initial_time_stamp_h_ts_out2, 0 );
	sparam( initial_time_stamp_l_ts_out2, 0 );
	sparam( initial_stc_h, 0 );
	sparam( initial_stc_l, 0 );
	sparam( initial_stc_ll, 0 );
	sparam( initial_stc_ext, 0 );
	sparam( initial_stc_h_ts_out2, 0 );
	sparam( initial_stc_l_ts_out2, 0 );
	sparam( initial_stc_ll_ts_out2, 0 );
	sparam( initial_stc_ext_ts_out2, 0 );

	transcoderWriteVideoAndAudioPids();

	sparam( sit_pid_out, sitPID );
	sparam( sit_pid_out_ts_out2, sitPID );
	sparam( pmt_pid_out, pmtPID );
	sparam( pmt_pid_out_ts_out2, pmtPID );
	sparam( pcr_pid_out, pcrPID );
	sparam( pcr_pid_out_ts_out2, pcrPID );
	sparam( thumbnail_pid_out_ts_out2, thumbnailPID );
	sparam( v_sid_out, videoSID );
	sparam( v_sid_out_ts_out2, videoSID );
	sparam( pat_cyc, 0 );
	sparam( pat_cyc_ts_out2, 0 );
	sparam( pmt_cyc, 0 );
	sparam( pmt_cyc_ts_out2, 0 );
	sparam( pcr_cyc, 0 );
	sparam( pcr_cyc_ts_out2, 0 );
	sparam( sit_cyc, 0 );
	sparam( sit_cyc_ts_out2, 0 );
	uint16_t transportStreamIdentifier=0x0;

	//Generate the PAT
	std::vector<uint8_t> patVector;
	uint16_t pmtProgramNumber=0x01;
	uint16_t sitProgramNumber=0x00;
	PAT pat=PAT(transportStreamIdentifier);
	pat.addEntry( PAT_Entry( pmtProgramNumber, pmtPID ) );
	pat.addEntry( PAT_Entry( sitProgramNumber, sitPID ) );
	pat.bytes( patVector );

	//Write the PAT out
	sparam( pat_length, patVector.size() );
	transcoderTableWrite( pat_table_start, patVector );

	//Write the PAT out TS_OUT2
	sparam( pat_length_ts_out2, patVector.size() );
	transcoderTableWrite( pat_table_ts_out2_start, patVector );

	//Generate the SIT
	std::vector<uint8_t> sitVector;
	SIT sit=SIT(); //Default SIT, no descriptors.
	sit.bytes( sitVector );

	//Write the SIT out
	sparam( sit_length, sitVector.size() );
	transcoderTableWrite( sit_table_start, sitVector );

	//Write the SIT out TS_OUT2
	sparam( sit_length_ts_out2, sitVector.size() );
	transcoderTableWrite( sit_table_ts_out2_start, sitVector );

	//Generate the PMT
	std::vector<uint8_t> pmtVector;
	PMT pmt=PMT(pmtProgramNumber, pcrPID);
	pmt.addProgramInfo(std::make_shared<PSI_HDMV_ShortDescriptor>());
	pmt.addProgramInfo(std::make_shared<PSI_HDMV_CopyControlDescriptor>());

	//PMT VIDEO STREAM
	PMT_Mapping mapping=PMT_Mapping(videoPID, STREAM_TYPE_H264);
	mapping.addDescriptor(std::make_shared<PSI_HDMV_LongDescriptor>());
	uint8_t profile_idc=0x64;
	uint8_t level_idc=0x28;
	auto videoDescriptor =
			std::make_shared<PSI_AVC_VideoDescriptor>(profile_idc, level_idc);
	mapping.addDescriptor(videoDescriptor);

	auto timingDescriptor =
			std::make_shared<PSI_AVC_TimingAndHRDDescriptor>(false, true, true, true);
	mapping.addDescriptor( timingDescriptor );

	pmt.addMapEntry(mapping);

	//AUDIO
	mapping=PMT_Mapping(0x10f, STREAM_TYPE_MPEG1_AUDIO);
	pmt.addMapEntry(mapping);

	//Get bytes into pmtVector
	pmt.bytes( pmtVector );

	sparam( a_ts_out_mode, 2 );
	sparam( a_ts_out2_mode, 2 );
	sparam( a_mode, 3 );
	sparam( a_sid_out, audioSID );

	//Write the PMT out
	sparam( pmt_length, pmtVector.size() );
	transcoderTableWrite( pmt_table_start, pmtVector );

	sparam( a_ts_out2_mode, 0 );
	sparam( a2_mode, 0 );
	sparam( a_sid_out_ts_out2, audioSID );

	sparam( pmt_length_ts_out2, pmtVector.size() );
	transcoderTableWrite( pmt_table_ts_out2_start, pmtVector );

	sparam( v_v420, 1 );
	slsi( 0x1710, 0x00fc );
	slsi( 0x1712, 0x0024 );
	slsi( 0x1714, 0x00f8 );
	slsi( 0x1716, 0x00e8 );
	slsi( 0x1718, 0x00da );
	slsi( 0x171a, 0x00c6 );
	slsi( 0x171c, 0x0076 );
	slsi( 0x171e, 0x00ea );
	sparam( vin_swap, 1 );
	sparam( v_format, 4 );
	sparam( v_ip_format, 0 );
	sparam( tfrff, 2 );
	sparam( v_eos, 0 );
	sparam( v_eos_bl, 0 );
	sparam( v_cl_gop, 0 );
	sparam( v_cl_gop_bl, 0 );
	sparam( v_temporal_direct_on, 1 );
	sparam( v_vlc_mode, 0 );
	sparam( v_vlc_mode_bl, 1 );
	sparam( v_filler, 0 );
	sparam( v_filler_bl, 0 );
	sparam( v_rate_mode, 1 );
	sparam( v_rate_mode_bl, 1 );
	sparam( v_gop_size, 15 );
	sparam( v_gop_size_bl, 15 );
	sparam( v_gop_struct, 0 );
	sparam( v_gop_struct_bl, 1 );
	sparam( v_idr_interval_h, 0 );
	sparam( v_idr_interval_l, 0 );
	sparam( v_idr_interval_h_bl, 0 );
	sparam( v_idr_interval_l_bl, 0 );
	sparam( v_h264_profile, 1 );
	sparam( v_h264_level, 41 );
	sparam( v_hsize_out, 720 );
	sparam( v_vsize_out, 480 );
	sparam( v_h264_profile_bl, 3 );
	sparam( v_h264_level_bl, 12 );
	sparam( v_hsize_out_bl, 320 );
	sparam( v_vsize_out_bl, 240 );
	sparam( v_bitrate, 16000 );
	sparam( v_max_bitrate, 14000 );
	sparam( v_ave_bitrate, 8000 );
	sparam( v_min_bitrate, 0 );
	sparam( v_bitrate_bl, 10000 );
	sparam( v_max_bitrate_bl, 6000 );
	sparam( v_ave_bitrate_bl, 1800 );
	sparam( v_min_bitrate_bl, 0 );
	sparam( error_rate_ipic, 0 );
	sparam( error_rate_ppic, 0 );
	sparam( error_rate_refpic, 0 );
	sparam( error_rate_bpic, 0 );
	sparam( v_error_level_th_h, 0 );
	sparam( v_error_level_th_l, 0 );
	sparam( insert_buf_ctrl_param, 1 );
	sparam( insert_buf_ctrl_param_bl, 1 );
	sparam( insert_pic_struct, 1 );
	sparam( insert_pic_struct_bl, 1 );
	sparam( insert_recovery_point_sei, 1 );
	sparam( insert_recovery_point_sei_bl, 1 );
	sparam( overscan_info_present_flag, 1 );
	sparam( overscan_info_present_flag_bl, 1 );
	sparam( overscan_appropriate_flag, 1 );
	sparam( overscan_appropriate_flag_bl, 1 );
	sparam( insert_restriction, 1 );
	sparam( insert_restriction_bl, 1 );
	sparam( video_signal_type_present_flag, 0 );
	sparam( video_signal_type_present_flag_bl, 0 );
	sparam( video_format, 0 );
	sparam( video_format_bl, 0 );
	sparam( video_full_range_flag, 0 );
	sparam( video_full_range_flag_bl, 0 );
	sparam( colour_description_present_flag, 0 );
	sparam( colour_description_present_flag_bl, 0 );
	sparam( colour_primaries, 0 );
	sparam( colour_primaries_bl, 0 );
	sparam( transfer_characteristics, 0 );
	sparam( transfer_characteristics_bl, 0 );
	sparam( matrix_coefficients, 0 );
	sparam( matrix_coefficients_bl, 0 );
	sparam( time_scale_h_bl, 0 );
	sparam( time_scale_l_bl, 0 );
	sparam( num_units_in_tick_bl, 0 );
	sparam( a_sample, 0 );
	sparam( ain_master, 0 );
	sparam( ain_lsb_first, 0 );
	sparam( ain_bit_posb, 0 );
	sparam( amck_mode, 1 );
	sparam( ain_dlength, 2 );
	sparam( ain_lr_hlset, 1 );
	sparam( ain_i2s, 1 );
	sparam( ain_lrclk_sel, 0 );
	sparam( ain_sclk_sel, 0 );
	sparam( ain_bclk_sel, 0 );
	sparam( ain_bclk_hl, 0 );
	sparam( ain_offset_mode, 0 );
	sparam( ain_offset, 0 );
	sparam( lch_scale, 32768 );
	sparam( rch_scale, 32768 );
	sparam( lsch_scale, 32768 );
	sparam( rsch_scale, 32768 );
	sparam( cch_scale, 32768 );
	sparam( lfech_scale, 32768 );
	sparam( a_bitrate, 384 );
	sparam( e_mpeg_emp, 0 );
	sparam( e_mpeg_orig, 1 );
	sparam( e_mpeg_copyr, 0 );
	sparam( e_mpeg_mode, 0 );
	sparam( e_mpeg_protect, 0 );
	sparam( ach_sel, 0 );
}

using namespace Transcoder;
void GCHD::transcoderSetup(InputSettings &inputSettings, TranscoderSettings &settings)
{
	//////////////////
	//VIDEO SETTINGS//
	//////////////////

	unsigned horizontal, vertical;
	settings.getResolution(horizontal, vertical);

	//Whether we use group of pictures, or other methods.
	uint16_t useGroupOfPictures=1; //Always 1.

	//This is distance between anchor frames (I or P) for a group of pictures
	uint16_t distanceBetweenAnchorFrames=3; //Always 3.
	if( settings.getH264Profile()==H264Profile::Baseline ) {
		//No B frames allowed.
		distanceBetweenAnchorFrames=1;
	}
	//Okay default gopGroupSize is going to be (inputFrameRate+10)/5

	Utility::fraction_t fpsFraction={0, 0};
	double frameRate = settings.getFrameRate();
	double effectiveFrameRate = settings.getEffectiveFrameRate(); //Used for setting GOP up.

	bool fixedFrameRate=false;
	if ( frameRate != 0.0 ) {
		fpsFraction=Utility::findFraction( frameRate, 8192 );
		fixedFrameRate=true;
	}

	uint16_t gopGroupSize=(effectiveFrameRate+10)/5 ; //Distance between I frames.
	uint16_t variableLengthCodingMode=0; //Always 0, likely means CAVLC coding.
	//Probably sets variable length coding mode. Based on encoded format
	//this probably matches entropy_coding_mode_flag in T-REC-H.264-201003-S
	//so 0 = CAVLC coding and 1 = CABAC coding.

	//Think this sets whether we are using constant or variable bit rate.
	//I believe 0 is constant bit rate, 1 is variable, but 2 bit field....
	uint8_t bitRateMode;
	switch( settings.getBitRateMode() ) {
		case BitRateMode::Constant:
			bitRateMode=0;
			break;
		case BitRateMode::Variable:
			bitRateMode=1;
			break;
	}

	//Figure out bit rates. This sets reasonable defaults.
	unsigned bitRate        = settings.getConstantBitRateKbps();

	unsigned maxBitRate, averageBitRate, minBitRate;
	settings.getVariableBitRateKbps( maxBitRate, averageBitRate, minBitRate );

	// How often an IDR frame is sent.
	uint32_t idrInterval=0; //I believe setting it to 0 means every frame is IDR frame

	//I think profile 1=high, since that is what PMT profile_idc set to.
	//The Fujitsu mb86H58 only is documented to support high, main, and baseline
	//h.264 profile setting. May have to change PMT if this changes. No need to ever change.
	unsigned h264Profile=1;
	switch( settings.getH264Profile() ) {
		case H264Profile::High:
			h264Profile=1;
			break;
		case H264Profile::Main:
			h264Profile=2;
			break;
		default: //Currently don't support Baseline
			h264Profile=2;
			break;
	}

	//This is shifted left on decimal place: IE 31 is 3.1
	//https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC#Levels
	uint8_t h264Level= settings.unsignedH264Level(settings.getH264Level());

	//////////////////
	//AUDIO SETTINGS//
	//////////////////
	bool audioProtect=false;

	//probably value is set as:  http://msdn.microsoft.com/en-us/library/windows/desktop/dd319399%28v=vs.85%29.aspxonst std a
	// We don't seem to have non-mono example in our captures.
	//eAVEncMPACodingMode_Mono         = 0,
	//eAVEncMPACodingMode_Stereo       = 1,
	//eAVEncMPACodingMode_DualChannel  = 2,
	//eAVEncMPACodingMode_JointStereo  = 3,
	//eAVEncMPACodingMode_Surround     = 4
	uint8_t audioMode=0; //0 is mono

	//Copyright bit, set 1 if copywritten. https://msdn.microsoft.com/en-us/library/windows/desktop/hh162908%28v=vs.85%29.aspx
	bool audioCopyright=false;
	//Original bit in audio stream....1 = original
	bool audioOriginal=true;
	//Mpeg emphasis type: https://msdn.microsoft.com/en-us/library/windows/desktop/dd319403%28v=vs.85%29.aspx
	uint16_t audioMpegEmphasis=0;
	//bits per second for audio bitRate. 320 is out default currently.
	uint16_t audioBitRate=settings.getAudioBitRate();

	///////////////////
	//COLOUR SETTINGS//
	///////////////////
	bool colourDescriptionPresentSetting=true; //This is set in all captures.

	bool analog = inputSettings.getSource() != InputSource::HDMI;
	//These 3 are defined in p376 (pdf page 396) of T-REC-H.264-201003-S
	uint8_t colourPrimariesSetting;

	unsigned colourValue;
	if( analog ) {
		colourValue=6;
	} else {
		switch( inputSettings.getHDMIColorSpace() ) {
			case HDMIColorSpace::Full:
				colourValue=6;
				break;
			default:
				colourValue=1;
				break;
		}
	}

	colourPrimariesSetting=colourValue;

	uint8_t transferCharacteristicsSetting=1; //Always set to 1.

	uint8_t matrixCoefficientsSetting;
	matrixCoefficientsSetting=colourValue;

	bool videoFullRangeFlagSetting=false; //false in all captures.
	bool videoSignalTypePresentFlagSetting=true; //true in all captures.

	//0=Component
	//1=PAL
	//2=NTSC
	//3=SECAM
	//4=MAC
	//5=UNSPECIFIED.
	uint8_t videoFormatSetting=5; //5 in all captures.

	if ( deviceType_ == DeviceType::GameCaptureHDNew )  {
		sparam( 0x1574, 15, 1, 0 );
		sparam( v_max_cpb_delay, 3 );
	}

	/////////////////////////
	//SYSTEM BIT RATE WRITE//
	/////////////////////////
	//This will not yield the exact same numbers
	//as original driver, but slightly more conservative settings.
	uint32_t systemBitRate =  (1.075 *(settings.getRealMaxBitRateKbps()+audioBitRate) + 256);

	sparam( system_rate, systemBitRate ); //Always set higher than the bitRate...
	sparam( system_min_rate, 0 );

	////////////////////////
	//VIDEO SETTINGS WRITE//
	////////////////////////

	sparam( v_cl_gop, useGroupOfPictures );
	sparam( v_gop_struct, distanceBetweenAnchorFrames );
	sparam( v_gop_size, gopGroupSize );
	sparam( v_vlc_mode, variableLengthCodingMode );
	//This is always set to 0.
	sparam( v_vbr_converge_mode, 0 ); //not sure what it does.

	sparam( v_rate_mode, bitRateMode );

	sparam( v_bitrate, bitRate );
	sparam( v_max_bitrate, maxBitRate );
	sparam( v_ave_bitrate, averageBitRate);
	sparam( v_min_bitrate, minBitRate );

	//These set the basic resolution
	sparam( v_hsize_out, horizontal );
	sparam( v_vsize_out, vertical );

	sparam( v_vinpelclk, fixedFrameRate ); //This seems to be set to 1 when doing fixed frame rate....but might be for complicated

	//Lot of information on these in  T-REC-H.264-201003-S
	sparam( time_scale_h, fpsFraction.num>>16 ); //USED FOR SETTING FIXED FRAME -frame numerator here
	sparam( time_scale_l, fpsFraction.num & 0xffff );
	sparam( num_units_in_tick, fpsFraction.denom ); //FRAME DENOMINATOR HERE

	sparam( v_idr_interval_h, idrInterval>>16 );
	sparam( v_idr_interval_l, idrInterval & 0xffff );

	//This is subpart of v_idr_interval_l, don't understand this write.
	//Pretty sure not necessary.
	if ( deviceType_ == DeviceType::GameCaptureHDNew )  {
		sparam( 0x151c, 0, 5, 0 );
	}

	sparam( v_h264_profile, h264Profile );
	sparam( v_h264_level, h264Level );

	////////////////////////
	//AUDIO SETTINGS WRITE//
	////////////////////////
	sparam( a_sample, 0 ); // 2 bit unknown field.

	sparam( e_mpeg_protect, audioProtect );
	sparam( e_mpeg_mode, audioMode ); //Set mpeg audio mode?
	sparam( e_mpeg_copyr, audioCopyright );
	sparam( e_mpeg_orig, audioOriginal );
	sparam( e_mpeg_emp, audioMpegEmphasis );
	sparam( a_bitrate, audioBitRate );

	/////////////////////////
	//Colour Settings Write//
	/////////////////////////
	sparam( colour_description_present_flag, colourDescriptionPresentSetting );
	sparam( colour_primaries, colourPrimariesSetting );
	sparam( transfer_characteristics, transferCharacteristicsSetting );
	sparam( matrix_coefficients, matrixCoefficientsSetting );
	sparam( video_full_range_flag, videoFullRangeFlagSetting );

	//Whether to send video_format, video_full_range_flag,
	// and if colour_descripton_present_flag, colour_primaries, transfer_char..., matrix_coeff
	//p 372 of  T-REC-H.264-201003-S (pdf 392)
	sparam( video_signal_type_present_flag, videoSignalTypePresentFlagSetting );

	//Video format before we encoded it
	sparam( video_format, videoFormatSetting );
	sparam( video_full_range_flag, videoFullRangeFlagSetting ); //Still set to 0

	////////////////////////////////
	//Secondary streams (disabled)//
	////////////////////////////////
	if ( deviceType_ == DeviceType::GameCaptureHDNew )  {
		sparam( 0x1674, 15, 1, 0 );
		sparam( v_max_cpb_delay_bl, 3 );
	}

	sparam( system_rate_ts_out2, systemBitRate );
	sparam( system_min_rate_ts_out2, 0 );

	//Does bl mean baseline?
	sparam( v_cl_gop_bl, useGroupOfPictures );

	sparam( v_gop_struct_bl, distanceBetweenAnchorFrames );
	sparam( v_gop_size_bl, gopGroupSize);
	sparam( v_vlc_mode_bl, variableLengthCodingMode );
	sparam( v_rate_mode_bl, bitRateMode );
	sparam( v_vbr_converge_mode_bl, 0 );
	sparam( v_bitrate_bl, bitRate );
	sparam( v_max_bitrate_bl, maxBitRate );
	sparam( v_ave_bitrate_bl, averageBitRate );
	sparam( v_min_bitrate_bl, minBitRate );
	sparam( v_filler_bitrate_bl, 0 );
	sparam( v_hsize_out_bl, horizontal );
	sparam( v_vsize_out_bl, vertical );

	sparam( tfrff, 2 );

	sparam( v_idr_interval_h_bl, idrInterval>>16 );
	sparam( v_idr_interval_l_bl, idrInterval & 0xffff );

	//This is subpart of v_idr_interval_l_bl, don't understand this write.
	//Pretty sure not necessary.
	if ( deviceType_ == DeviceType::GameCaptureHDNew )  {
		sparam( 0x161c, 0, 5, 0 );
	}

	sparam( v_h264_profile_bl, h264Profile );
	sparam( v_h264_level_bl, 31 ); //Always set to 0x31 by driver, rather than calc value.

	sparam( time_scale_h_bl, fpsFraction.num>>16 ); //USED FOR SETTING FIXED FRAME -frame numerator here
	sparam( time_scale_l_bl, fpsFraction.num & 0xffff );
	sparam( num_units_in_tick_bl, fpsFraction.denom ); //FRAME DENOMINATOR HERE

	sparam( ach_sel_bl, 0 );

	sparam( e_mpeg_protect_bl, audioProtect );
	sparam( e_mpeg_mode_bl, audioMode );
	sparam( e_mpeg_copyr_bl, audioCopyright );
	sparam( e_mpeg_orig_bl, audioOriginal );
	sparam( e_mpeg_emp_bl, audioMpegEmphasis );
	sparam( a_bitrate_bl, audioBitRate );

	sparam( colour_description_present_flag_bl, colourDescriptionPresentSetting );
	sparam( colour_primaries_bl, colourPrimariesSetting );
	sparam( transfer_characteristics_bl, transferCharacteristicsSetting );
	sparam( matrix_coefficients_bl, matrixCoefficientsSetting );
	sparam( video_full_range_flag_bl, videoFullRangeFlagSetting );
	sparam( video_signal_type_present_flag_bl, videoSignalTypePresentFlagSetting );
	sparam( video_format_bl, videoFormatSetting );
	sparam( video_full_range_flag_bl, videoFullRangeFlagSetting );
	sparam( sout_mode, 0 );
	sparam( sout2_mode, 0 );
	sparam( v_ts_out_mode, 1 ); //Encode (H.264/MPEG2)
	sparam( a_ts_out_mode, 2 ); //Encode audio
	sparam( a_mode, 3 ); //Always 3 for MPEG-1 layer 2
	sparam( v_ts_out2_mode, 0 ); //1 first time through original driver
	sparam( a_ts_out2_mode, 0 ); //2 first time through original driver
	sparam( a2_mode, 0 ); //3 first time through original driver
}

using namespace Transcoder;
void GCHD::transcoderOutputEnable(bool value)
{
	sparam( amck_mode, value );
}

using namespace Transcoder;
void GCHD::transcoderFinalConfigure(InputSettings &inputSettings, TranscoderSettings &settings)
{
	transcoderWriteVideoAndAudioPids();
	sparam( dma_sel_out, 1 );

	uint8_t inSourceType=0;
	if ( inputSettings.getResolution() == Resolution::PAL ) {
		if( inputSettings.getSDStretch() ) {
			inSourceType=2;
		} else {
			inSourceType=3;
		}
	}
	if ( inputSettings.getResolution() == Resolution::NTSC ) {
		if( inputSettings.getSDStretch() ) {
			inSourceType=1;
		} else {
			inSourceType=0;
		}
	}
	//This appears to be set to 3 if signal is PAL 4:3
	//                          2 if signal is PAL stretched to 16:9
	//                          1 if signal is NTSC stretched to 16:9
	//                          0 if anything else whatsoever.
	sparam( v_in_source_type, inSourceType );

	//next setting, v_format is based on screen and input signal type.
	//Known settings:
	//0=1920x1080i 60hz
	//1=1920x1080i 50hz
	//2=1280x720p 60hz
	//3=1280x720p 50hz
	//4=640x480i 60hz (Educated guess).
	//5=576i@50hz PAL
	//8=480p@60hz NTSC
	//9=576p@50hz PAL
	//32=1080p@60hz, also appears to work for 1080p@30hz on component cable.
	//33=1080p@50hz
	//34=1080p@24hz
	uint8_t videoFormat=0;
	ScanMode scanMode = inputSettings.getScanMode();
	switch (inputSettings.getResolution())
	{
		case Resolution::HD1080:
			if( scanMode==ScanMode::Interlaced )  {
				videoFormat=0;
			} else {
				videoFormat=32;
			}
			break;
		case Resolution::HD720:
			if( scanMode==ScanMode::Interlaced ) {
				throw runtime_error( "Does not support HD720 interlaced modes currently." );
			} else {
				videoFormat=2;
			}
			break;
		case Resolution::PAL:
			if( scanMode==ScanMode::Interlaced ) {
				videoFormat=5;
			} else {
				videoFormat=9;
			}
			break;
		case Resolution::NTSC:
			if( scanMode==ScanMode::Interlaced ) {
				videoFormat=4;
			} else {
				videoFormat=8;
			}
			break;
		default:
			throw runtime_error( "Unsupported resolution." );
			break;
	}

	double refreshRate = inputSettings.getRefreshRate();
	if( inputSettings.getRefreshRate()==50.0 ) {
		videoFormat |= 1; //We currently don't support mode 0x34, so this works.
	} else if ((refreshRate != 60.0 )  && (refreshRate != 30.0)) {
		throw runtime_error( "We only support 30hz, 50hz, and 60hz refresh rates currently." );
	}
	sparam( v_format, videoFormat );

	//progressive scan or interlaced set here, 0=interlaced, 2=progressive scan.
	if ( scanMode==ScanMode::Interlaced ) {
		sparam( v_ip_format, 0);
	} else {
		sparam( v_ip_format, 2);
	}
	sparam( tfrff, 2 );
	sparam( tbc_mode, 1 );
	sparam( tbc_mode, 1 ); //probably artifact of function calls that this is repeated.
	sparam( a_sample, 0 );
	sparam( ain_master, 0 );
	sparam( ain_lsb_first, 0 );
	sparam( ain_bit_posb, 0 );
	sparam( amck_mode, 1 );
	sparam( ain_dlength, 2 );
	sparam( ain_lr_hlset, 1 );
	sparam( ain_i2s, 1 );
	sparam( ain_lrclk_sel, 0 );
	sparam( ain_sclk_sel, 0 );
	sparam( ain_bclk_sel, 0 );
	sparam( ain_bclk_hl, 0 );
	sparam( ain_offset_mode, 0 );
	sparam( ain_offset, 0 );
	sparam( v_disable_aspect_ratio_info_present_flag, 0 );
	sparam( v_disable_aspect_ratio_info_present_flag_bl, 0 );
	sparam( v_pic_order_present_flag, 1 );
	sparam( v_pic_order_present_flag_bl, 1 );
}

