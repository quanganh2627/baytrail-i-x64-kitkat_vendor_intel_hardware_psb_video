/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Elaine Wang <elaine.wang@intel.com>
 *    Zeng Li <zeng.li@intel.com>
 *    Edward Lin <edward.lin@intel.com>
 *
 */

#include <stdio.h>
#include <string.h>
#include "img_types.h"
#include "psb_def.h"
#include "psb_drv_debug.h"
#include "ptg_hostheader.h"
#ifdef _TOPAZHP_PDUMP_ALL_
#define _TOPAZHP_PDUMP_BITS_
#define _PDUMP_SEQ_HEADER
#define _TOPAZHP_PDUMP_PICTURE_
#define _TOPAZHP_PDUMP_SLICE_
#endif
/* Global stores the latest QP information for the DoHeader()
 * routine, should be filled in by the rate control algorithm.
 */
#define HEADERS_VERBOSE_OUTPUT 0

/* #define USESTATICWHEREPOSSIBLE 1 */

#define MAXNUMBERELEMENTS 16

/* SOME USEFUL TEST FUNCTIONS */
#ifndef TOPAZ_MTX_HW
static void Show_Bits(
    IMG_UINT8 *ucBitStream,
    IMG_UINT32 ByteStartBit,
    IMG_UINT32 Bits)
{
    char Txt[1024];
    IMG_UINT32 uiByteSize;
    IMG_UINT32 uiLp, uiBt, Bcnt;
    Bcnt = 0;
    uiByteSize = (Bits + ByteStartBit + 7) >> 3;
    for (uiLp = 0; uiLp < uiByteSize; uiLp++) {
        snprintf(Txt, strlen(" "), " ");
        for (uiBt = 128; uiBt >= 1; uiBt = uiBt >> 1) {
            Bcnt++;
            if (Bcnt > Bits + ByteStartBit || Bcnt <= ByteStartBit)
                snprintf(Txt, sizeof(Txt), "%sX", Txt);
            else
                snprintf(Txt, sizeof(Txt), "%s%i", Txt, (ucBitStream[uiLp] & uiBt) > 0);
        }

        snprintf(Txt, sizeof(Txt), "%s ", Txt);
        //printf(Txt);
        if ((uiLp + 1) % 8 == 0) printf("\n");
    }

    printf("\n\n");
}
#endif

#ifndef TOPAZ_MTX_HW

static void Show_Elements(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    IMG_UINT8 f;
    IMG_UINT32 TotalByteSize;
    IMG_UINT32 RTotalByteSize;

    RTotalByteSize = TotalByteSize = 0;
    for (f = 0; f < mtx_hdr->ui32Elements; f++) {
#if HEADERS_VERBOSE_OUTPUT
        drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Encoding Element [%i] - Type:%i\n", f, elt_p[f]->Element_Type);
#endif
        if (elt_p[f]->Element_Type == ELEMENT_STARTCODE_RAWDATA ||
            elt_p[f]->Element_Type == ELEMENT_RAWDATA) {
            TotalByteSize = elt_p[f]->ui8Size;
#if HEADERS_VERBOSE_OUTPUT
            drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Writing %i RAW bits to element.\n", elt_p[f]->ui8Size);
            Show_Bits((IMG_UINT8 *)(&elt_p[f]->ui8Size) + 1, 0, TotalByteSize);
#endif
            TotalByteSize += 8;

            RTotalByteSize += (((IMG_UINT32)((TotalByteSize + 7) / 8)) * 8);
            RTotalByteSize += 32;
        } else {
            TotalByteSize = 0;
            switch (elt_p[f]->Element_Type) {
            case ELEMENT_QP:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Insert token ELEMENT_QP (H264)- for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_SQP:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Insert token ELEMENT_SQP (H264)- for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_FRAMEQSCALE:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Insert token ELEMENT_FRAMEQSCALE (H263/MPEG4) - for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_SLICEQSCALE:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Insert token ELEMENT_SLICEQSCALE (H263/MPEG4) - for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_INSERTBYTEALIGN_H264:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Insert token ELEMENT_INSERTBYTEALIGN_H264 -  MTX to generate 'rbsp_trailing_bits()' field\n");
#endif
                break;
            case ELEMENT_INSERTBYTEALIGN_MPG4:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, ("Insert token ELEMENT_INSERTBYTEALIGN_MPG4 -  MTX to generate MPEG4 'byte_aligned_bits' field\n");
#endif
                break;
            default:
                break;
            }

            RTotalByteSize += 32;
#if HEADERS_VERBOSE_OUTPUT
            drv_debug_msg(VIDEO_DEBUG_GENERAL, ("No RAW bits\n\n");
#endif
        }
    }

    /* TotalByteSize=TotalByteSize+32+(&elt_p[f-1]->Element_Type-&elt_p[0]->Element_Type)*8; */

#if HEADERS_VERBOSE_OUTPUT
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("\nCombined ELEMENTS Stream:\n");
    Show_Bits((IMG_UINT8 *) mtx_hdr->asElementStream, 0, RTotalByteSize);
#endif
}
#endif

#ifdef _TOPAZHP_PDUMP_BITS_
static void ptg_print(unsigned char *ptmp, int num)
{
    int tmp; 
    do {  
      for(tmp=0;tmp < num;tmp++) {
        if(tmp%8==0)
            drv_debug_msg(VIDEO_DEBUG_GENERAL, ("\n\t\t");  
       drv_debug_msg(VIDEO_DEBUG_GENERAL, ("\t0x%02x", ptmp[tmp]);
     } 
     drv_debug_msg(VIDEO_DEBUG_GENERAL, ("\n\t\t}\n"); 
    } while (0);
}
#endif

/**
 * Header Writing Functions
 * Low level bit writing and ue, se functions
 * HOST CODE
 */
static void ptg__write_upto8bits_elements(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT8 ui8WriteBits,
    IMG_UINT16 ui16BitCnt)
{
    // This is the core function to write bits/bytes to a header stream, it writes them directly to ELEMENT structures.
    IMG_UINT8 *pui8WriteBytes;
    IMG_UINT8 *pui8SizeBits;
    union {
        IMG_UINT32 UI16Input;
        IMG_UINT8 UI8Input[2];
    } InputVal;
    IMG_UINT8 ui8OutByteIndex;
    IMG_INT16 i16Shift;
    if (ui16BitCnt==0)
        return ;

#ifdef _TOPAZHP_PDUMP_BITS_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "WBS(8) bits %x, cnt = %d\n", ui8WriteBits, ui16BitCnt);
#endif
    // First ensure that unused bits  in ui8WriteBits are zeroed
    ui8WriteBits &= (0x00ff >> (8 - ui16BitCnt));
    InputVal.UI16Input=0;
    pui8SizeBits=&(aui32ElementPointers[pMTX_Header->ui32Elements]->ui8Size); //Pointer to the bit count field
    pui8WriteBytes=&(aui32ElementPointers[pMTX_Header->ui32Elements]->aui8Bits); //Pointer to the space where header bits are to be written
    ui8OutByteIndex=(pui8SizeBits[0] / 8);
    if (!(pui8SizeBits[0]&7)) {
        if (pui8SizeBits[0]>=120) {
            //Element maximum bits send to element, time to start a new one
            pMTX_Header->ui32Elements++; // Increment element index
            aui32ElementPointers[pMTX_Header->ui32Elements]=(MTX_HEADER_ELEMENT *) &pui8WriteBytes[15]; //Element pointer set to position of next element (120/8 = 15 bytes)
            aui32ElementPointers[pMTX_Header->ui32Elements]->Element_Type=ELEMENT_RAWDATA; //Write ELEMENT_TYPE
            aui32ElementPointers[pMTX_Header->ui32Elements]->ui8Size=0; // Set new element size (bits) to zero
            ptg__write_upto8bits_elements(pMTX_Header,aui32ElementPointers, ui8WriteBits, ui16BitCnt); // Begin writing to the new element
        return ;//(IMG_UINT32) ui16BitCnt;
        }
        pui8WriteBytes[ui8OutByteIndex]=0; // Beginning a new byte, clear byte
    }
    i16Shift=(IMG_INT16) ((8-ui16BitCnt)-(pui8SizeBits[0]&7));
    if (i16Shift>=0) {
        ui8WriteBits <<= i16Shift;
        pui8WriteBytes[ui8OutByteIndex]|=ui8WriteBits;
        pui8SizeBits[0]=pui8SizeBits[0]+ui16BitCnt;
    } else {
        InputVal.UI8Input[1]=(IMG_UINT8) ui8WriteBits+256;
        InputVal.UI16Input >>= -i16Shift;
        pui8WriteBytes[ui8OutByteIndex]|=InputVal.UI8Input[1];

        pui8SizeBits[0]=pui8SizeBits[0]+ui16BitCnt;
        pui8SizeBits[0]=pui8SizeBits[0]-((IMG_UINT8) -i16Shift);
        InputVal.UI8Input[0]=InputVal.UI8Input[0] >> (8+i16Shift);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, InputVal.UI8Input[0],(IMG_UINT16) -i16Shift);
    }
    return ;//(IMG_UINT32) ui16BitCnt;
}

static void ptg__write_upto32bits_elements(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT32 ui32WriteBits,
    IMG_UINT32 ui32BitCnt)
{
    IMG_UINT32 ui32BitLp;
    IMG_UINT32 ui32EndByte;
    IMG_UINT8 ui8Bytes[4];
#ifdef _TOPAZHP_PDUMP_BITS_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "WBS(32) bits %x, cnt = %d\n", ui32WriteBits, ui32BitCnt);
#endif
    for (ui32BitLp=0; ui32BitLp<4; ui32BitLp++) {
        ui8Bytes[ui32BitLp]=(IMG_UINT8) (ui32WriteBits & 255);
        ui32WriteBits = ui32WriteBits >> 8;
    }

    ui32EndByte=((ui32BitCnt+7)/8);
    if ((ui32BitCnt)%8)
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ui8Bytes[ui32EndByte-1], (IMG_UINT8) ((ui32BitCnt)%8));
    else
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ui8Bytes[ui32EndByte-1], 8);
    if (ui32EndByte>1)
        for (ui32BitLp=ui32EndByte-1; ui32BitLp>0; ui32BitLp--) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ui8Bytes[ui32BitLp-1], 8);
        }
    return ;//ui32BitCnt;
}

static void ptg__generate_ue(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT32 uiVal)
{
    IMG_UINT32 uiLp;
    IMG_UINT8 ucZeros;
    IMG_UINT32 uiChunk;

#ifdef _TOPAZHP_PDUMP_BITS_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, " UE uiVla %x\n", uiVal);
#endif

    for (uiLp=1, ucZeros=0;  (uiLp-1) < uiVal ; uiLp=uiLp+uiLp, ucZeros++)
        uiVal=uiVal-uiLp;
    // ucZeros = number of preceding zeros required
    // uiVal = value to append after zeros and 1 bit
    //** Write preceding zeros
    for (uiLp=(IMG_UINT32) ucZeros; uiLp+1>8; uiLp-=8)
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
    //** Write zeros and 1 bit set
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8) 1, (IMG_UINT8) (uiLp+1));
    //** Write Numeric part
    while (ucZeros>8) {
        ucZeros-=8;
        uiChunk=(uiVal >> ucZeros);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8) uiChunk, 8);
        uiVal=uiVal-(uiChunk << ucZeros);
    }
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8) uiVal, ucZeros);

    return ;//ui32BitCnter;
}

static void ptg__generate_se(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    int iVal)
{
    IMG_UINT32 uiCodeNum;

 #ifdef _TOPAZHP_PDUMP_BITS_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, " SE iVla %x\n", iVal);
#endif
 
    if (iVal > 0)
        uiCodeNum=(IMG_UINT32) (iVal+iVal-1);
    else
        uiCodeNum=(IMG_UINT32) (-iVal-iVal);
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, uiCodeNum);
    return ;//ui32BitCnter;
}


static void ptg__insert_element_token(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    HEADER_ELEMENT_TYPE ui32Token)
{
    IMG_UINT8 ui8Offset = 0;
    IMG_UINT8 *ui8P = NULL;
#ifdef _TOPAZHP_PDUMP_BITS_
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
    "%s: in element = %d, Token = %d\n", __FUNCTION__, pMTX_Header->ui32Elements, ui32Token);
#endif

    if (pMTX_Header->ui32Elements!=ELEMENTS_EMPTY) {
        if (aui32ElementPointers[pMTX_Header->ui32Elements]->Element_Type==ELEMENT_STARTCODE_RAWDATA ||
            aui32ElementPointers[pMTX_Header->ui32Elements]->Element_Type==ELEMENT_RAWDATA ||
            aui32ElementPointers[pMTX_Header->ui32Elements]->Element_Type==ELEMENT_STARTCODE_MIDHDR) {
            //Add a new element aligned to word boundary
            //Find RAWBit size in bytes (rounded to word boundary))
            ui8Offset=aui32ElementPointers[pMTX_Header->ui32Elements]->ui8Size+8+31; // NumberofRawbits (excluding size of bit count field)+ size of the bitcount field
            ui8Offset/=32; //Now contains rawbits size in words
            ui8Offset+=1; //Now contains rawbits+element_type size in words
            ui8Offset*=4; //Convert to number of bytes (total size of structure in bytes, aligned to word boundary).
        } else {
            ui8Offset=4;
        }
        pMTX_Header->ui32Elements++;
        ui8P=(IMG_UINT8 *) aui32ElementPointers[pMTX_Header->ui32Elements-1];
        ui8P+=ui8Offset;
        aui32ElementPointers[pMTX_Header->ui32Elements]=(MTX_HEADER_ELEMENT *) ui8P;
    }
    else
        pMTX_Header->ui32Elements=0;

    aui32ElementPointers[pMTX_Header->ui32Elements]->Element_Type=ui32Token;
    aui32ElementPointers[pMTX_Header->ui32Elements]->ui8Size=0;
#ifdef _TOPAZHP_PDUMP_BITS_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, 
        "%s: ou element = %d, Token = %d\n",
        __FUNCTION__, pMTX_Header->ui32Elements, ui32Token);
#endif
}

/*
 * Intermediary functions to build H264 headers
 */
static void ptg__H264_writebits_startcode_prefix_element(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT32 ui32ByteSize)
{
    ///**** GENERATES THE FIRST ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
    IMG_UINT32 ui32Lp;
    // Byte aligned (bit 0)
    //(3 bytes in slice header when slice is first in a picture without sequence/picture_header before picture

    for (ui32Lp=0;ui32Lp<ui32ByteSize-1;ui32Lp++)
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 8);
    // Byte aligned (bit 32 or 24)
    return;
}

// helper function to start new raw data block
static IMG_BOOL bStartNextRawDataElement = IMG_FALSE;
static void CheckStartRawDataElement(MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers)
{
    if(bStartNextRawDataElement)
    {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        bStartNextRawDataElement = IMG_FALSE;
    }
}


static void ptg__insert_prefix_nal_header(MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers,
        H264_SLICE_HEADER_PARAMS *pSlHParams, IMG_BOOL bCabacEnabled)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start \n", __FUNCTION__);
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);

    ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, pSlHParams->ui8Start_Code_Prefix_Size_Bytes); //Can be 3 or 4 bytes - always 4 bytes in our implementations
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,        0, 1);   // forbidden_zero_bit

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_REFERENCE); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 14, 5);    // nal unit type

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 1);   // SVC extension flag

    // nal_unit_header_mvc_extension()
    {
        if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 1);   // non_idr_flag flag
        } else {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    1, 1);   // non_idr_flag flag
        }

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 6);   // priority_id flag
        ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers,    0, 10);   // view_id flag

        //ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 3);   // temporal_id flag
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_TEMPORAL_ID); // temporal_id flag

        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_ANCHOR_PIC_FLAG); // anchor_pic_flag

        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

        if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 1);   // interview flag
        } else {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    1, 1);   // interview flag
        }

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    1, 1);   // reserved one bit
    }

}

static void ptg__H264_writebits_VUI_params(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_VUI_PARAMS *VUIParams)
{
	// Builds VUI Params for the Sequence Header (only present in the 1st sequence of stream)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
	(0 << 4) |	// aspect_ratio_info_present_flag = 0 in Topaz
	(0 << 3) |	// overscan_info_present_flag (1 bit) = 0 in Topaz
	(0 << 2) |	// video_signal_type_present_flag (1 bit) = 0 in Topaz
	(0 << 1) |	// chroma_loc_info_present_flag (1 bit) = 0 in Topaz
	(1),												// timing_info_present_flag (1 bit) = 1 in Topaz
	5);
	// num_units_in_tick (32 bits) = 1 in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 8);

	// time_scale (32 bits) = frame rate
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,(IMG_UINT8)  VUIParams->Time_Scale, 8);

	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// fixed_frame_rate_flag (1 bit) = 1 in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// nal_hrd_parameters_present_flag (1 bit) = 1 in Topaz
	//** Definitions for nal_hrd_parameters() contained in VUI structure for Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// cpb_cnt_minus1 ue(v) = 0 in Topaz = 1b
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 4);	// bit_rate_scale (4 bits) = 0 in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 2, 4);	// cpb_size_scale (4 bits) = 2 in Topaz

	ptg__generate_ue(pMTX_Header, aui32ElementPointers, VUIParams->bit_rate_value_minus1); // bit_rate_value_minus1[0] ue(v) = (Bitrate/64)-1 [RANGE:0 to (2^32)-2]
	ptg__generate_ue(pMTX_Header, aui32ElementPointers, VUIParams->cbp_size_value_minus1); // cpb_size_value_minus1[0] ue(v) = (CPB_Bits_Size/16)-1   where CPB_Bits_Size = 1.5 * Bitrate  [RANGE:0 to (2^32)-2]
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, VUIParams->CBR,1);// cbr_flag[0] (1 bit) = 0 for VBR, 1 for CBR
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, VUIParams->initial_cpb_removal_delay_length_minus1, 5); // initial_cpb_removal_delay_length_minus1 (5 bits) = ???
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, VUIParams->cpb_removal_delay_length_minus1, 5); // cpb_removal_delay_length_minus1 (5 bits) = ???
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, VUIParams->dpb_output_delay_length_minus1, 5); // dpb_output_delay_length_minus1 (5 bits) = ???
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, VUIParams->time_offset_length, 5); // time_offst_length (5 bits) = ???
	///** End of nal_hrd_parameters()
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);	// vcl_hrd_parameters_present_flag (1 bit) = 0 in Topaz
	//if( nal_hrd_parameters_present_flag  ||  vcl_hrd_parameters_present_flag )
	//FIX for BRN23039
	//	low_delay_hrd_flag
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);	// low_delay_hrd_flag

	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);	// pic_struct_present_flag (1 bit) = 0 in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);	// bitstream_restriction_flag (1 bit) = 0 in Topaz
}


static void ptg__H264ES_writebits_picture_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_PICTURE_HEADER_PARAMS *pPHParams,
    H264_SCALING_MATRIX_PARAMS * psScalingMatrix)
{
#ifdef _TOPAZHP_TRACE_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: pic_parameter_set_id = %d\n",__FUNCTION__, pPHParams->pic_parameter_set_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: seq_parameter_set_id = %d\n",__FUNCTION__, pPHParams->seq_parameter_set_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: entropy_coding_mode_flag = %d\n",__FUNCTION__, pPHParams->entropy_coding_mode_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: weighted_pred_flag = %d\n",__FUNCTION__, pPHParams->weighted_pred_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: weighted_bipred_idc = %d\n",__FUNCTION__, pPHParams->weighted_bipred_idc);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: chroma_qp_index_offset = %d\n",__FUNCTION__, pPHParams->chroma_qp_index_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: constrained_intra_pred_flag = %d\n",__FUNCTION__, pPHParams->constrained_intra_pred_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: transform_8x8_mode_flag = %d\n",__FUNCTION__, pPHParams->transform_8x8_mode_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: pic_scaling_matrix_present_flag = %d\n",__FUNCTION__, pPHParams->pic_scaling_matrix_present_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: bUseDefaultScalingList = %d\n",__FUNCTION__, pPHParams->bUseDefaultScalingList);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: second_chroma_qp_index_offset = %d\n",__FUNCTION__, pPHParams->second_chroma_qp_index_offset);
#endif
    //**-- Begin building the picture header element
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
    ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, 4);

    ///* GENERATES THE FIRST (STATIC) ELEMENT OF THE H264_PICTURE_HEADER() STRUCTURE *///
    ///**** ELEMENT BITCOUNT: 18

    // 4 Byte StartCodePrefix Pregenerated in: H264_WriteBits_StartCodePrefix_Element()
    // Byte aligned (bit 32)
    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  (0 << 7) |    // forbidden_zero_bit
                                  (1 << 5) |    // nal_ref_idc (2 bits) = 1
                                  (8),  // nal_unit_tpye (5 bits) = 8
                                  8);
   // Byte aligned (bit 40)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pPHParams->pic_parameter_set_id);  // pic_parameter_set_id ue(v)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pPHParams->seq_parameter_set_id);  // seq_parameter_set_id ue(v)
    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  (pPHParams->entropy_coding_mode_flag << 4) |    // entropy_coding_mode_flag (1 bit) 0 for CAVLC
                                  (0 << 3) |                                                                      // pic_order_present_flag (1 bit) = 0
                                  (1 << 2) |                                                                      // num_slice_group_minus1 ue(v) = 0 in Topaz
                                  (1 << 1) |                                                                      // num_ref_idx_l0_active_minus1 ue(v) = 0 in Topaz
                                  (1),                                                                                    // num_ref_idx_l1_active_minus1 ue(v) = 0 in Topaz
                                  5);
    // WEIGHTED PREDICTION
    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers,
        (pPHParams->weighted_pred_flag << 2) |  // weighted_pred_flag (1 bit)
        (pPHParams->weighted_bipred_idc),           // weighted_bipred_flag (2 bits)
        3);

    //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_QP);
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES THE SECOND ELEMENT OF THE H264_PICTURE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 5
    //The following field will be generated as a special case by MTX - so not here
    // ptg__generate_se(pMTX_Header, pPHParams->pic_init_qp_minus26); // pic_int_qp_minus26 se(v) = -26 to 25 in Topaz
    // pic_int_qs_minus26 se(v) = 0 in Topaz
    ptg__generate_se(pMTX_Header, aui32ElementPointers, 0);

    // chroma_qp_index_offset se(v) = 0 in Topaz
    ptg__generate_se(pMTX_Header, aui32ElementPointers, pPHParams->chroma_qp_index_offset);

    // deblocking_filter_control_present_flag (1 bit) = 1 in Topaz
    // constrained_intra_pred_Flag (1 bit) = 0 in Topaz
    // redundant_pic_cnt_present_flag (1 bit) = 0 in Topaz
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (1 << 2) |
                                  (pPHParams->constrained_intra_pred_flag << 1) |
                                  (0),
                                  3);

    if (pPHParams->transform_8x8_mode_flag ||
        (pPHParams->second_chroma_qp_index_offset != pPHParams->chroma_qp_index_offset) ||
        pPHParams->pic_scaling_matrix_present_flag) {
        // 8x8 transform flag
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pPHParams->transform_8x8_mode_flag, 1);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
        // second_chroma_qp_index_offset se(v) = 0 in Topaz
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pPHParams->second_chroma_qp_index_offset);
    }

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_H264); // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n",__FUNCTION__);

    return;
}

static void ptg__H264ES_writebits_scalinglists(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers, 
    H264_SCALING_MATRIX_PARAMS * psScalingMatrix,
    IMG_BOOL bWrite8x8)
{
	// Used by H264_WriteBits_SequenceHeader and H264_WriteBits_PictureHeader
	IMG_UINT32	ui32List, ui32Index;
	IMG_INT32	i32CurScale, i32DeltaScale;

	if (!psScalingMatrix)
	{
		ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_CUSTOM_QUANT);
		return;
	}

	for (ui32List = 0; ui32List < 6; ui32List++)
	{
		if (psScalingMatrix->ui32ListMask & (1 << ui32List))
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); 	// seq_scaling_list_present_flag[ui32List] = 1

			i32CurScale = 8;
			for (ui32Index = 0; ui32Index < 16; ui32Index++)
			{
				i32DeltaScale = ((IMG_INT32)psScalingMatrix->ui8ScalingLists4x4[ui32List][ui32Index]) - i32CurScale;
				i32CurScale += i32DeltaScale;
				ptg__generate_se(pMTX_Header, aui32ElementPointers, i32DeltaScale); 	// delta_scale
			}
		}
		else
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); 	// seq_scaling_list_present_flag[ui32List] = 0
		}
	}

	if (!bWrite8x8) return;

	for (; ui32List < 8; ui32List++)
	{
		if (psScalingMatrix->ui32ListMask & (1 << ui32List))
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); 	// seq_scaling_list_present_flag[ui32List] = 1

			i32CurScale = 8;
			for (ui32Index = 0; ui32Index < 64; ui32Index++)
			{
				i32DeltaScale = ((IMG_INT32)psScalingMatrix->ui8ScalingLists8x8[ui32List - 6][ui32Index]) - i32CurScale;
				i32CurScale += i32DeltaScale;
				ptg__generate_se(pMTX_Header, aui32ElementPointers, i32DeltaScale);		// delta_scale
			}
		}
		else
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); 	// seq_scaling_list_present_flag[ui32List] = 0
		}
	}

}

static void ptg__H264ES_writebits_sequence_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    H264_CROP_PARAMS *psCrop,
    H264_SCALING_MATRIX_PARAMS * psScalingMatrix,
    IMG_BOOL8 bASO)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n",__FUNCTION__);
#ifdef _PDUMP_SEQ_HEADER
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
    "%s pSHParams->gaps_in_frame_num_value = %x\n",
    __FUNCTION__, pSHParams->gaps_in_frame_num_value);
#endif

	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
	ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, 4);


	///**** GENERATES THE FIRST ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///

	// 4 Byte StartCodePrefix Pregenerated in: H264_WriteBits_StartCodePrefix_Element()
	// Byte aligned (bit 32)
	ptg__write_upto8bits_elements(pMTX_Header, 
            aui32ElementPointers,(0 << 7) | // forbidden_zero_bit=0
	    (0x3 << 5) | // nal_ref_idc=01 (may be 11)
	    (7), // nal_unit_type=00111
            8);
	// Byte aligned (bit 40)
	switch (pSHParams->ucProfile)
	{
	case SH_PROFILE_BP:
		// profile_idc = 8 bits = 66 for BP (PROFILE_IDC_BP)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 66, 8);

		// Byte	aligned	(bit 48)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
			(1 << 7) |													// constraint_set0_flag = 1 for BP constraints
			((bASO ? 0 : 1) << 6) |                                                                           // constraint_set1_flag = 1 for MP constraints
			(1 << 5) |													// constraint_set2_flag = 1 for EP constraints
			((pSHParams->ucLevel==SH_LEVEL_1B ? 1:0) << 4),				// constraint_set3_flag = 1 for level 1b, 0 for others
			// reserved_zero_4bits = 0
			8);
		break;

	case SH_PROFILE_MP:
		// profile_idc = 8 bits = 77 for MP (PROFILE_IDC_MP)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 77, 8);

		// Byte	aligned	(bit 48)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
			(0 << 7) |													// constraint_set0_flag = 0 for no BP constraints
			(1 << 6) |													// constraint_set1_flag = 1 for MP constraints
			(1 << 5) |													// constraint_set2_flag = 1 for EP constraints
			((pSHParams->ucLevel==SH_LEVEL_1B ? 1:0) << 4),				// constraint_set3_flag = 1 for level 1b, 0 for others
			// reserved_zero_4bits = 0
			8);
		break;

	case SH_PROFILE_HP:
		// profile_idc = 8 bits = 100 for HP (PROFILE_IDC_HP)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 100, 8);

		// Byte aligned (bit 48)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
			(0 << 7) |													// constraint_set0_flag = 0 for no BP constraints
			(0 << 6) |													// constraint_set1_flag = 0 for no MP constraints
			(0 << 5) |													// constraint_set2_flag = 0 for no EP constraints
			(0 << 4),													// constraint_set3_flag = 0
			// reserved_zero_4bits = 0
			8);
		break;
        case SH_PROFILE_H444P:
            // profile_idc = 8 bits = 244 for H444P (PROFILE_IDC_H444P)
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 244, 8);

            // Byte aligned (bit 48)
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                (0 << 7) |      // constraint_set0_flag = 0 for no BP constraints
                (0 << 6) |      // constraint_set1_flag = 0 for no MP constraints
                (0 << 5) |      // constraint_set2_flag = 0 for no EP constraints
                (0 << 4),	        // constraint_set3_flag = 0
                // reserved_zero_4bits = 0
                8);
            break;
	}


	// Byte aligned (bit 56)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (pSHParams->ucLevel == SH_LEVEL_1B) ? 11 : (IMG_UINT8)pSHParams->ucLevel, 8);			// level_idc (8 bits) = 11 for 1b, 10xlevel for others

	ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);		// seq_parameter_set_id = 0

	if ((pSHParams->ucProfile == SH_PROFILE_HP) || (pSHParams->ucProfile == SH_PROFILE_H444P))
	{
		ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);		// chroma_format_idc = 1
		ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);		// bit_depth_luma_minus8 = 0
		ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);		// bit_depth_chroma_minus8 = 0
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSHParams->bIsLossless?1:0, 1); // qpprime_y_zero_transform_bypass_flag = 1 if lossless

		if (pSHParams->bUseDefaultScalingList || pSHParams->seq_scaling_matrix_present_flag)
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); 	// seq_scaling_matrix_present_flag
			if (!pSHParams->bUseDefaultScalingList)
			{
				ptg__H264ES_writebits_scalinglists(pMTX_Header, aui32ElementPointers, psScalingMatrix, IMG_TRUE);
				ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
			}
			else
			{
				ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8); 	// seq_scaling_list_present_flag[i] = 0; 0 < i < 8
			}
		}
		else
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); 	// seq_scaling_matrix_present_flag
		}
	}


	ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);		// log2_max_frame_num_minus4 = 1

	ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);		// pic_order_cnt_type = 0

	ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->log2_max_pic_order_cnt - 4);		// log2_max_pic_order_cnt_Isb_minus4 = 2

	ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->max_num_ref_frames); //num_ref_frames ue(2), typically 2

	// Bytes aligned (bit 72)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 
		(pSHParams->gaps_in_frame_num_value), // gaps_in_frame_num_value_allowed_Flag	- (1 bit)
		1);
	///**** GENERATES THE SECOND, VARIABLE LENGTH, ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
	///**** ELEMENT BITCOUNT: xx
	ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->ucWidth_in_mbs_minus1);				//pic_width_in_mbs_minus1: ue(v) from 10 to 44 (176 to 720 pixel per row)
	ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->ucHeight_in_maps_units_minus1);		//pic_height_in_maps_units_minus1: ue(v) Value from 8 to 35 (144 to 576 pixels per column)
	// We don't know the alignment at this point, so will have to use bit writing functions

	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,pSHParams->ucFrame_mbs_only_flag,1); // frame_mb_only_flag 1=frame encoding, 0=field encoding

	if(!pSHParams->ucFrame_mbs_only_flag) // in the case of interlaced encoding
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,0,1); // mb_adaptive_frame_field_flag = 0 in Topaz(field encoding at the sequence level)

	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,1,1); // direct_8x8_inference_flag=1 in Topaz
	if(psCrop->bClip) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,1,1);
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16LeftCropOffset);
	    ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16RightCropOffset);
	    ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16TopCropOffset);
	    ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16BottomCropOffset);
	} else {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,0,1);
	}

	///**** GENERATES THE THIRD ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
	///**** ELEMENT BITCOUNT: xx
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
            (pSHParams->VUI_Params_Present), // vui_parameters_present_flag (VUI only in 1st sequence of stream)
            1);

	if (pSHParams->VUI_Params_Present>0)
		ptg__H264_writebits_VUI_params(pMTX_Header, aui32ElementPointers, &(pSHParams->VUI_Params));


	// Finally we need to align to the next byte
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_H264); // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)

#ifdef _TOPAZHP_PDUMP_BITS_
  ptg_print(pMTX_Header, 64);
#endif
  
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n",__FUNCTION__);
    return;
}


static void ptg__H264ES_writebits_slice_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_BOOL bCabacEnabled
)
{
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
    //Can be 3 or 4 bytes - always 4 bytes in our implementations
    ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, pSlHParams->ui8Start_Code_Prefix_Size_Bytes);

    ///**** GENERATES THE FIRST ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 8

    // StartCodePrefix Pregenerated in: Build_H264_4Byte_StartCodePrefix_Element() (4 or 3 bytes)
    //(3 bytes when slice is first in a picture without sequence/picture_header before picture
    // Byte aligned (bit 32 or 24)
    // NOTE: Slice_Type and Frame_Type are always the same, hence SliceFrame_Type
    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers, (0 << 7) |                // forbidden_zero_bit
                                  ((pSlHParams->bReferencePicture) << 5) |        // nal_ref_idc (2 bits)
                                  ((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE ? 5 : 1)),    // nal_unit_tpye (5 bits) = I-frame IDR, and 1 for  rest
                                  8);

    //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_CURRMBNR);
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES THE SECOND ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    /* The following is slice parameter set in BP/MP */
    //ptg__generate_ue(pMTX_Header, aui32ElementPointers, (IMG_UINT32) pSlHParams->First_MB_Address);                               //first_mb_in_slice = First MB address in slice: ue(Range 0 -  1619)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, (IMG_UINT32)((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) ? SLHP_I_SLICEFRAME_TYPE : pSlHParams->SliceFrame_Type));                                    //slice_type ue(v): 0 for P-slice, 1 for B-slice, 2 for I-slice
    // kab: //not clean change from IDR to intra, IDR should have separate flag
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                  (1 << 5) |                                                                                                                                             //pic_parameter_set_id, ue(v) = 0  (=1b) in Topaz
                                  pSlHParams->Frame_Num_DO,                                                                                                               //frame_num (5 bits) = frame nuo. in decoding order
                                  6);

    // interlaced encoding
    if (pSlHParams->bPiCInterlace) {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);                                          // field_pic_flag = 1
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSlHParams->bFieldType, 1); // bottom_field_flag (0=top field, 1=bottom field)
    }

    if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE)
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->Idr_Pic_Id);    // idr_pic_id ue(v)

    if (pSlHParams->bPiCInterlace)
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (pSlHParams->Picture_Num_DO + pSlHParams->bFieldType), pSlHParams->log2_max_pic_order_cnt);                    // pic_order_cnt_lsb (6 bits) - picture no in display order
    else
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSlHParams->Picture_Num_DO, pSlHParams->log2_max_pic_order_cnt);                       // pic_order_cnt_lsb (6 bits) - picture no in display order


    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSlHParams->direct_spatial_mv_pred_flag, 1);// direct_spatial_mv_pred_flag (1 bit)
    if (pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE || pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE) {
        if (pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE && pSlHParams->num_ref_idx_l0_active_minus1 > 0) { //Do we have more then one reference picture?
            //Override amount of ref pics to be only 1 in L0 direction
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->num_ref_idx_l0_active_minus1);
        } else
            // num_ref_idx_active_override_flag (1 bit) = 0 in Topaz
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
    }
    if (pSlHParams->SliceFrame_Type != SLHP_I_SLICEFRAME_TYPE && pSlHParams->SliceFrame_Type != SLHP_IDR_SLICEFRAME_TYPE) {
        if ((pSlHParams->diff_ref_pic_num[0] || pSlHParams->bRefIsLongTermRef[0])
            || ((pSlHParams->diff_ref_pic_num[1] || pSlHParams->bRefIsLongTermRef[1]) && (pSlHParams->num_ref_idx_l0_active_minus1 > 0))) {
            //Specifiy first ref pic in L0
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); //ref_pic_list_modification_flag_l0

            if (pSlHParams->bRefIsLongTermRef[0]) {
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 2); // mod_of_pic_num = 2 (long term ref)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->uRefLongTermRefNum[0]); // long_term_pic_num
            } else if (pSlHParams->diff_ref_pic_num[0] == 0) {
                // Can't use 0, so use MaxPicNum which will wrap to 0
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1); // mod_of_pic_num = 1 (add to)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 31); // abs_diff_minus_1
            } else if (pSlHParams->diff_ref_pic_num[0] < 0) {
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // mod_of_pic_num = 0 (subtract from)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, -pSlHParams->diff_ref_pic_num[0] - 1); // abs_diff_minus_1
            } else {
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1); // mod_of_pic_num = 1 (add too)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->diff_ref_pic_num[0] - 1); // abs_diff_minus_1
            }

            if ((pSlHParams->diff_ref_pic_num[1] || pSlHParams->bRefIsLongTermRef[1]) && pSlHParams->SliceFrame_Type != SLHP_B_SLICEFRAME_TYPE) { //potentially second reference picture on P
                if (pSlHParams->bRefIsLongTermRef[1]) {
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 2); // mod_of_pic_num = 2 (long term ref)
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->uRefLongTermRefNum[1]); // long_term_pic_num
                } else if (pSlHParams->diff_ref_pic_num[1] < 0) {
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // mod_of_pic_num = 0 (subtract from)
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, -pSlHParams->diff_ref_pic_num[1] - 1); // abs_diff_minus_1
                } else {
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1); // mod_of_pic_num = 1 (add too)
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->diff_ref_pic_num[1] - 1); // abs_diff_minus_1
                }
            }

            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 3); // mod_of_pic_num = 3 (no more changes)
        } else
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); // ref_pic_list_ordering_flag_I0 (1 bit) = 0, no reference picture ordering in Topaz
    }
    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE) {
        if (pSlHParams->diff_ref_pic_num[1] || pSlHParams->bRefIsLongTermRef[1]) {
            //Specifiy first ref pic in L1
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); //ref_pic_list_modification_flag_l1

            if (pSlHParams->bRefIsLongTermRef[1]) {
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 2); // mod_of_pic_num = 2 (long term ref)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->uRefLongTermRefNum[1]); // long_term_pic_num
            } else if (pSlHParams->diff_ref_pic_num[1] < 0) {
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // mod_of_pic_num = 0 (subtract from)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, -pSlHParams->diff_ref_pic_num[1] - 1); // abs_diff_minus_1
            } else {
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1); // mod_of_pic_num = 1 (add too)
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->diff_ref_pic_num[1] - 1); // abs_diff_minus_1
            }

            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 3); // mod_of_pic_num = 3 (no more changes)
        } else
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); // ref_pic_list_ordering_flag_I1 (1 bit) = 0, no reference picture ordering in Topaz
    }
    if (pSlHParams->weighted_pred_flag &&
        ((pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE) || (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE))
        && (pSlHParams->weighted_bipred_idc == 1)) {
        int i;
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->luma_log2_weight_denom);
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->chroma_log2_weight_denom); // Always do chroma
        for (i = 0; i < (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE ? 2 : (pSlHParams->num_ref_idx_l0_active_minus1 + 1)); i++) { // Either 1 or 2
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSlHParams->luma_weight_l0_flag[i], 1);
            if (pSlHParams->luma_weight_l0_flag[i]) {
                ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->luma_weight_l0[i]);
                ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->luma_offset_l0[i]);
            }
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSlHParams->chroma_weight_l0_flag[i], 1);
            if (pSlHParams->chroma_weight_l0_flag[i]) {
                ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->chromaB_weight_l0[i]);
                ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->chromaB_offset_l0[i]);
                ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->chromaR_weight_l0[i]);
                ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->chromaR_offset_l0[i]);
            }
        }
    }

    if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);                                                                 // no_output_of_prior_pics_flag (1 bit) = 0
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSlHParams->bIsLongTermRef ? 1 : 0, 1);  // long_term_reference_flag (1 bit) = 0
    } else if (pSlHParams->bReferencePicture) {
        if (pSlHParams->bIsLongTermRef) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); // adaptive_ref_pic_marking_mode_flag (1 bit) = 0

            // Allow a single long-term reference
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 4);                                 // memory_management_control_operation
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 2);                                 // max_long_term_frame_idx_plus1

            // Set current picture as the long-term reference
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 6);                                 // memory_management_control_operation
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->uLongTermRefNum);                                       // long_term_frame_idx

            // End
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);                                 // memory_management_control_operation
        } else
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);         // adaptive_ref_pic_marking_mode_flag (1 bit) = 0
    }

    if (bCabacEnabled && ((SLHP_P_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type) ||
                          (SLHP_B_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type))) {
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // hard code cabac_init_idc value of 0
    }
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SQP); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 11
    // Next field is generated on MTX with a special commnad (not ELEMENT_RAW) - so not defined here
    //pucHS=ptg__generate_se(pucHS, puiBitPos, pSlHParams->Slice_QP_Delta); //slice_qp_delta se(v) = SliceQPy - (pic_init_qp_minus26+26)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->Disable_Deblocking_Filter_Idc); //disable_deblocking_filter_idc ue(v) = 2?
    if (pSlHParams->Disable_Deblocking_Filter_Idc != 1) {
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->iDebAlphaOffsetDiv2); //slice_alpha_c0_offset_div2 se(v) = 0 (1b) in Topaz
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->iDebBetaOffsetDiv2); //slice_beta_offset_div2 se(v) = 0 (1b) in Topaz
    }
    //num_slice_groups_minus1 ==0 in Topaz, so no slice_group_change_cycle field here
    // no byte alignment at end of slice headers
    return ;
}


static void ptg__H264_getelements_skip_P_slice(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT32 MB_No_In_Slice,
    IMG_BOOL bCabacEnabled)
{
    /* Skipped P-Slice
     * Ensure pSlHParams is filled with appropriate parameters for a B-slice
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* ptg__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER); */
    /* Not sure if this will be required in the final spec */
    ptg__H264ES_writebits_slice_header(mtx_hdr, elt_p, pSlHParams, bCabacEnabled);
    ptg__generate_ue(mtx_hdr, elt_p, MB_No_In_Slice); /* mb_skip_run = mb_no_in_slice */

    /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point) */
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_H264);
    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}


#if 0
static void ptg__H264_getelements_sequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    H264_CROP_PARAMS *psCropParams)
{
    /* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    ptg__H264ES_writebits_sequence_header(mtx_hdr, elt_p, pSHParams, psCropParams, NULL);
    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}
#endif

//static void ptg__H264_getelements_picture_header(MTX_HEADER_PARAMS *mtx_hdr)
//{
///* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
//* Essential we initialise our header structures before building
//*/
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//ptg__H264ES_writebits_picture_header(mtx_hdr, elt_p);
//mtx_hdr->Elements++; //Has been used as an index, so need to add 1 for a valid element count
//}


static void ptg__H264_getelements_slice_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_BOOL bCabacEnabled
)
{
    /* Builds a single slice header from the given parameters (mid frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    /* Not sure if this will be required in the final spec */
    /* ptg__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER);*/
    ptg__H264ES_writebits_slice_header(pMTX_Header, aui32ElementPointers, pSlHParams, bCabacEnabled);

    pMTX_Header->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

void H263_NOTFORSIMS_WriteBits_VideoPictureHeader(MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers,
					   H263_PICTURE_CODING_TYPE PictureCodingType,
					   //IMG_UINT8 ui8Q_Scale,
					   H263_SOURCE_FORMAT_TYPE SourceFormatType,
					   IMG_UINT8 ui8FrameRate,
					   IMG_UINT32 ui32PictureWidth,
					   IMG_UINT32 ui32PictureHeight
					   )
{
	IMG_UINT8 UFEP;
	IMG_UINT8 OCPCF = 0;

	// Essential we insert the element before we try to fill it!
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);

	// short_video_start_marker	= 22 Bits	= 0x20 Picture start code
	ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, 32, 22);

	// temporal_reference		= 8 Bits	= 0-255	Each picture increased by 1
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_TEMPORAL_REFERENCE);
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

	// marker_bit				= 1 Bit		= 1
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);

    // zero_bit					= 1 Bits	= 0
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);

    // split_screen_indicator	= 1	Bits	= 0	No direct effect on encoding of picture
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	
    // document_camera_indicator= 1	Bits	= 0	No direct effect on encoding of picture
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);

	// full_picture_freeze_release=1 Bits	= 0	No direct effect on encoding of picture
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);

    // source_format				= 3	Bits	= 1-4	See note
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, SourceFormatType, 3);

	if (SourceFormatType != 7)
	{
		// picture_coding_type		= 1 Bit		= 0/1	0 for I-frame and 1 for P-frame
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, PictureCodingType, 1);
		// four_reserved_zero_bits	= 4 Bits	= 0
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 4);
	}
    // the else block is for PLUSPTYPE header insertion.
	else
	{
        static IMG_UINT8 RTYPE = 0;

        // if I- Frame set Update Full Extended PTYPE to true
		if (PictureCodingType == I_FRAME)
		{
			UFEP = 1;
		}
		else
		{
			UFEP = 0;
		}
        
        // write UFEP of 3 bits.
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, UFEP, 3);

        // if UFEP was present( if it was 1).
        // Optional part of PPTYPE.
		if (UFEP == 1)
		{
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 6, 3);
			// souce_format_optional
			if (ui8FrameRate == 30 || ui8FrameRate == 0 /* unspecified */)
			{
				OCPCF  = 0;
			}
			else
			{
				OCPCF = 1; // 1 for Custom PCF.
			}
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, OCPCF , 1);

            /* 10 reserve bits ( Optional support for the encoding). All are OFF(0).
               - Optional Unrestricted Motion Vector (UMV)
               - Optional Syntax-based Arithmetic Coding (SAC)
               - Optional Advanced Prediction (AP) mode
               - Optional Advanced INTRA Coding (AIC) mode
               - Optional Deblocking Filter (DF) mode
               - Optional Slice Structured (SS) mode
               - Optional Reference Picture Selection(RPS) mode.
               - Optional Independent Segment Decoding (ISD) mode
               - Optional Alternative INTER VLC (AIV) mode
               - Optional Modified Quantization (MQ) mode
            */
			ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, 0, 10); // 10 reserve bits

            /* 4 reserve bits
               - 1 (ON) to prevent start code emulation.
               - 0  Reserved(shall be 0).
               - 0  Reserved(shall be 0).
               - 0  Reserved(shall be 0).
            */
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 8, 4);	// 4 reserve bits
		}
        // Optional Part of PPTYPE ends.

        // Mandatory part of PPTYPE starts.(MPPTYPE)
        // picture_coding_type		= 1 Bit		= 0/1	0 for I-frame and 1 for P-frame
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, PictureCodingType, 3 );
	
        /*
            - Optional Reference Picture Resampling (RPR) mode ( OFF) : 0
            - Optional Reference Picture Resampling (RPR) mode (OFF) : 0
         */
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0 , 2);

        // Rounding Type (RTYPE) (1 for P Picture, 0 for all other Picture frames.
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, RTYPE, 1);

        /* 2 reserve bits
           - 0  Reserved(shall be 0).
           - 0  Reserved(shall be 0).
        */
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 2);

        //   - 1 (ON) to prevent start code emulation.
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1 , 1);
		// Mandatory part of PTYPE ends.

        // CPM immediately follows the PPTYPE part of the header.
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0 ,1);

        /* Custom Picture Format (CPFMT) */
        /* if UFEP was present and Source Format type was 7(custom format) */
		if (UFEP == 1)
		{
            IMG_UINT16 ui16PWI,ui16PHI;

            // aspect ratio 4 bits value = 0010 (12:11)
			//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0x01, 4);
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 2, 4);

            // Picture Width Indication 9 bits.
			//ui16PictureWidth --;
			//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8)(ui16PictureWidth >> 8), 1);
			//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8)(ui16PictureWidth & 0xFF), 8);
            ui16PWI = (ui32PictureWidth >> 2) - 1;
            ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers,ui16PWI, 9);
			
            // Marker bit 1bit = 1 to prevent start code emulation.
			ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
			
            // Picture Height Indication 9 bits.
			//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8)(ui16PictureHeigth >> 8), 1);
			//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (IMG_UINT8)(ui16PictureHeigth & 0xFF), 8);
            ui16PHI = ui32PictureHeight >> 2;
            ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers,ui16PHI, 9);
            // good up to that point

            // Custom Picture Clock Frequency Code (CPCFC) (8 bits) only if PPTYPE is true, UFEP is set
            // and OCPCF is set.
			if (OCPCF == 1)
			{

				//IMG_UINT8 CPCFC;
				//CPCFC = (IMG_UINT8)(1800 / (IMG_UINT16)ui8FrameRate);
				//		// you can use the table for division
				//CPCFC <<= 1; // for Clock Conversion Code
				//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, CPCFC, 8);

                // Clock Conversion Code 1 bit = 0 for 1000 and 1 for 1001
                ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);

                // Clock Divisor : 7 bits The natural binary representation of the value of the clock divisor.
                ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ui8FrameRate, 7);
			}
		}
		if (OCPCF == 1)
		{
			ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_EXTENDED_TR);
			ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
		}

        // Rest of the header stuff for Picture Layer is absent because 
        // we do not support any of the advanced modes.
	}
    // Insert token to tell MTX to insert rate-control value (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, FrameQScale))
    // vop_quant				= 5 Bits	= x	5-bit frame Q_scale from rate control - GENERATED BY MTX
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_FRAMEQSCALE); 

	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    // if it was not PLUSPTYPE i.e for standard format size insert CPM bit here.
    if (SourceFormatType != 7)
	{
        // cpm	= 1 Bit		= 0	No direct effect on encoding of picture
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0 ,1);
    }
	// pei						= 1 Bit		= 0	No direct effect on encoding of picture
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);

	return;
}

void MPEG4_NOTFORSIMS_WriteBits_VOPHeader(MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers,
							IMG_BOOL	bIsVOP_coded,
							SEARCH_RANGE_TYPE sSearch_range,
							VOP_CODING_TYPE sVopCodingType)
{
    // Essential we insert the element before we try to fill it!
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
    // visual_object_sequence_start_code	= 32 Bits	= 0x1B6
    ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, 438, 32);
    // vop_coding_type						= 2 Bits	= 0 for I-frame and 1 for P-frame
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, sVopCodingType, 2);

    // modulo_time_base	
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_MODULO_TIME_BASE);
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    // marker_bit							= 1	Bits	= 1
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);

    // vop_time_increment
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_VOP_TIME_INCREMENT);
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    // marker_bit							= 1 Bit		= 1
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);

    if (!bIsVOP_coded)
    {
	// vop_coded						= 1 Bit		= 0 for skipped frame
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
       	// byte_aligned_bits (skipped pictures are byte aligned)
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_MPG4); // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
	// End of VOP - skipped picture
    } else {
	// vop_coded						= 1 Bit		= 1 for normal coded frame
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
	if (sVopCodingType == P_FRAME)
	{
	    // vop_rounding_type			= 1 Bit		= 0 vop_rounding_type is 0 in Topaz
	    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	}
	// intra_dc_vlc_thr					= 3 Bits	= 0 Use intra DC VLC in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 3);
	// vop_quant				= 5 Bits	= x	5-bit frame Q_scale from rate control - GENERATED BY MTX
	//ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, Frame_Q_scale, 5);
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_FRAMEQSCALE);
	if (sVopCodingType == P_FRAME)
	{
	    // vop_fcode_forward			= 3 bits	= 2 for +/-32 and 3 for +/-64 search range
	    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
	    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, sSearch_range, 3);
	}
    }
}
/**************************************************************************************************
 * * Function:         H263_GeneratePicHdrTemplate
 * * Description:      Generates the h.263 picture header template
 * *
 * ***************************************************************************************************/
void ptg__H263_notforsims_prepare_video_pictureheader(
    MTX_HEADER_PARAMS* pMTX_Header,
    H263_PICTURE_CODING_TYPE ePictureCodingType,
    H263_SOURCE_FORMAT_TYPE eSourceFormatType,
    IMG_UINT8 ui8FrameRate,
    IMG_UINT32 ui32PictureWidth,
    IMG_UINT32 ui32PictureHeigth )
{
    // Essential we initialise our header structures before building
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements=ELEMENTS_EMPTY;
    This_Element=(MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0]=This_Element;

    H263_NOTFORSIMS_WriteBits_VideoPictureHeader(
               pMTX_Header,
               aui32ElementPointers,
               ePictureCodingType,
               eSourceFormatType,
               ui8FrameRate,
               ui32PictureWidth,
               ui32PictureHeigth );

    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
}

/**************************************************************************************************
 * * Function:         MPEG4_GeneratePicHdrTemplate
 * * Description:      Generates the MPEG4 picture header template
 * *
 * ***************************************************************************************************/
void ptg__MPEG4_notforsims_prepare_vop_header(
    MTX_HEADER_PARAMS* pMTX_Header,
    IMG_BOOL bIsVOP_coded,
    SEARCH_RANGE_TYPE eSearch_range,
    VOP_CODING_TYPE eVop_Coding_Type)
{
    //Builds a single MPEG4 VOP (picture) header from the given parameters
    //Essential we initialise our header structures before building
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements=ELEMENTS_EMPTY;
    This_Element=(MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0]=This_Element;

    //Frame QScale no longer written here as it is inserted by MTX later (add as parameter to MTX_Send_Elements_To_VLC)
    MPEG4_NOTFORSIMS_WriteBits_VOPHeader(
	pMTX_Header, 
	aui32ElementPointers, 
	bIsVOP_coded, 
	eSearch_range, 
	eVop_Coding_Type);

    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
}

void H263_NOTFORSIMS_WriteBits_GOBSliceHeader(MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers)
{
       // Essential we insert the element before we try to fill it!
       ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
    // gob_resync_marker               = 17            = 0x1
       ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, 1, 17);

       // gob_number                           = 5                     = 0-17  It is gob number in a picture
       ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SLICE_NUM); // Insert token to tell MTX to insert gob_number

    // gob_frame_id                            = 2                     = 0-3   See note
       ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_GOB_FRAME_ID); // Insert token to tell MTX to insert gob_frame_id

       ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

       // quant_scale                          = 5                     = 1-32  gob (Slice) Q_scale
       // ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ui8GOB_Q_Scale, 5);
       ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SLICEQSCALE); // Insert token to tell MTX to insert rate-control value (QScale is sent as an argument
}


//static void ptg__H264_getelements_endofsequence_header(
//MTX_HEADER_PARAMS *mtx_hdr)
//{
///* Builds a single endofsequence header from the given parameters (mid frame) */
//
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//ptg__H264_writebits_endofsequence_header(mtx_hdr, elt_p);
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}



//static void ptg__H264_getelements_endofstream_header(MTX_HEADER_PARAMS *mtx_hdr)
//{
///* Builds a single endofstream header from the given parameters (mid frame) */
//
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//ptg__H264_writebits_endofstream_header(mtx_hdr, elt_p);
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}

static IMG_UINT8 Bits2Code(IMG_UINT32 CodeVal)
{
    IMG_UINT8 Bits = 32;
    if (CodeVal == 0)
        return 1;
    while (!(CodeVal & 0x80000000)) {
        CodeVal <<= 1;
        Bits--;
    }
    return Bits;
}

/*
 * Intermediary functions to build MPEG4 headers
 */
#define MATCH_TO_ENC


static void ptg__MPEG4_writebits_sequence_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE bProfile,
    IMG_UINT8 ui8Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS *sVBVParams, IMG_UINT32 ui32VopTimeResolution) /* Send NULL pointer if there are no VBVParams */
{
	// Essential we insert the element before we try to fill it!
	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
	// visual_object_sequence_start_code	= 32 Bits	= 0x1B0
	ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, 432, 32);
	//profile_and_level_indication			= 8 Bits	= SP L0-L3 and SP L4-L5 are supported
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ui8Profile_and_level_indication, 8);

	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
	// visual_object_start_code				= 32 Bits	= 0x1B5
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 181, 8);
	// is_visual_object_identifier			= 1 Bit 	= 0
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	// visual_object_type					= 4 Bits	= Video ID = 1
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 4);
	// video_signal_type					= 1 Bit		= 1
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	// byte_aligned_bits					= 2 Bits	= 01b (byte_aligned_bits is 2-bit stuffing bit field 01)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 2);

	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
	//video_object_start_code				= 32 Bits	= 0x100 One VO only in a Topaz video stream
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);

	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
	// video_object_layer_start_code		= 32 Bits	= 0x120 One VOL only in a Topaz stream
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 8);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 32, 8);
	// random_accessible_vol				= 1 Bit		= 0 (P-Frame in GOP)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	if (bProfile == SP)
	{
		// video_object_type_indication			= 8 Bits	= 0x01 for SP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 8);
#ifndef MATCH_TO_ENC
		// is_object_layer_identifier			= 1 Bit		= 0 for SP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
#else
	// to match the encoder
		// is_object_layer_identifier			= 1 Bit
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
		// video_object_layer_verid				= 4 Bits
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 4);
        // video_object_layer_priority			= 3 Bits
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 3);  // 0 is reserved...
#endif
	}
	else
	{
		// video_object_type_indication			= 8 Bits	= 0x11 for ASP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 3, 8);
		// is_object_layer_identifier			= 1 Bit		= 1 for ASP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
		// video_object_layer_verid				= 4 Bits	= 5 is for ASP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 5, 4);
        // video_object_layer_priority			= 3 Bits	= 1 (Highest priority)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 3);
	}
	// aspect_ratio_info						= 4 Bits	=0x1 (Square pixel)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 4);
#if !defined(MATCH_TO_ENC) || !defined (EXCLUDE_VOL_CONTROL_PARAMS)
	// vol_control_parameters					= 1 Bit		= 1 (Always send VOL control parameters)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);

    // chroma_format							= 2 Bits	= 01b (4:2:0)
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 2);
	// low_delay							= 1 Bit			= 0 with B-frame and 1 without B-frame
	if (bBFrame)
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	else
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
    // vbv_parameters						= 1 Bit			=0/1
	if (sVBVParams)
	{
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
		 //For recording, only send vbv parameters in 1st sequence header. For video phone, it should be sent more often, such as once per sequence
		ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, sVBVParams->First_half_bit_rate, 15);			// first_half_bit_rate
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// Marker Bit = 1
		ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, sVBVParams->Latter_half_bit_rate, 15);			// latter_half_bit_rate
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// Marker Bit = 1
		ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, sVBVParams->First_half_vbv_buffer_size, 15);	// first_half_vbv_buffer_size
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// Marker Bit = 1
        ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, sVBVParams->Latter_half_vbv_buffer_size, 3);	//  latter_half_vbv_buffer_size
        ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, sVBVParams->First_half_vbv_occupancy, 11);		//  first_half_vbv_occupancy
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// Marker Bit = 1
		ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, sVBVParams->Latter_half_vbv_occupancy, 15);		//  latter_half_vbv_occupancy
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);	// Marker Bit = 1
	}
	else
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); // No vbv parameters present
#else
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
#endif
	// video_object_layer_shape			= 2 Bits		=	00b	Rectangular shape
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 2);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);		// Marker Bit = 1
	// vop_time_increment_solution		= 16 Bits
	ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, ui32VopTimeResolution, 16);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);		// Marker Bit = 1
#ifndef MATCH_TO_ENC
	// fixed_vop_rate					= 1 Bits		=	1 Always fixed frame rate
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
	// fixed_vop_time_increment			= Variable number of bits based on the time increment resolution.
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, Bits2Code(ui32VopTimeResolution));
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);		// Marker Bit = 1
#else
	// fixed_vop_rate					= 1 Bits		=	0
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);		// Marker Bit = 1
#endif
	// video_object_layer_width			= 13 Bits		Picture width in pixel units
	ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, Picture_Width_Pixels, 13);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);		// Marker Bit = 1
	// video_object_layer_height		= 13 Bits		Picture height in pixel units
	ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, Picture_Height_Pixels, 13);
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);		// Marker Bit = 1
	// interlaced						= 1 Bit			= 0 Topaz only encodes progressive frames
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	// obmc_disable						= 1 Bit			= 1 No overlapped MC in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
    // sprite_enable					= 1 Bit			= 0 Not use sprite in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	// not_8_bit						= 1	Bit			= 0	8-bit video in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	// quant_type						= 1 Bit			= 0 2nd quantization method in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	if (bProfile==ASP)
	{
		// quarter_sample				= 1 Bit			= 0 No ?pel MC in Topaz
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	}

	// complexity_estimation_disable	= 1 Bit			= 1	No complexity estimation in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
#ifndef MATCH_TO_ENC
	// resync_marker_disable			= 1 Bit			= 0 Always enable resync marker in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
#else
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
#endif
    // data_partitioned					= 1 Bit			= 0 No data partitioning in Topaz
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
    if (bProfile == ASP)
	{
		// newpred_enable				= 1 Bit			= 0 No newpred mode in SP/ASP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
		// reduced_vop_resolution_enable=1 Bit			= 0	No reduced resolution frame in SP/ASP
		ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
    }
    // scalability						= 1 Bit			= 0	No scalability in SP/ASP
	ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
	// byte_aligned_bits
   	ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_MPG4); // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
	return;
}


/* Utility function */
/*
  IMG_UINT8 Bits2Code(IMG_UINT32 CodeVal)
  {
  IMG_UINT8 Bits=32;
  if(CodeVal==0)
  return 1;
  while(!(CodeVal & 0x80000000))
  {
  CodeVal<<=1;
  Bits--;
  }
  return Bits;
  }
*/

/* MPEG 4 VOP (Picture) Header */
static void ptg__MPEG4_writebits_VOP_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_BOOL    bIsVOP_coded,
    IMG_UINT8   VOP_time_increment,
    SEARCH_RANGE_TYPE sSearch_range,
    VOP_CODING_TYPE sVopCodingType,
    IMG_UINT32 VopTimeResolution)
{
    IMG_BOOL bIsSyncPoint;
    /* Essential we insert the element before we try to fill it! */
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B6 */
    ptg__write_upto32bits_elements(mtx_hdr, elt_p, 438, 32);

    /* vop_coding_type  = 2 Bits = 0 for I-frame and 1 for P-frame */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, sVopCodingType, 2);
    bIsSyncPoint = (VOP_time_increment > 1) && ((VOP_time_increment) % VopTimeResolution == 0);

#ifndef MATCH_TO_ENC
    /* modulo_time_base = 1 Bit = 0 As at least  1 synchronization point (I-frame) per second in Topaz */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else

    ptg__write_upto8bits_elements(mtx_hdr, elt_p, bIsSyncPoint  ? 2 : 0 , bIsSyncPoint ? 2 : 1);

#endif

    /* marker_bit = 1   Bits    = 1      */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

#ifndef MATCH_TO_ENC
    /* vop_time_increment = Variable bits based on resolution
     *  = x Reset to 0 at I-frame and plus fixed_vop_time_increment each frame
     */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, VOP_time_increment, 5);
#else
    /* will chrash here... */
    ptg__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (VOP_time_increment) % VopTimeResolution,
        Bits2Code(VopTimeResolution - 1));

#endif
    /* marker_bit = 1 Bit               = 1      */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    if (!bIsVOP_coded) {
        /* vop_coded    = 1 Bit         = 0 for skipped frame */
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

        /* byte_aligned_bits (skipped pictures are byte aligned) */
        /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
         * End of VOP - skipped picture
         */
        ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_MPG4);
    } else {
        /* vop_coded = 1 Bit            = 1 for normal coded frame */
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        if (sVopCodingType == P_FRAME) {
            /* vop_rounding_type = 1 Bit = 0 vop_rounding_type is 0 in Topaz */
            ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        }

        /* intra_dc_vlc_thr = 3 Bits = 0 Use intra DC VLC in Topaz */
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 3);

        /* vop_quant = 5 Bits   = x     5-bit frame Q_scale from rate control - GENERATED BY MTX */
        /* ptg__write_upto8bits_elements(mtx_hdr, elt_p, Frame_Q_scale, 5); */
        ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_FRAMEQSCALE);

        if (sVopCodingType == P_FRAME) {
            /* vop_fcode_forward = 3 bits       = 2 for +/-32 and 3 for +/-64 search range  */
            ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
            ptg__write_upto8bits_elements(mtx_hdr, elt_p, sSearch_range, 3);
        }

        /*
        **** THE FINAL PART OF VOP STRUCTURE CAN'T BE GENERATED HERE
        ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
        video_packet_data ( )                   = 1st  VP that doesn�t have the VP header

        while (nextbits_bytealigned ( ) == resync_marker)
        {
        video_packet _header( )
        video_packet _data( )                   All MB in the slice
        }
        */
    }
}


/* MPEG 4 Video Packet (Slice) Header */
//static void ptg__MPEG4_writebits_videopacket_header(
//MTX_HEADER_PARAMS *mtx_hdr,
//MTX_HEADER_ELEMENT **elt_p,
//VOP_CODING_TYPE eVop_Coding_Type,
//IMG_UINT8 Fcode,
//IMG_UINT32 MBNumber,
//IMG_UINT32 MBNumberlength,
//IMG_BOOL bHeader_Extension_Code,
//IMG_UINT8 VOP_Time_Increment,
//SEARCH_RANGE_TYPE sSearch_range)
//{
///* Essential we insert the element before we try to fill it! */
//ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
//
//if (eVop_Coding_Type == I_FRAME)
//{
///* resync_marker      = 17 bit        =0x1    17-bit for I-frame */
//ptg__write_upto32bits_elements(mtx_hdr, elt_p, 1, 17);
//}
//else
//{
///* resync_marker = 17 bit     =0x1    (16+fcode) bits for P-frame */
//ptg__write_upto32bits_elements(mtx_hdr, elt_p, 1, 16+Fcode);
//}
//
///* macroblock_number = 1-14 bits      = ?????? */
//ptg__write_upto32bits_elements(mtx_hdr, elt_p, MBNumber, MBNumberlength);
//
///* quant_scale = 5 bits       =1-32   VP (Slice) Q_scale
//* ptg__write_upto8bits_elements(mtx_hdr, elt_p, VP_Slice_Q_Scale, 5);
//*/
//ptg__insert_element_token(
//mtx_hdr, elt_p,
//ELEMENT_SLICEQSCALE); /* Insert token to tell MTX to insert rate-control value */
//
//ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA); /* Begin writing rawdata again */
//
//if (bHeader_Extension_Code)
//{
///* header_extension_code = 1bit = 1   picture header parameters are repeated */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
//
///* modulo_time_base = 1 bit = 0       The same as it is in the current picture header */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
//
///* marker_bit = 1 bit         = 1 */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
//
///* vop_time_increment = 5 bits        = 0-30  The same as it is in the current picture header */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, VOP_Time_Increment, 5);
//
///* marker_bit = 1 bit         = 1      */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
//
///* vop_coding_type= 2 bits    = 0/1 The same as it is in the current picture header */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, eVop_Coding_Type, 2);
//
///* intra_dc_vlc_thr = 3 bits  = 0 The same as it is in the current picture header */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 3);
//
//if (eVop_Coding_Type == P_FRAME)
//{
///* vop_fcode_forward = 3 bits = 2/3 The same as it is in the current picture header */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, sSearch_range, 3);
//}
//}
//else
//{
///* header_extension_code = 1 bits =0  picture header parameters are NOT repeated */
//ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
//}
//}

/*
 * High level functions to call when a MPEG4 header is required - HOST ROUTINES
 */
//static void ptg__MPEG4_getelements_sequence_header (
//MTX_HEADER_PARAMS *mtx_hdr,
//IMG_BOOL bBFrame,
//MPEG4_PROFILE_TYPE sProfile,
//IMG_UINT8 Profile_and_level_indication,
//FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
//IMG_UINT32 Picture_Width_Pixels,
//IMG_UINT32 Picture_Height_Pixels,
//VBVPARAMS *sVBVParams,
//IMG_UINT32 VopTimeResolution) /* NULL pointer if there are no VBVParams */
//{
///* Builds a single MPEG4 video sequence header from the given parameters */
//
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//ptg__MPEG4_writebits_sequence_header(
//mtx_hdr,
//elt_p,
//bBFrame, sProfile,
//Profile_and_level_indication,
//sFixed_vop_time_increment,
//Picture_Width_Pixels,
//Picture_Height_Pixels,
//sVBVParams,VopTimeResolution);
//
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}


/* MPEG 4 VOP (Picture) Header */
//static void ptg__MPEG4_getelements_VOP_header(
//MTX_HEADER_PARAMS *mtx_hdr,
//IMG_BOOL      bIsVOP_coded,
//IMG_UINT8     VOP_time_increment,
//SEARCH_RANGE_TYPE sSearch_range,
//VOP_CODING_TYPE sVopCodingType,
//IMG_UINT32 VopTimeResolution)
//{
///* Builds a single MPEG4 VOP (picture) header from the given parameters */
//
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
///* Frame QScale no longer written here as it is inserted by MTX later
//* (add as parameter to MTX_Send_Elements_To_VLC)
//*/
//ptg__MPEG4_writebits_VOP_header(
//mtx_hdr, elt_p, bIsVOP_coded,
//VOP_time_increment,
//sSearch_range,
//sVopCodingType,VopTimeResolution);
//
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}


//static void ptg_MPEG4_getelements_video_packet_header(
//MTX_HEADER_PARAMS *mtx_hdr,
//VOP_CODING_TYPE eVop_Coding_Type,
//IMG_UINT8 Fcode,
//IMG_UINT32 MBNumber,
//IMG_UINT32 MBNumberlength,
//IMG_BOOL bHeader_Extension_Code,
//IMG_UINT8 VOP_Time_Increment,
//SEARCH_RANGE_TYPE sSearch_range)
//{
///* Builds a single MPEG4 video packet (slice) header from the given parameters */
//
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
///* Slice QScale no longer written here as it is inserted by MTX later
//* (add as parameter when sending to VLC)
//*/
//ptg__MPEG4_writebits_videopacket_header(
//mtx_hdr, elt_p, eVop_Coding_Type,
//Fcode, MBNumber, MBNumberlength,
//bHeader_Extension_Code, VOP_Time_Increment, sSearch_range);
//
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}

/*
 * Intermediary functions to build H263 headers
 */
static void H263_writebits_VideoSequenceHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 Profile_and_level_indication)
{
    /* Essential we insert the element before we try to fill it! */
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B0 */
    ptg__write_upto32bits_elements(mtx_hdr, elt_p, 432, 32);

    /* profile_and_level_indication = 8 Bits =  x SP L0-L3 and SP L4-L5 are supported */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, Profile_and_level_indication, 8);

    /* visual_object_start_code = 32 Bits       = 0x1B5 */

    /* 437 too large for the   ptg__write_upto32bits_elements function */
    ptg__write_upto32bits_elements(mtx_hdr, elt_p, 437, 32);

    /* is_visual_object_identifier = 1 Bit              = 0 */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* is_visual_object_type    = 4 Bits        = 1 Video ID */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

    /* video_signal_type = 1 Bit                = 0      */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits = 2 Bits = 01b byte_aligned_bits is 2-bit stuffing bit field 01 */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* video_object_start_code  =32 Bits        = 0x100 One VO only in a Topaz video stream */
    ptg__write_upto32bits_elements(mtx_hdr, elt_p, 256, 32);

    return;
}

static void H263_writebits_VideoPictureHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    //IMG_UINT8 Q_Scale,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT32 PictureWidth,
    IMG_UINT32 PictureHeight)
{
    IMG_UINT8 UFEP;
    IMG_UINT8 OCPCF = 0;

    /* Essential we insert the element before we try to fill it! */
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* short_video_start_marker = 22 Bits       = 0x20 Picture start code */
    ptg__write_upto32bits_elements(mtx_hdr, elt_p, 32, 22);

    /* temporal_reference = 8 Bits      = 0-255 Each picture increased by 1 */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, Temporal_Ref, 8);

    /* marker_bit = 1 Bit = 1    */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* zero_bit = 1 Bits        = 0      */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* split_screen_indicator   = 1     Bits    = 0     No direct effect on encoding of picture */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* document_camera_indicator= 1     Bits    = 0     No direct effect on encoding of picture */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* full_picture_freeze_release=1 Bits       = 0     No direct effect on encoding of picture */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* source_format                            = 3     Bits    = 1-4   See note */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, SourceFormatType, 3);

    /*Write optional Custom Picture Clock Frequency(OCPCF)*/
    if (FrameRate == 30 || FrameRate == 0/* unspecified */) {
        OCPCF = 0; // 0 for CIF PCF
    } else {
        OCPCF = 1; //1 for Custom PCF
    }

    if (SourceFormatType != 7) {
        // picture_coding_type          = 1 Bit         = 0/1   0 for I-frame and 1 for P-frame
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, PictureCodingType, 1);
        // four_reserved_zero_bits      = 4 Bits        = 0
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 4);
    } else {
        static unsigned char  RTYPE = 0;

        // if I- Frame set Update Full Extended PTYPE to true
        if ((PictureCodingType == I_FRAME) || (SourceFormatType == 7) || OCPCF) {
            UFEP = 1;
        } else {
            UFEP = 0;
        }
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, UFEP, 3);
        if (UFEP == 1) {
            ptg__write_upto8bits_elements(mtx_hdr, elt_p, 6, 3);
            ptg__write_upto8bits_elements(mtx_hdr, elt_p, OCPCF , 1);

            /* 10 reserve bits ( Optional support for the encoding). All are OFF(0).
             *  - Optional Unrestricted Motion Vector (UMV)
             *  - Optional Syntax-based Arithmetic Coding (SAC)
             *  - Optional Advanced Prediction (AP) mode
             *  - Optional Advanced INTRA Coding (AIC) mode
             *  - Optional Deblocking Filter (DF) mode
             *  - Optional Slice Structured (SS) mode
             *  - Optional Reference Picture Selection(RPS) mode.
             *  - Optional Independent Segment Decoding (ISD) mode
             *  - Optional Alternative INTER VLC (AIV) mode
             *  - Optional Modified Quantization (MQ) mode */

            ptg__write_upto32bits_elements(mtx_hdr, elt_p, 0, 10);
            // 10 reserve bits
            ptg__write_upto8bits_elements(mtx_hdr, elt_p, 8, 4);
            // 4 reserve bits
        }
        // picture_coding_type          = 1 Bit         = 0/1   0 for I-frame and 1 for P-frame
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, PictureCodingType, 3);

        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
        // two_reserve_bits,      rounding_type,       two_reserve_bits       marker_bit       CPM
        // Rounding Type (RTYPE) (1 for P Picture, 0 for all other Picture frames.
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, RTYPE, 1);
        //2 reserve bits
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
        //   - 1 (ON) to prevent start code emulation.
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1 , 1);
        // CPM immediately follows the PPTYPE part of the header.
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0 , 1);


        if (UFEP == 1) {
            IMG_UINT16 ui16PWI, ui16PHI;
            ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);
            // aspect ratio
            //PictureWidth--;
            //ptg__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureWidth >> 8), 1);
            //ptg__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureWidth & 0xFF), 8);
            //Width = (PWI-1)*4, Height = PHI*4, see H263 spec 5.1.5
            ui16PWI = (PictureWidth >> 2) - 1;
            ptg__write_upto32bits_elements(mtx_hdr, elt_p, (IMG_UINT8)ui16PWI, 9);

            ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
            // marker_bit                               = 1 Bit         = 1
            //ptg__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureHeight >> 8), 1);
            //ptg__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureHeight & 0xFF), 8);

            ui16PHI = PictureHeight >> 2;
            ptg__write_upto32bits_elements(mtx_hdr, elt_p, (IMG_UINT8)ui16PHI, 9);
            // good up to that point
            //  ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
            // marker_bit                               = 1 Bit         = 1
            // just checking
            if (OCPCF == 1) {
                //IMG_UINT8 CPCFC;
                //CPCFC = (IMG_UINT8)(1800/(IMG_UINT16)FrameRate);
                /* you can use the table for division */
                //CPCFC <<= 1; /* for Clock Conversion Code */
                ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
                // Clock Divisor : 7 bits The natural binary representation of the value of the clock divisor.
                ptg__write_upto8bits_elements(mtx_hdr, elt_p, 1800000 / (FrameRate * 1000), 7);
            }
        }
        if (OCPCF == 1) {
            IMG_UINT8 ui8ETR; // extended Temporal reference
            // Two MSBs of 10 bit temporal_reference : value 0
            ui8ETR = Temporal_Ref >> 8;

            ptg__write_upto8bits_elements(mtx_hdr, elt_p, ui8ETR, 2);
            /* Two MSBs of temporal_reference */
        }
    }
    // vop_quant                                = 5 Bits        = x     5-bit frame Q_scale from rate control - GENERATED BY MTX
    //ptg__write_upto8bits_elements(mtx_hdr, elt_p, ui8Q_Scale, 5);
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_FRAMEQSCALE); // Insert token to tell MTX to insert rate-control value (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, FrameQScale))
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
    // zero_bit                                 = 1 Bit         = 0
    // pei                                              = 1 Bit         = 0     No direct effect on encoding of picture
    if (SourceFormatType != 7) {
        ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    ptg__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    // FOLLOWING SECTION CAN'T BE GENERATED HERE
    //gob_data( )
    //for(i=1; i<num_gob_in_picture; i++) {
    //      gob_header( )
    //      gob_data( )
    // }
    return;
}

static void H263_writebits_GOBSliceHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
    /* Essential we insert the element before we try to fill it! */
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* gob_resync_marker                = 17            = 0x1 */
    ptg__write_upto32bits_elements(mtx_hdr, elt_p, 1, 17);

    /* gob_number = 5   = 0-17  It is gob number in a picture */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, GOBNumber, 5);

    /* gob_frame_id     = 2 = 0-3       See note */
    ptg__write_upto8bits_elements(mtx_hdr, elt_p, GOBFrameId, 2);

    /* quant_scale      = 5     = 1-32  gob (Slice) Q_scale  */
    /* ptg__write_upto8bits_elements(mtx_hdr, elt_p, GOB_Q_Scale, 5); */

    /* Insert token to tell MTX to insert rate-control value
     *  (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, SliceQScale))
     */
    ptg__insert_element_token(mtx_hdr, elt_p, ELEMENT_SLICEQSCALE);
    return;
}

/*
 * High level functions to call when a H263 header is required - HOST ROUTINES
 */
//static void ptg__H263_getelements_videosequence_header(
//MTX_HEADER_PARAMS *mtx_hdr,
//IMG_UINT8 Profile_and_level_indication)
//{
///* Builds a single H263 video sequence header from the given parameters */
//
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//H263_writebits_VideoSequenceHeader(mtx_hdr, elt_p, Profile_and_level_indication);
//
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}

//static void ptg__H263_getelements_videopicture_header(
//MTX_HEADER_PARAMS *mtx_hdr,
//IMG_UINT8 Temporal_Ref,
//H263_PICTURE_CODING_TYPE PictureCodingType,
//H263_SOURCE_FORMAT_TYPE SourceFormatType,
//IMG_UINT8 FrameRate,
//IMG_UINT16 PictureWidth,
//IMG_UINT16 PictureHeigth)
//{
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//H263_writebits_VideoPictureHeader(
//mtx_hdr, elt_p,
//Temporal_Ref,
//PictureCodingType,
//SourceFormatType,
//FrameRate,
//PictureWidth,
//PictureHeigth);
//
//mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
//}

//static void ptg__H263_getelements_GOBslice_header(
//MTX_HEADER_PARAMS *mtx_hdr,
//IMG_UINT8 GOBNumber,
//IMG_UINT8 GOBFrameId)
//{
///* Essential we initialise our header structures before building */
//MTX_HEADER_ELEMENT *This_Element;
//MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
//mtx_hdr->Elements=ELEMENTS_EMPTY;
//This_Element=(MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
//elt_p[0]=This_Element;
//
//H263_writebits_GOBSliceHeader(mtx_hdr, elt_p, GOBNumber, GOBFrameId);
//
//mtx_hdr->Elements++; //Has been used as an index, so need to add 1 for a valid element count
//}

// SEI_INSERTION
static void ptg__H264ES_writebits_AUD_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers)
{
    // Essential we insert the element before we try to fill it!
    ptg__insert_element_token(pMTX_Header,
        aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);

    ptg__H264_writebits_startcode_prefix_element(pMTX_Header,
        aui32ElementPointers, 4); // 00 00 00 01 start code prefix

    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers, 9, 8); // AUD nal_unit_type = 09

    // primary_pic_type  u(3) 0=I slice, 1=P or I slice, 2=P,B or I slice
    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers, 2, 3);

    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers, 1 << 4, 5); // rbsp_trailing_bits

    // Write terminator
    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers, 0x80, 8);
    return;
}

static void ptg__H264ES_writebits_SEI_buffering_period_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT8 ui8NalHrdBpPresentFlag,
    IMG_UINT8 ui8nal_cpb_cnt_minus1,
    IMG_UINT8 ui8nal_initial_cpb_removal_delay_length,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay_offset,
    IMG_UINT8 ui8VclHrdBpPresentFlag,
    IMG_UINT8 ui8vcl_cpb_cnt_minus1,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay_offset)
{
    IMG_UINT8 ui8SchedSelIdx;
    IMG_UINT8 ui8PayloadSizeBits;
#ifdef SEI_NOT_USE_TOKEN_ALIGN
    IMG_UINT8 ui8Pad;
#endif
    // Essential we insert the element before we try to fill it!
    ptg__insert_element_token(pMTX_Header,
        aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);

    ptg__H264_writebits_startcode_prefix_element(pMTX_Header,
        aui32ElementPointers, 3); // 00 00 01 start code prefix

    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers, 6, 8); // nal_unit_type = 06 (SEI Message)

    ptg__write_upto8bits_elements(pMTX_Header,
        aui32ElementPointers, 0, 8); // SEI payload type (buffering period)

    ui8PayloadSizeBits = 1; // seq_parameter_set_id bitsize = 1
    if (ui8NalHrdBpPresentFlag)
        ui8PayloadSizeBits += ((ui8nal_cpb_cnt_minus1 + 1)
                               * ui8nal_initial_cpb_removal_delay_length * 2);
    if (ui8VclHrdBpPresentFlag)
        ui8PayloadSizeBits += ((ui8vcl_cpb_cnt_minus1 + 1)
                               * ui8nal_initial_cpb_removal_delay_length * 2);

    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  ((ui8PayloadSizeBits + 7) / 8),
                                  8);
    // SEI payload size = No of bytes required for SEI payload
    // (including seq_parameter_set_id)

    //seq_parameter_set_id      ue(v) = 0 default? = 1 (binary)
    //= sequence parameter set containing HRD attributes
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);

    if (ui8NalHrdBpPresentFlag) {
        for (ui8SchedSelIdx = 0; ui8SchedSelIdx <= ui8nal_cpb_cnt_minus1; ui8SchedSelIdx++) {
            // ui32nal_initial_cpb_removal_delay = delay between time of arrival in CODED PICTURE BUFFER of coded data of this access
            // unit and time of removal from CODED PICTURE BUFFER of the coded data of the same access unit.
            // Delay is based on the time taken for a 90 kHz clock.
            // Range >0 and < 90000 * (CPBsize / BitRate)
            // For the 1st buffering period after HARDWARE REFERENCE DECODER initialisation.
            // ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_NAL_INIT_CPB_REMOVAL_DELAY); // Eventually use this if firmware value required

            //ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers, ui32nal_initial_cpb_removal_delay, ui8nal_initial_cpb_removal_delay_length);
            ptg__insert_element_token(pMTX_Header,
                                      aui32ElementPointers,
                                      BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY);

            // ui32nal_initial_cpb_removal_delay_offset = used for the SchedSelIdx-th CPB in combination with the cpb_removal_delay to
            // specify the initial delivery time of coded access units to the CODED PICTURE BUFFER initial_cpb_removal_delay_offset
            // Delay is based on the time taken for a 90 kHz clock.
            // NOT USED BY DECODERS and is needed only for the delivery scheduler (HSS) specified in Annex C

            ptg__insert_element_token(pMTX_Header,
                                      aui32ElementPointers,
                                      BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET);
        }
    }
    if (ui8VclHrdBpPresentFlag) {
        for (ui8SchedSelIdx = 0; ui8SchedSelIdx <= ui8vcl_cpb_cnt_minus1; ui8SchedSelIdx++) {
            ptg__insert_element_token(pMTX_Header,
                                      aui32ElementPointers,
                                      ELEMENT_STARTCODE_RAWDATA);
            // ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_VCL_INIT_CPB_REMOVAL_DELAY); // Eventually use this if firmware value required
            ptg__write_upto32bits_elements(pMTX_Header,
                                           aui32ElementPointers,
                                           ui32vcl_initial_cpb_removal_delay,
                                           ui8nal_initial_cpb_removal_delay_length);
            // ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_VCL_INIT_CPB_REMOVAL_DELAY_CPB); // Eventually use this if firmware value required
            ptg__write_upto32bits_elements(pMTX_Header,
                                           aui32ElementPointers,
                                           ui32vcl_initial_cpb_removal_delay_offset,
                                           ui8nal_initial_cpb_removal_delay_length);
        }
    }

    // Pad to end of byte
#ifdef SEI_NOT_USE_TOKEN_ALIGN
    if (!ui8VclHrdBpPresentFlag)
        ptg__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  ELEMENT_STARTCODE_RAWDATA);
    ui8Pad = (ui8PayloadSizeBits + 7) / 8;
    ui8Pad = (ui8Pad * 8) - ui8PayloadSizeBits;
    if (ui8Pad > 0)
        ptg__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      1 << (ui8Pad - 1),
                                      ui8Pad); // SEI payload type (buffering period)
#else
    ptg__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_INSERTBYTEALIGN_H264);
    // Tell MTX to insert the byte align field
    ptg__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);
#endif

    // Write terminator
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0x80, 8);

    return;
}


static void ptg__H264ES_writebits_SEI_picture_timing_header(
    MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT8 ui8CpbDpbDelaysPresentFlag,
    IMG_UINT32 ui32cpb_removal_delay_length_minus1,
    IMG_UINT32 ui32dpb_output_delay_length_minus1,
    IMG_UINT32 ui32cpb_removal_delay,
    IMG_UINT32 ui32dpb_output_delay,
    IMG_UINT8 ui8pic_struct_present_flag,
    IMG_UINT8 ui8pic_struct,
    IMG_UINT8 ui8NumClockTS,
    IMG_UINT8 *aui8clock_timestamp_flag,
    IMG_UINT8 ui8full_timestamp_flag,
    IMG_UINT8 ui8seconds_flag,
    IMG_UINT8 ui8minutes_flag,
    IMG_UINT8 ui8hours_flag,
    IMG_UINT8 ui8seconds_value,
    IMG_UINT8 ui8minutes_value,
    IMG_UINT8 ui8hours_value,
    IMG_UINT8 ui8ct_type,
    IMG_UINT8 ui8nuit_field_based_flag,
    IMG_UINT8 ui8counting_type,
    IMG_UINT8 ui8discontinuity_flag,
    IMG_UINT8 ui8cnt_dropped_flag,
    IMG_UINT8 ui8n_frames,
    IMG_UINT8 ui8time_offset_length,
    IMG_UINT32 i32time_offset)
{
    IMG_UINT8 ui8PayloadSizeBits, ui8Tmp;
#ifdef SEI_NOT_USE_TOKEN_ALIGN
    IMG_UINT8 ui8Pad;
#endif

    // Essential we insert the element before we try to fill it!
    ptg__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);

    ptg__H264_writebits_startcode_prefix_element(pMTX_Header,
            aui32ElementPointers,
            3); // 00 00 01 start code prefix

    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  6, 8); // nal_unit_type = 06 (SEI Message)

    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  1, 8); // SEI payload type (picture timing)


    // Precalculate the payload bit size
    ui8PayloadSizeBits = 0;
    if (ui8CpbDpbDelaysPresentFlag)
        ui8PayloadSizeBits += ui32cpb_removal_delay_length_minus1
                              + 1 + ui32dpb_output_delay_length_minus1 + 1;

    if (ui8pic_struct_present_flag) {
        ui8PayloadSizeBits += 4;
        for (ui8Tmp = 0; ui8Tmp < ui8NumClockTS ; ui8Tmp++) {
            ui8PayloadSizeBits += 1;

            if (aui8clock_timestamp_flag[ui8Tmp]) {
                ui8PayloadSizeBits += 2 + 1 + 5 + 1 + 1 + 1 + 8;
                if (ui8full_timestamp_flag)
                    ui8PayloadSizeBits += 6 + 6 + 5;
                else {
                    ui8PayloadSizeBits += 1;
                    if (ui8seconds_flag) {
                        ui8PayloadSizeBits += 6 + 1;
                        if (ui8minutes_flag) {
                            ui8PayloadSizeBits += 6 + 1;
                            if (ui8hours_flag)
                                ui8PayloadSizeBits += 5;
                        }
                    }
                }

                if (ui8time_offset_length > 0)
                    ui8PayloadSizeBits += ui8time_offset_length;
            }
        }
    }

    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  ((ui8PayloadSizeBits + 7) / 8), 8);
    // SEI payload size = No of bytes required for SEI payload (including seq_parameter_set_id)


    if (ui8CpbDpbDelaysPresentFlag) {
        //SEI_INSERTION
#ifdef SEI_HOSTCALC_CPB_DPB
        ptg__write_upto32bits_elements(pMTX_Header,
                                       aui32ElementPointers,
                                       ui32cpb_removal_delay,
                                       ui32cpb_removal_delay_length_minus1 + 1); // cpb_removal_delay
        ptg__write_upto32bits_elements(pMTX_Header,
                                       aui32ElementPointers,
                                       ui32dpb_output_delay,
                                       ui32dpb_output_delay_length_minus1 + 1); // dpb_output_delay
#else
        ptg__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  PTH_SEI_NAL_CPB_REMOVAL_DELAY);
        ptg__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  PTH_SEI_NAL_DPB_OUTPUT_DELAY);
#endif
    }

    if (ui8pic_struct_present_flag) {
        ptg__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  ELEMENT_STARTCODE_RAWDATA);
        ptg__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      ui8pic_struct, 4); // See TRM able D 1 ?Interpretation of pic_struct

        for (ui8Tmp = 0; ui8Tmp < ui8NumClockTS ; ui8Tmp++) {
            ptg__write_upto8bits_elements(pMTX_Header,
                                          aui32ElementPointers,
                                          aui8clock_timestamp_flag[ui8Tmp], 1);

            if (aui8clock_timestamp_flag[ui8Tmp]) {
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8ct_type, 2);
                // (2=Unknown) See TRM Table D 2 ?Mapping of ct_type to source picture scan
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8nuit_field_based_flag, 1);
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8counting_type, 5);
                // See TRM Table D 3 ?Definition of counting_type values
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8full_timestamp_flag, 1);
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8discontinuity_flag, 1);
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8cnt_dropped_flag, 1);
                ptg__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8n_frames, 8);

                if (ui8full_timestamp_flag) {
                    ptg__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8seconds_value, 6); // 0 - 59
                    ptg__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8minutes_value, 6); // 0 - 59
                    ptg__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8hours_value, 5); // 0 - 23
                } else {
                    ptg__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8seconds_flag, 1);

                    if (ui8seconds_flag) {
                        ptg__write_upto8bits_elements(pMTX_Header,
                                                      aui32ElementPointers,
                                                      ui8seconds_value, 6); // 0 - 59
                        ptg__write_upto8bits_elements(pMTX_Header,
                                                      aui32ElementPointers,
                                                      ui8minutes_flag, 1);

                        if (ui8minutes_flag) {
                            ptg__write_upto8bits_elements(pMTX_Header,
                                                          aui32ElementPointers,
                                                          ui8minutes_value, 6); // 0 - 59
                            ptg__write_upto8bits_elements(pMTX_Header,
                                                          aui32ElementPointers,
                                                          ui8hours_flag, 1);

                            if (ui8hours_flag)
                                ptg__write_upto8bits_elements(pMTX_Header,
                                                              aui32ElementPointers,
                                                              ui8hours_value, 5); // 0 - 23
                        }
                    }
                }

                if (ui8time_offset_length > 0) {
                    // Two's complement storage : If time_offset<0 = ((2 ^ v) + time_offset)
                    if (i32time_offset < 0)
                        ptg__write_upto32bits_elements(pMTX_Header,
                                                       aui32ElementPointers,
                                                       (IMG_UINT32)((2 ^ ui8time_offset_length) + i32time_offset),
                                                       ui8time_offset_length);
                    else
                        ptg__write_upto32bits_elements(pMTX_Header,
                                                       aui32ElementPointers,
                                                       (IMG_UINT32) i32time_offset,
                                                       ui8time_offset_length);
                }
            }
        }
    }

#ifdef SEI_NOT_USE_TOKEN_ALIGN
    // Pad to end of byte
    if (!ui8pic_struct_present_flag)
        ptg__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  ELEMENT_STARTCODE_RAWDATA);
    ui8Pad = (ui8PayloadSizeBits + 7) / 8;
    ui8Pad = (ui8Pad * 8) - ui8PayloadSizeBits;
    if (ui8Pad > 0)
        ptg__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      1 << (ui8Pad - 1),
                                      ui8Pad); // SEI payload type (buffering period)
#else
    ptg__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_INSERTBYTEALIGN_H264); // Tell MTX to insert the byte align field
    ptg__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);
#endif

    // Write terminator
    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  0x80, 8);
    return;
}



void ptg__H264ES_prepare_AUD_header(unsigned char *virtual_addr)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_PARAMS * pMTX_Header = (MTX_HEADER_PARAMS *)virtual_addr;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    ptg__H264ES_writebits_AUD_header(pMTX_Header, aui32ElementPointers);

    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
}


void ptg__H264ES_prepare_SEI_buffering_period_header(
    unsigned char *virtual_addr,
    IMG_UINT8 ui8nal_cpb_cnt_minus1,
    IMG_UINT8 ui8nal_initial_cpb_removal_delay_length,
    IMG_UINT8 ui8NalHrdBpPresentFlag,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay_offset,
    IMG_UINT8 ui8VclHrdBpPresentFlag,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay_offset)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_PARAMS * pMTX_Header = (MTX_HEADER_PARAMS *)virtual_addr;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    ptg__H264ES_writebits_SEI_buffering_period_header(
        pMTX_Header, aui32ElementPointers,
        ui8NalHrdBpPresentFlag,
        ui8nal_cpb_cnt_minus1,
        ui8nal_initial_cpb_removal_delay_length,
        ui32nal_initial_cpb_removal_delay,
        ui32nal_initial_cpb_removal_delay_offset,
        ui8VclHrdBpPresentFlag,
        ui8nal_cpb_cnt_minus1,
        ui32vcl_initial_cpb_removal_delay,
        ui32vcl_initial_cpb_removal_delay_offset);

    pMTX_Header->ui32Elements++;
    //Has been used as an index, so need to add 1 for a valid element count
    return;
}

void ptg__H264ES_prepare_SEI_picture_timing_header(
    unsigned char *virtual_addr,
    IMG_UINT8 ui8CpbDpbDelaysPresentFlag,
    IMG_UINT32 ui32cpb_removal_delay_length_minus1,
    IMG_UINT32 ui32dpb_output_delay_length_minus1,
    IMG_UINT32 ui32cpb_removal_delay,
    IMG_UINT32 ui32dpb_output_delay,
    IMG_UINT8 ui8pic_struct_present_flag,
    IMG_UINT8 ui8pic_struct,
    IMG_UINT8 ui8NumClockTS,
    IMG_UINT8 *aui8clock_timestamp_flag,
    IMG_UINT8 ui8full_timestamp_flag,
    IMG_UINT8 ui8seconds_flag,
    IMG_UINT8 ui8minutes_flag,
    IMG_UINT8 ui8hours_flag,
    IMG_UINT8 ui8seconds_value,
    IMG_UINT8 ui8minutes_value,
    IMG_UINT8 ui8hours_value,
    IMG_UINT8 ui8ct_type,
    IMG_UINT8 ui8nuit_field_based_flag,
    IMG_UINT8 ui8counting_type,
    IMG_UINT8 ui8discontinuity_flag,
    IMG_UINT8 ui8cnt_dropped_flag,
    IMG_UINT8 ui8n_frames,
    IMG_UINT8 ui8time_offset_length,
    IMG_INT32 i32time_offset)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_PARAMS * pMTX_Header = (MTX_HEADER_PARAMS *)virtual_addr;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    ptg__H264ES_writebits_SEI_picture_timing_header(
        pMTX_Header, aui32ElementPointers,
        ui8CpbDpbDelaysPresentFlag,
        ui32cpb_removal_delay_length_minus1,
        ui32dpb_output_delay_length_minus1,
        ui32cpb_removal_delay,
        ui32dpb_output_delay,
        ui8pic_struct_present_flag,
        ui8pic_struct,
        ui8NumClockTS,
        aui8clock_timestamp_flag,
        ui8full_timestamp_flag,
        ui8seconds_flag,
        ui8minutes_flag,
        ui8hours_flag,
        ui8seconds_value,
        ui8minutes_value,
        ui8hours_value,
        ui8ct_type,
        ui8nuit_field_based_flag,
        ui8counting_type,
        ui8discontinuity_flag,
        ui8cnt_dropped_flag,
        ui8n_frames,
        ui8time_offset_length,
        i32time_offset);

    pMTX_Header->ui32Elements++;
    //Has been used as an index, so need to add 1 for a valid element count
    return;
}

static void ptg__H264ES_set_sequence_level_profile(
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    IMG_UINT8 uiLevel,
    IMG_UINT8 uiProfile)
{
    switch (uiLevel) {
    case 10:
        pSHParams->ucLevel =  SH_LEVEL_10;
        break;
    case 111:
        pSHParams->ucLevel =  SH_LEVEL_1B;
        break;
    case 11:
        pSHParams->ucLevel =  SH_LEVEL_11;
        break;
    case 12:
        pSHParams->ucLevel =  SH_LEVEL_12;
        break;
    case 20:
        pSHParams->ucLevel =  SH_LEVEL_20;
        break;
    case 30:
        pSHParams->ucLevel =  SH_LEVEL_30;
        break;
    case 31:
        pSHParams->ucLevel =  SH_LEVEL_31;
        break;
    case 32:
        pSHParams->ucLevel =  SH_LEVEL_32;
        break;
    case 40:
        pSHParams->ucLevel =  SH_LEVEL_40;
        break;
    case 41:
        pSHParams->ucLevel =  SH_LEVEL_41;
        break;
    case 42:
        pSHParams->ucLevel =  SH_LEVEL_42;
        break;
    default:
        pSHParams->ucLevel =  SH_LEVEL_30;
        break;
    }

    switch (uiProfile) {
    case 5:
        pSHParams->ucProfile  = SH_PROFILE_BP;
        break;
    case 6:
        pSHParams->ucProfile  = SH_PROFILE_MP;
        break;
    default:
        pSHParams->ucProfile  = SH_PROFILE_MP;
        break;
    }
    return ;
}

#ifdef _PDUMP_SEQ_HEADER
void ptg_trace_seq_header_params(H264_SEQUENCE_HEADER_PARAMS *psSHParams)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s ucProfile                         = %x\n", __FUNCTION__, psSHParams->ucProfile);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s ucLevel                           = %x\n", __FUNCTION__, psSHParams->ucLevel);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s ucWidth_in_mbs_minus1             = %x\n", __FUNCTION__, psSHParams->ucWidth_in_mbs_minus1);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s ucHeight_in_maps_units_minus1     = %x\n", __FUNCTION__, psSHParams->ucHeight_in_maps_units_minus1);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s log2_max_pic_order_cnt            = %x\n", __FUNCTION__, psSHParams->log2_max_pic_order_cnt);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s max_num_ref_frames                = %x\n", __FUNCTION__, psSHParams->max_num_ref_frames);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s gaps_in_frame_num_value           = %x\n", __FUNCTION__, psSHParams->gaps_in_frame_num_value);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s ucFrame_mbs_only_flag             = %x\n", __FUNCTION__, psSHParams->ucFrame_mbs_only_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s VUI_Params_Present                = %x\n", __FUNCTION__, psSHParams->VUI_Params_Present);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s seq_scaling_matrix_present_flag   = %x\n", __FUNCTION__, psSHParams->seq_scaling_matrix_present_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s bUseDefaultScalingList            = %x\n", __FUNCTION__, psSHParams->bUseDefaultScalingList);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s bIsLossless                       = %x\n", __FUNCTION__, psSHParams->bIsLossless);
  if (psSHParams->VUI_Params_Present) {
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s Time_Scale            = %x\n", __FUNCTION__, psSHParams->VUI_Params.Time_Scale);            //!< Time scale as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s bit_rate_value_minus1 = %x\n", __FUNCTION__, psSHParams->VUI_Params.bit_rate_value_minus1); //!< An inter framebitrate/64)-1
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s cbp_size_value_minus1 = %x\n", __FUNCTION__, psSHParams->VUI_Params.cbp_size_value_minus1); //!< An inter frame(bitrate*1.5)/16
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s CBR                   = %x\n", __FUNCTION__, psSHParams->VUI_Params.CBR);                   //!< CBR as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s initial_cpb_removal   = %x\n", __FUNCTION__, psSHParams->VUI_Params.initial_cpb_removal_delay_length_minus1); //!< as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s cpb_removal_delay_length_minus1 = %x\n", __FUNCTION__, psSHParams->VUI_Params.cpb_removal_delay_length_minus1); //!< as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s dpb_output_delay_length_minus1  = %x\n", __FUNCTION__, psSHParams->VUI_Params.dpb_output_delay_length_minus1);  //!< as defined in the H.264 specification
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s time_offset_length              = %x\n", __FUNCTION__, psSHParams->VUI_Params.time_offset_length);              //!< as defined in the H.264 specification
    }
}
#endif

void ptg__H264ES_prepare_sequence_header(
    void *pHeaderMemory,
    H264_VUI_PARAMS *psVUI_Params,
    H264_CROP_PARAMS *psCropParams,
    IMG_UINT16 ui16PictureWidth,
    IMG_UINT16 ui16PictureHeight,
    IMG_UINT32 ui32CustomQuantMask,
    IMG_UINT8 ui8ProfileIdc,
    IMG_UINT8 ui8LevelIdc,
    IMG_UINT8 ui8FieldCount,
    IMG_UINT8 ui8MaxNumRefFrames,
    IMG_BOOL  bPpsScaling,
    IMG_BOOL  bUseDefaultScalingList,
    IMG_BOOL  bEnableLossless,
    IMG_BOOL  bASO
)
{
    /* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
     * Essential we initialise our header structures before building
     */
    H264_SEQUENCE_HEADER_PARAMS SHParams = {0, };
    /* Route output elements to memory provided */
    MTX_HEADER_PARAMS  *mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    memset(&SHParams, 0, sizeof(H264_SEQUENCE_HEADER_PARAMS));

    SHParams.ucProfile = ui8ProfileIdc - 5;
    SHParams.ucLevel = (ui8LevelIdc != 111) ? ui8LevelIdc : SH_LEVEL_1B;
    SHParams.ucWidth_in_mbs_minus1 = (IMG_UINT8)((ui16PictureWidth >> 4)- 1);
    SHParams.ucHeight_in_maps_units_minus1 = (IMG_UINT8)((ui16PictureHeight >> 4) - 1);
    SHParams.gaps_in_frame_num_value = IMG_FALSE;
    SHParams.VUI_Params_Present = psVUI_Params->vui_flag;
    if (SHParams.VUI_Params_Present)
        memcpy(&SHParams.VUI_Params, psVUI_Params, sizeof(H264_VUI_PARAMS));
    SHParams.ucFrame_mbs_only_flag = (ui8FieldCount > 1) ? IMG_FALSE : IMG_TRUE;
    SHParams.seq_scaling_matrix_present_flag = (bPpsScaling) ? IMG_FALSE : (IMG_UINT8)(ui32CustomQuantMask);
    SHParams.bUseDefaultScalingList = (bPpsScaling) ? IMG_FALSE : bUseDefaultScalingList;
    SHParams.max_num_ref_frames = ui8MaxNumRefFrames;
    SHParams.bIsLossless = bEnableLossless;
    SHParams.log2_max_pic_order_cnt = 6;

    //drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: ui8_level = %d, ucLevel = %d\n", __FUNCTION__, ui8_level, SHParams.ucLevel);

#ifdef _PDUMP_SEQ_HEADER
    ptg_trace_seq_header_params(&SHParams);
#endif
    ptg__H264ES_writebits_sequence_header(mtx_hdr, elt_p, &SHParams, psCropParams, NULL, bASO);
    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

static void ptg__H264ES_writebits_mvc_sequence_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    H264_CROP_PARAMS *psCrop,
    H264_SCALING_MATRIX_PARAMS * psScalingMatrix)
{
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
    ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, 4);

    ///**** GENERATES THE FIRST ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///

    // 4 Byte StartCodePrefix Pregenerated in: H264_WriteBits_StartCodePrefix_Element()
    // Byte aligned (bit 32)
    ptg__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers, (0 << 7) |                                                        // forbidden_zero_bit=0
                                  (0x3 << 5) |                                                                            // nal_ref_idc=01 (may be 11)
                                  (15),                                                                                           // nal_unit_type=15
                                  8);

    // Byte aligned (bit 40)
    // profile_idc = 8 bits = 66 for BP (PROFILE_IDC_BP), 77 for MP (PROFILE_IDC_MP)
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 118, 8);

    // Byte aligned (bit 48)
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (0 << 7) |     // constrain_set0_flag = 1 for MP + BP constraints
                                  (0 << 6) |                                                  // constrain_set1_flag  = 1 for MP + BP constraints
                                  (0 << 5) |                                                                                              // constrain_set2_flag = always 0 in BP/MP
                                  (0 << 4),                                                           // constrain_set3_flag = 1 for level 1b, 0 for others
                                  // reserved_zero_4bits = 0
                                  8);

    // Byte aligned (bit 56)
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (pSHParams->ucLevel == SH_LEVEL_1B) ? 11 : (IMG_UINT8)pSHParams->ucLevel, 8);                  // level_idc (8 bits) = 11 for 1b, 10xlevel for others

    ptg__generate_ue(pMTX_Header, aui32ElementPointers, MVC_SPS_ID);                // seq_parameter_Set_id = 1 FOR subset-SPS
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);         // chroma_format_idc = 1
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);         // bit_depth_luma_minus8 = 0
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);         // bit_depth_chroma_minus8 = 0
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSHParams->bIsLossless ? 1 : 0, 1); // qpprime_y_zero_transform_bypass_flag = 0

    if (pSHParams->bUseDefaultScalingList || pSHParams->seq_scaling_matrix_present_flag) {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);         // seq_scaling_matrix_present_flag
        if (!pSHParams->bUseDefaultScalingList) {
#ifdef _FIXME_
            H264_WriteBits_ScalingLists(pMTX_Header, aui32ElementPointers, psScalingMatrix, IMG_TRUE);
#endif
            ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        } else {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);         // seq_scaling_list_present_flag[i] = 0; 0 < i < 8
        }
    } else {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);         // seq_scaling_matrix_present_flag
    }

    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);         // log2_max_frame_num_minus4 = 1
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);         // pic_order_cnt_type = 0
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 2);         // log2_max_pic_order_cnt_Isb_minus4 = 2

    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->max_num_ref_frames); //num_ref_frames ue(2), typically 2
    // Bytes aligned (bit 72)
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                  (pSHParams->gaps_in_frame_num_value), // gaps_in_frame_num_value_allowed_Flag   - (1 bit)
                                  1);

    ///**** GENERATES THE SECOND, VARIABLE LENGTH, ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: xx
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->ucWidth_in_mbs_minus1);                          //pic_width_in_mbs_minus1: ue(v) from 10 to 44 (176 to 720 pixel per row)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->ucHeight_in_maps_units_minus1);          //pic_height_in_maps_units_minus1: ue(v) Value from 8 to 35 (144 to 576 pixels per column)
    // We don't know the alignment at this point, so will have to use bit writing functions

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSHParams->ucFrame_mbs_only_flag, 1); // frame_mb_only_flag 1=frame encoding, 0=field encoding

    if (!pSHParams->ucFrame_mbs_only_flag) // in the case of interlaced encoding
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1); // mb_adaptive_frame_field_flag = 0 in Topaz(field encoding at the sequence level)

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); // direct_8x8_inference_flag=1 in Topaz

    if (psCrop->bClip) {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16LeftCropOffset);
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16RightCropOffset);
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16TopCropOffset);
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->ui16BottomCropOffset);

    } else {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
    }

    ///**** GENERATES THE THIRD ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: xx
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                  (pSHParams->VUI_Params_Present),                                        // vui_parameters_present_flag (VUI only in 1st sequence of stream)
                                  1);
    if (pSHParams->VUI_Params_Present > 0)
        ptg__H264_writebits_VUI_params(pMTX_Header, aui32ElementPointers, &(pSHParams->VUI_Params));


    {
        int viewIdx = 0;
        int numViews = MAX_MVC_VIEWS;
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); //bit_equal_to_one

        // sequence parameter set MVC extension
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, (numViews - 1));     //num_views_minus1
        for (viewIdx = 0; viewIdx < numViews; viewIdx++) {
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, viewIdx);
        }

        // anchor references
        for (viewIdx = 1; viewIdx < numViews; viewIdx++) {
            //ptg__generate_ue( pMTX_Header, aui32ElementPointers, 0);     // num_anchor_refs_l0  = 0
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);      // num_anchor_refs_l0  = 1; view-1 refers to view-0
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);      // anchor_ref_l0 = 0

            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);      // num_anchor_refs_l1  = 0
        }

        // non-anchor references
        for (viewIdx = 1; viewIdx < numViews; viewIdx++) {
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);      // num_non_anchor_refs_l0  = 0
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);      // non_anchor_refs_l0  = 0

            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);      // num_non_anchor_refs_l1  = 0
        }

        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // num_level_values_signaled_minus1  = 0

        //for(levelIdx=0; levelIdx<= 0; levelIdx++)
        {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, (pSHParams->ucLevel == SH_LEVEL_1B) ? 11 : (IMG_UINT8)pSHParams->ucLevel, 8);          // level_idc (8 bits) = 11 for 1b, 10xlevel for others
            ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // num_applicable_ops_minus1  = 0
            {
                ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 3); // applicable_ops_temporal_id  = 0
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);                  // applicable_op_num_target_views_minus1  = 0
                {
                    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);              // applicable_op_target_view_id  = 0
                }
                ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);                  // applicable_op_num_views_minus1  = 0
            }
        }

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                      0,                    // mvc_vui_parameters_present_flag =0
                                      1);

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                      0,                    // additional_extension2_flag =0
                                      1);
    }


    // Finally we need to align to the next byte
    // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_H264);
}


/*
******************************************************************************
 @Function              H264_PrepareMvcSequenceHeader
 @details
 Prepare an H264 SPS in a form for the MTX to encode into a bitstream.
 @param    pMTX_Header       : pointer to header structure to populate
 @param    uiPicWidthInMbs   : picture width in MBs
 @param    uiPicHeightInMbs  : picture height in MBs
 @param    bVuiParamsPresent : IMG_TRUE if VUI paramters present
 @param    psParams          : VUI parameters
 @param    psCrop                         : Pointer to crop parameter structure
 @param   psSHParams              : Pointer to sequence header params structure
 @return   None
******************************************************************************/
void ptg__H264ES_prepare_mvc_sequence_header(
    void *pHeaderMemory,
    H264_CROP_PARAMS *psCropParams,
    IMG_UINT16 ui16PictureWidth,
    IMG_UINT16 ui16PictureHeight,
    IMG_UINT32 ui32CustomQuantMask,
    IMG_UINT8 ui8ProfileIdc,
    IMG_UINT8 ui8LevelIdc,
    IMG_UINT8 ui8FieldCount,
    IMG_UINT8 ui8MaxNumRefFrames,
    IMG_BOOL  bPpsScaling,
    IMG_BOOL  bUseDefaultScalingList,
    IMG_BOOL  bEnableLossless,
    IMG_BOOL  bASO)
{
    H264_SEQUENCE_HEADER_PARAMS sSHParams = {0, };
    MTX_HEADER_PARAMS * pMTX_Header;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header = (MTX_HEADER_PARAMS *) pHeaderMemory;

#if HEADERS_VERBOSE_OUTPUT
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("\n\n**********************************************************************\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("******** HOST FIRMWARE ROUTINES TO PASS HEADERS AND TOKENS TO MTX******\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("**********************************************************************\n\n");
#endif

    // Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
    // Essential we initialise our header structures before building
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

#ifdef _PDUMP_FUNC_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s start\n", __FUNCTION__);
#endif


    memset(&sSHParams, 0, sizeof(H264_SEQUENCE_HEADER_PARAMS));

    sSHParams.ucProfile = ui8ProfileIdc - 5;
    sSHParams.ucLevel = (ui8LevelIdc != 111) ? ui8LevelIdc : SH_LEVEL_1B;
    sSHParams.ucWidth_in_mbs_minus1 = (IMG_UINT8)((ui16PictureWidth >> 4)- 1);
    sSHParams.ucHeight_in_maps_units_minus1 = (IMG_UINT8)((ui16PictureHeight >> 4) - 1);
    sSHParams.gaps_in_frame_num_value = IMG_FALSE;
    sSHParams.VUI_Params_Present = IMG_FALSE;
    sSHParams.ucFrame_mbs_only_flag = (ui8FieldCount > 1) ? IMG_FALSE : IMG_TRUE;
    sSHParams.seq_scaling_matrix_present_flag = (bPpsScaling) ? IMG_FALSE : (IMG_UINT8)(ui32CustomQuantMask);
    sSHParams.bUseDefaultScalingList = (bPpsScaling) ? IMG_FALSE : bUseDefaultScalingList;
    sSHParams.max_num_ref_frames = ui8MaxNumRefFrames;
    sSHParams.bIsLossless = bEnableLossless;
    sSHParams.log2_max_pic_order_cnt = 6;

#ifdef _PDUMP_SEQ_HEADER
    ptg_trace_seq_header_params(&sSHParams);
#endif

    ptg__H264ES_writebits_mvc_sequence_header(pMTX_Header, aui32ElementPointers, &sSHParams, psCropParams, NULL);
    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
#ifdef _PDUMP_FUNC_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s end\n", __FUNCTION__);
#endif

}

static void ptg_trace_pic_header_params(H264_PICTURE_HEADER_PARAMS *psSHParams)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s pic_parameter_set_id;           = %x\n", __FUNCTION__, psSHParams->pic_parameter_set_id);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s seq_parameter_set_id;           = %x\n", __FUNCTION__, psSHParams->seq_parameter_set_id);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s entropy_coding_mode_flag;       = %x\n", __FUNCTION__, psSHParams->entropy_coding_mode_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s weighted_pred_flag;             = %x\n", __FUNCTION__, psSHParams->weighted_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s weighted_bipred_idc;            = %x\n", __FUNCTION__, psSHParams->weighted_bipred_idc);           
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s chroma_qp_index_offset;         = %x\n", __FUNCTION__, psSHParams->chroma_qp_index_offset);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s constrained_intra_pred_flag;    = %x\n", __FUNCTION__, psSHParams->constrained_intra_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s transform_8x8_mode_flag;        = %x\n", __FUNCTION__, psSHParams->transform_8x8_mode_flag);  
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s pic_scaling_matrix_present_flag = %x\n", __FUNCTION__, psSHParams->pic_scaling_matrix_present_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s bUseDefaultScalingList;         = %x\n", __FUNCTION__, psSHParams->bUseDefaultScalingList);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s second_chroma_qp_index_offset;  = %x\n", __FUNCTION__, psSHParams->second_chroma_qp_index_offset);

}

void ptg__H264ES_prepare_picture_header(
    void *pHeaderMemory,
    IMG_BOOL    bCabacEnabled,
    IMG_BOOL    b_8x8transform,
    IMG_BOOL    bIntraConstrained,
    IMG_INT8    i8CQPOffset,
    IMG_BOOL    bWeightedPrediction,
    IMG_UINT8   ui8WeightedBiPred,
    IMG_BOOL    bMvcPPS,
    IMG_BOOL    bScalingMatrix,
    IMG_BOOL    bScalingLists)
{
    MTX_HEADER_PARAMS *pMTX_Header;
    H264_PICTURE_HEADER_PARAMS sPHParams = {0,};

    /* Route output elements to memory provided */
    pMTX_Header = (MTX_HEADER_PARAMS *) pHeaderMemory;
    /* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;
#ifdef _PDUMP_FUNC_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: start\n",__FUNCTION__);
#endif
    sPHParams.pic_parameter_set_id = bMvcPPS ? MVC_PPS_ID : 0;
    sPHParams.seq_parameter_set_id = bMvcPPS ? MVC_SPS_ID : 0;
    sPHParams.entropy_coding_mode_flag = bCabacEnabled ? 1 : 0;
    sPHParams.weighted_pred_flag = bWeightedPrediction;
    sPHParams.weighted_bipred_idc = ui8WeightedBiPred;
    sPHParams.chroma_qp_index_offset = i8CQPOffset;
    sPHParams.constrained_intra_pred_flag = bIntraConstrained ? 1 : 0;
    sPHParams.transform_8x8_mode_flag = b_8x8transform ? 1 : 0;
    sPHParams.pic_scaling_matrix_present_flag = bScalingMatrix ? 1 : 0;
    sPHParams.bUseDefaultScalingList = !bScalingLists;
    sPHParams.second_chroma_qp_index_offset = i8CQPOffset;

#ifdef _TOPAZHP_PDUMP_PICTURE_
  ptg_trace_pic_header_params(&sPHParams);
#endif
    ptg__H264ES_writebits_picture_header(pMTX_Header, aui32ElementPointers, &sPHParams, NULL);
    /* Has been used as an index, so need to add 1 for a valid element count */
    pMTX_Header->ui32Elements++;
#ifdef _PDUMP_FUNC_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: end\n",__FUNCTION__);
#endif
}

void ptg__H264_prepare_slice_header(
    IMG_UINT32 *pHeaderMemory,
    IMG_BOOL        bIntraSlice,
    IMG_BOOL        bInterBSlice,
    IMG_BOOL        bMultiRef,
    IMG_UINT8       ui8DisableDeblockingFilterIDC,
    IMG_UINT32      ui32DisplayFrameNumber,
    IMG_UINT32      ui32FrameNumId,
    IMG_UINT32      uiFirst_MB_Address,
    IMG_UINT32      uiMBSkipRun,
    IMG_BOOL        bCabacEnabled,
    IMG_BOOL        bIsInterlaced,
    IMG_UINT8       ui8FieldNum,
    WEIGHTED_PREDICTION_VALUES *pWeightedSetup,
    IMG_BOOL        bIsLongTermRef
)
{
    H264_SLICE_HEADER_PARAMS SlHParams = {0};
    MTX_HEADER_PARAMS *pMTX_Header;
    /* Route output elements to memory provided */
    pMTX_Header = (MTX_HEADER_PARAMS *) pHeaderMemory;

    //MTX_HEADER_PARAMS *pMTX_Header;
    //// Route output elements to memory provided
    //pMTX_Header = (MTX_HEADER_PARAMS *) pHeaderMemory;
    SlHParams.ui8Start_Code_Prefix_Size_Bytes = 4;
    /* pcb -        I think that this is more correct now*/
    SlHParams.SliceFrame_Type       = bIntraSlice ? (((ui32FrameNumId % (1 << 5)) == 0) ?
                                      SLHP_IDR_SLICEFRAME_TYPE : SLHP_I_SLICEFRAME_TYPE) : (bInterBSlice ? SLHP_B_SLICEFRAME_TYPE : SLHP_P_SLICEFRAME_TYPE);
    /*
                    if (bIntraSlice) {
                            if ((ui32FrameNumId%(1<<5))==0)
                                    SlHParams.SliceFrame_Type = SLHP_IDR_SLICEFRAME_TYPE;
                            else
                                    SlHParams.SliceFrame_Type = SLHP_I_SLICEFRAME_TYPE;
                    } else {
                            if (bInterBSlice)
                                    SlHParams.SliceFrame_Type = SLHP_B_SLICEFRAME_TYPE;
                            else
                                    SlHParams.SliceFrame_Type = SLHP_P_SLICEFRAME_TYPE;
                    }
    */
    SlHParams.Frame_Num_DO   = (IMG_UINT8) ui32FrameNumId % (1 << 5);
    SlHParams.Idr_Pic_Id     = (IMG_UINT8)(ui32DisplayFrameNumber & 1);
    SlHParams.Picture_Num_DO = (IMG_UINT8)((ui32DisplayFrameNumber % (1 << 5)) * 2);

    SlHParams.First_MB_Address =  uiFirst_MB_Address;
    SlHParams.Disable_Deblocking_Filter_Idc = (IMG_UINT8) ui8DisableDeblockingFilterIDC;
    SlHParams.bPiCInterlace = bIsInterlaced;
    SlHParams.bFieldType    = ui8FieldNum;
    SlHParams.iDebAlphaOffsetDiv2   = 0;
    SlHParams.iDebBetaOffsetDiv2    = 0;

    if (bMultiRef)
        SlHParams.num_ref_idx_l0_active_minus1 = 1;
    else
        SlHParams.num_ref_idx_l0_active_minus1 = 0;

    SlHParams.weighted_pred_flag            = pWeightedSetup ? pWeightedSetup->weighted_pred_flag             : 0;
    SlHParams.weighted_bipred_idc   = pWeightedSetup ? pWeightedSetup->weighted_bipred_idc            : 0;
    SlHParams.luma_log2_weight_denom        = pWeightedSetup ? pWeightedSetup->luma_log2_weight_denom         : 0;
    SlHParams.chroma_log2_weight_denom      = pWeightedSetup ? pWeightedSetup->chroma_log2_weight_denom : 0;
    SlHParams.luma_weight_l0_flag[0]        = pWeightedSetup ? pWeightedSetup->weight_flag[0][0]                      : 0;
    SlHParams.luma_weight_l0[0]             = pWeightedSetup ? pWeightedSetup->weight[0][0]                   : 0;
    SlHParams.luma_offset_l0[0]             = pWeightedSetup ? pWeightedSetup->offset[0][0]                   : 0;
    SlHParams.chroma_weight_l0_flag[0]      = pWeightedSetup ? pWeightedSetup->weight_flag[1][0]                      : 0;
    SlHParams.chromaB_weight_l0[0]          = pWeightedSetup ? pWeightedSetup->weight[1][0]                   : 0;
    SlHParams.chromaB_offset_l0[0]          = pWeightedSetup ? pWeightedSetup->offset[1][0]                   : 0;
    SlHParams.chromaR_weight_l0[0]          = pWeightedSetup ? pWeightedSetup->weight[2][0]                   : 0;
    SlHParams.chromaR_offset_l0[0]          = pWeightedSetup ? pWeightedSetup->offset[2][0]                   : 0;
    SlHParams.luma_weight_l0_flag[1]        = pWeightedSetup ? pWeightedSetup->weight_flag[0][1]                      : 0;
    SlHParams.luma_weight_l0[1]             = pWeightedSetup ? pWeightedSetup->weight[0][1]                   : 0;
    SlHParams.luma_offset_l0[1]             = pWeightedSetup ? pWeightedSetup->offset[0][1]                   : 0;
    SlHParams.chroma_weight_l0_flag[1]      = pWeightedSetup ? pWeightedSetup->weight_flag[1][1]                      : 0;
    SlHParams.chromaB_weight_l0[1]          = pWeightedSetup ? pWeightedSetup->weight[1][1]                   : 0;
    SlHParams.chromaB_offset_l0[1]          = pWeightedSetup ? pWeightedSetup->offset[1][1]                   : 0;
    SlHParams.chromaR_weight_l0[1]          = pWeightedSetup ? pWeightedSetup->weight[2][1]                   : 0;
    SlHParams.chromaR_offset_l0[1]          = pWeightedSetup ? pWeightedSetup->offset[2][1]                   : 0;

    SlHParams.bIsLongTermRef = bIsLongTermRef;
    ptg__H264_getelements_slice_header(pMTX_Header, &SlHParams, bCabacEnabled);

    /*
        {
            IMG_UINT32      *pMTX_Header_Mem = (IMG_UINT32 *)mtx_hdr;
            // rhk: first insert normal header.
            ptg__H264_getelements_slice_header(mtx_hdr, &SlHParams, bCabacEnabled, uiIdrPicId);

            // put a marker to indicate that this is a complex header
            // note that first "int" in the buffer is used for number of elements
            // which is not going to be more than 255
            *pMTX_Header_Mem |= 0x100;

            // rhk: insert skipped frame header at an offset of 128 bytes
            pMTX_Header_Mem += (HEADER_SIZE >> 3);  // get a pointer to the second half of memory
            mtx_hdr = (MTX_HEADER_PARAMS *)pMTX_Header_Mem;
            ptg__H264_getelements_skip_P_slice(mtx_hdr, &SlHParams, uiMBSkipRun, bCabacEnabled);
        }
    */
}

void ptg__MPEG4_prepare_sequence_header(
    void *pHeaderMemory,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE sProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS * psVBVParams,
    IMG_UINT32 VopTimeResolution)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;
    /* Builds a single MPEG4 video sequence header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    aui32ElementPointers[0] = This_Element;

    ptg__MPEG4_writebits_sequence_header(mtx_hdr,
        				 aui32ElementPointers,
        				 bBFrame,
					 sProfile,
       				 	 Profile_and_level_indication,
       				 	 sFixed_vop_time_increment,
        				 Picture_Width_Pixels,
        				 Picture_Height_Pixels,
        				 psVBVParams,
					 VopTimeResolution);

    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */

}

void ptg__MPEG4_prepare_vop_header(
    IMG_UINT32 *pHeaderMem,
    IMG_BOOL bIsVOP_coded,
    IMG_UINT32 VOP_time_increment,
    IMG_UINT8 sSearch_range,
    IMG_UINT8 eVop_Coding_Type,
    IMG_UINT32 VopTimeResolution)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    /* Builds a single MPEG4 VOP (picture) header from the given parameters */
    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* Frame QScale no longer written here as it is inserted by MTX later
     * (add as parameter to MTX_Send_Elements_To_VLC)
     */
    ptg__MPEG4_writebits_VOP_header(
        mtx_hdr, elt_p, bIsVOP_coded,
        VOP_time_increment,
        sSearch_range,
        eVop_Coding_Type,
        VopTimeResolution);

    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */

}

void ptg__H263_prepare_sequence_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 Profile_and_level_indication)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    /* Builds a single H263 video sequence header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_VideoSequenceHeader(mtx_hdr, elt_p, Profile_and_level_indication);

    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */

}

void ptg__H263_prepare_picture_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT16 PictureWidth,
    IMG_UINT16 PictureHeigth)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_VideoPictureHeader(
        mtx_hdr, elt_p,
        Temporal_Ref,
        PictureCodingType,
        SourceFormatType,
        FrameRate,
        PictureWidth,
        PictureHeigth);

    mtx_hdr->ui32Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

void ptg__H263_prepare_GOBslice_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_GOBSliceHeader(mtx_hdr, elt_p, GOBNumber, GOBFrameId);

    mtx_hdr->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count

    /* silent the warning message */
    (void)Show_Bits;
    (void)Show_Elements;
    /*
    (void)ptg__H264_writebits_SEI_rbspheader;
    (void)ptg__H264_getelements_skip_B_slice;
    (void)ptg__H264_getelements_backward_zero_B_slice;
    (void)ptg__H264_getelements_rbsp_ATE_only;
    (void)ptg_MPEG4_getelements_video_packet_header;
    */
}

void ptg__H264ES_notforsims_writebits_extension_sliceheader(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_BOOL bCabacEnabled, IMG_BOOL bIsIDR)
{
    bStartNextRawDataElement = IMG_FALSE;

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);

    ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, pSlHParams->ui8Start_Code_Prefix_Size_Bytes); //Can be 3 or 4 bytes - always 4 bytes in our implementations

    ///**** GENERATES THE FIRST ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 8

    // StartCodePrefix Pregenerated in: Build_H264_4Byte_StartCodePrefix_Element() (4 or 3 bytes)
    //(3 bytes when slice is first in a picture without sequence/picture_header before picture
    // Byte aligned (bit 32 or 24)
    // NOTE: Slice_Type and Frame_Type are always the same, hence SliceFrame_Type
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);   // forbidden_zero_bit

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_REFERENCE); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 20, 5);    // nal_unit_type for coded_slice_extension


    //if(nal_unit_type == 14 || nal_unit_type == 20)
    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);   // SVC extension flag

    // nal_unit_header_mvc_extension()
    {
        if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 1);   // non_idr_flag flag
        } else if ((pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE) && bIsIDR) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 1);   // non_idr_flag flag
        } else {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    1, 1);   // non_idr_flag flag
        }

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 6);   // priority_id flag
        ptg__write_upto32bits_elements(pMTX_Header, aui32ElementPointers,    1, 10);   // view_id = hardcoded to 1 for dependent view

        //ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 3);   // temporal_id flag
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_TEMPORAL_ID); // temporal_id flag
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_ANCHOR_PIC_FLAG); // anchor_pic_flag

        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    0, 1);   // interview flag is always FALSE for dependent frames

        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,    1, 1);   // reserved one bit
    }

    // slice header
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_CURRMBNR); //MTX fills this value in

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES THE SECOND ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    /* The following is slice parameter set in BP/MP */
    //ptg__generate_ue(pMTX_Header, aui32ElementPointers, (IMG_UINT32) pSlHParams->First_MB_Address);                               //first_mb_in_slice = First MB address in slice: ue(Range 0 -  1619)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, (IMG_UINT32)((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) ? SLHP_I_SLICEFRAME_TYPE : pSlHParams->SliceFrame_Type));                                    //slice_type ue(v): 0 for P-slice, 1 for B-slice, 2 for I-slice
    // kab: //not clean change from IDR to intra, IDR should have separate flag

    ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);  // pic_parameter_set_id = 1 for dependent view

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_FRAME_NUM); // Insert token to tell MTX to insert frame_num
    bStartNextRawDataElement = IMG_TRUE;

    if ((pSlHParams->bPiCInterlace) || (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE)) {
        //ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

        // interlaced encoding
        if (pSlHParams->bPiCInterlace) {
            CheckStartRawDataElement(pMTX_Header, aui32ElementPointers);
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);                                          // field_pic_flag = 1
            ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_BOTTOM_FIELD); // Insert token to tell MTX to insert BOTTOM_FIELD flag if required
            //ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
            bStartNextRawDataElement = IMG_TRUE;
        }
    }

    if ((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) || (bIsIDR)) {
        CheckStartRawDataElement(pMTX_Header, aui32ElementPointers);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); // idr_pic_id ue(v) = 0 (1b) in Topaz
    }

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_PIC_ORDER_CNT); // Insert token to tell MTX to insert pic_order_cnt_lsb
    bStartNextRawDataElement = IMG_TRUE;

    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_DIRECT_SPATIAL_MV_FLAG); // Insert token to tell MTX to insert direct_spatial_mv_pred_flag

    if (pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_NUM_REF_IDX_ACTIVE);
        bStartNextRawDataElement = IMG_TRUE;
    } else if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE) {
        CheckStartRawDataElement(pMTX_Header, aui32ElementPointers);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);             // num_ref_idx_active_override_flag (1 bit) = 0 in Topaz
    }

    // reference picture list modification
    if (pSlHParams->SliceFrame_Type != SLHP_I_SLICEFRAME_TYPE && pSlHParams->SliceFrame_Type != SLHP_IDR_SLICEFRAME_TYPE) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_REORDER_L0); // Insert token to tell MTX to insert BOTTOM_FIELD flag if required
        //ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        bStartNextRawDataElement = IMG_TRUE;
    }

    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)        {
        CheckStartRawDataElement(pMTX_Header, aui32ElementPointers);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);                                                                 // ref_pic_list_ordering_flag_l1 (1 bit) = 0, no reference picture ordering in Topaz
    }

    // WEIGHTED PREDICTION
    /*      ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT);
        bStartNextRawDataElement = IMG_TRUE;*/

    if ((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) || (bIsIDR)) {
        CheckStartRawDataElement(pMTX_Header, aui32ElementPointers);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);                                                                 // no_output_of_prior_pics_flag (1 bit) = 0
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);                                                                 // long_term_reference_flag (1 bit) = 0
    } else {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_ADAPTIVE); //MTX fills this value in
        //ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        bStartNextRawDataElement = IMG_TRUE;
    }

    if (bCabacEnabled && ((SLHP_P_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type) ||
                          (SLHP_B_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type))) {
        CheckStartRawDataElement(pMTX_Header, aui32ElementPointers);
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // hard code cabac_init_idc value of 0
    }
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SQP); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 11
    // Next field is generated on MTX with a special commnad (not ELEMENT_RAW) - so not defined here
    //pucHS=ptg__generate_se(pucHS, puiBitPos, pSlHParams->Slice_QP_Delta); //slice_qp_delta se(v) = SliceQPy - (pic_init_qp_minus26+26)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->Disable_Deblocking_Filter_Idc); //disable_deblocking_filter_idc ue(v) = 2?
    if (pSlHParams->Disable_Deblocking_Filter_Idc != 1) {
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->iDebAlphaOffsetDiv2); //slice_alpha_c0_offset_div2 se(v) = 0 (1b) in Topaz
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->iDebBetaOffsetDiv2); //slice_beta_offset_div2 se(v) = 0 (1b) in Topaz
    }
    //num_slice_groups_minus1 ==0 in Topaz, so no slice_group_change_cycle field here
    // no byte alignment at end of slice headers
}


/*****************************************************************************
******************************************************************************/
static void ptg__H264ES_notforsims_writebits_slice_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_BOOL bCabacEnabled, IMG_BOOL bIsIDR)
{
    bStartNextRawDataElement = IMG_FALSE;
    unsigned char* pdg = (unsigned char*)pMTX_Header;
#ifdef _TOPAZHP_PDUMP_SLICE_
    drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: in element = %d, ui16MvcViewIdx = %d\n", __FUNCTION__, pMTX_Header->ui32Elements, pSlHParams->ui16MvcViewIdx);
#endif
    if (pSlHParams->ui16MvcViewIdx == (IMG_UINT16)(NON_MVC_VIEW)) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);
    } else if (pSlHParams->ui16MvcViewIdx == MVC_BASE_VIEW_IDX) {
        ptg__insert_prefix_nal_header(pMTX_Header, aui32ElementPointers,
                                      pSlHParams, bCabacEnabled);
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_MIDHDR);
    } else {
        //Insert
        ptg__H264ES_notforsims_writebits_extension_sliceheader(pMTX_Header, aui32ElementPointers,
                pSlHParams, bCabacEnabled, bIsIDR);
        return;
    }

    ptg__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, pSlHParams->ui8Start_Code_Prefix_Size_Bytes); //Can be 3 or 4 bytes - always 4 bytes in our implementations

    ///**** GENERATES THE FIRST ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 8

    // StartCodePrefix Pregenerated in: Build_H264_4Byte_StartCodePrefix_Element() (4 or 3 bytes)
    //(3 bytes when slice is first in a picture without sequence/picture_header before picture
    // Byte aligned (bit 32 or 24)
    // NOTE: Slice_Type and Frame_Type are always the same, hence SliceFrame_Type

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);   // forbidden_zero_bit

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_REFERENCE); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, ((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE ? 5 : 1)),                   // nal_unit_tpye (5 bits) = I-frame IDR, and 1 for  rest
                                  5);

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_CURRMBNR); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);


    ///**** GENERATES THE SECOND ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    /* The following is slice parameter set in BP/MP */
    //ptg__generate_ue(pMTX_Header, aui32ElementPointers, (IMG_UINT32) pSlHParams->First_MB_Address);                               //first_mb_in_slice = First MB address in slice: ue(Range 0 -  1619)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, (IMG_UINT32)((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) ? SLHP_I_SLICEFRAME_TYPE : pSlHParams->SliceFrame_Type));                                    //slice_type ue(v): 0 for P-slice, 1 for B-slice, 2 for I-slice
    // kab: //not clean change from IDR to intra, IDR should have separate flag
    if (pSlHParams->ui16MvcViewIdx != (IMG_UINT16)(NON_MVC_VIEW))
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->ui16MvcViewIdx);  // pic_parameter_set_id = 0
    else
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);  // pic_parameter_set_id = 0

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_FRAME_NUM); // Insert token to tell MTX to insert frame_num

    if ((pSlHParams->bPiCInterlace) || (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE)) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        // interlaced encoding
        if (pSlHParams->bPiCInterlace) {
            ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);                                          // field_pic_flag = 1
            ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_BOTTOM_FIELD); // Insert token to tell MTX to insert BOTTOM_FIELD flag if required
            ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        }

        if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE)
            ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_IDR_PIC_ID);       // idr_pic_id ue(v)
    }

    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_PIC_ORDER_CNT); // Insert token to tell MTX to insert pic_order_cnt_lsb

    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_DIRECT_SPATIAL_MV_FLAG); // Insert token to tell MTX to insert direct_spatial_mv_pred_flag

    if (pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_NUM_REF_IDX_ACTIVE); // Insert token to tell MTX to insert override for number of active references
    } else if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);                                                                 // num_ref_idx_active_override_flag (1 bit) = 0
    }

    if (pSlHParams->SliceFrame_Type != SLHP_I_SLICEFRAME_TYPE &&
        pSlHParams->SliceFrame_Type != SLHP_IDR_SLICEFRAME_TYPE) {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_REORDER_L0); // Insert token to tell MTX to insert reference list 0 reordering
        if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
            ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_REORDER_L1); // Insert token to tell MTX to insert reference list 1 reordering
    }


    // WEIGHTED PREDICTION
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT);
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    if (pSlHParams->bReferencePicture && pSlHParams->bIsLongTermRef) {
        ptg__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1); // adaptive_ref_pic_marking_mode_flag (1 bit) = 0

        // Clear any existing long-term reference
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 5);                                 // memory_management_control_operation

        // Allow a single long-term reference
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 4);                                 // memory_management_control_operation
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 1);                                 // max_long_term_frame_idx_plus1

        // Set current picture as the long-term reference
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 6);                                 // memory_management_control_operation
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);                                 // long_term_frame_idx

        // End
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0);                                 // memory_management_control_operation
    } else {
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_ADAPTIVE); //MTX fills this value in
        ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);
    }

    if (bCabacEnabled && ((SLHP_P_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type) ||
                          (SLHP_B_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type))) {
        ptg__generate_ue(pMTX_Header, aui32ElementPointers, 0); // hard code cabac_init_idc value of 0
    }
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_SQP); //MTX fills this value in
    ptg__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 11
    // Next field is generated on MTX with a special commnad (not ELEMENT_RAW) - so not defined here
    //pucHS=ptg__generate_se(pucHS, puiBitPos, pSlHParams->Slice_QP_Delta); //slice_qp_delta se(v) = SliceQPy - (pic_init_qp_minus26+26)
    ptg__generate_ue(pMTX_Header, aui32ElementPointers, pSlHParams->Disable_Deblocking_Filter_Idc); //disable_deblocking_filter_idc ue(v) = 2?

    if (pSlHParams->Disable_Deblocking_Filter_Idc != 1) {
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->iDebAlphaOffsetDiv2); //slice_alpha_c0_offset_div2 se(v) = 0 (1b) in Topaz
        ptg__generate_se(pMTX_Header, aui32ElementPointers, pSlHParams->iDebBetaOffsetDiv2); //slice_beta_offset_div2 se(v) = 0 (1b) in Topaz
    }
    //num_slice_groups_minus1 ==0 in Topaz, so no slice_group_change_cycle field here
    // no byte alignment at end of slice headers
}

/******************************************************************************
 @Function              H264_NOTFORSIMS_PrepareSliceHeader
 @details
 Sim guys, DO NOT EVER USE THIS FUNCTION!
 Prepare an H264 slice header in a form for the MTX to encode into a
 bitstream.
 @param    pMTX_Header        : pointer to header structure to populate
 @param    bIntraSlice        : IMG_TRUE if this is an IDR or I slice
 @param    bInterBSlice        : IMG_TRUE if this is a B Frame slice
 @param    ui8DisableDeblockingFilterIDC : syntax element
 @param    ui32FrameNumId      : syntax element (used for references)
 @param    uiFirst_MB_Address : first_mb_in_slice syntax element
 @param    uiMBSkipRun        : number of MBs to skip
 @param    bCabacEnabled      : IMG_TRUE to use CABAC
 @param    bIsInterlaced      : IMG_TRUE for interlaced slices
 @param    bIsIDR             : IMG_TRUE if this is an IDR slice
 @param    bIsLongTermRef     : IMG_TRUE if the frame is to be used as a long-term reference
 @return   None
******************************************************************************/
#ifdef _TOPAZHP_PDUMP_SLICE_
static void ptg_trace_slice_header_params(H264_SLICE_HEADER_PARAMS *psSlHParams)
{
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: start addr = 0x%08x\n", __FUNCTION__, psSlHParams);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.ui8Start_Code_Prefix_Size_Bytes 0x%08x\n", psSlHParams->ui8Start_Code_Prefix_Size_Bytes);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.SliceFrame_Type                 0x%08x\n", psSlHParams->SliceFrame_Type);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.First_MB_Address                0x%08x\n", psSlHParams->First_MB_Address);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.Frame_Num_DO                    0x%08x\n", psSlHParams->Frame_Num_DO);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.Idr_Pic_Id                      0x%08x\n", psSlHParams->Idr_Pic_Id);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.log2_max_pic_order_cnt          0x%08x\n", psSlHParams->log2_max_pic_order_cnt);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.Picture_Num_DO                  0x%08x\n", psSlHParams->Picture_Num_DO);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.Disable_Deblocking_Filter_Idc   0x%08x\n", psSlHParams->Disable_Deblocking_Filter_Idc);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.bPiCInterlace                   0x%08x\n", psSlHParams->bPiCInterlace);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.bFieldType                      0x%08x\n", psSlHParams->bFieldType);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.bReferencePicture               0x%08x\n", psSlHParams->bReferencePicture);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.iDebAlphaOffsetDiv2             0x%08x\n", psSlHParams->iDebAlphaOffsetDiv2);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.iDebBetaOffsetDiv2              0x%08x\n", psSlHParams->iDebBetaOffsetDiv2);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.direct_spatial_mv_pred_flag     0x%08x\n", psSlHParams->direct_spatial_mv_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.num_ref_idx_l0_active_minus1    0x%08x\n", psSlHParams->num_ref_idx_l0_active_minus1);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.diff_ref_pic_num[0] 0x%08x\n", psSlHParams->diff_ref_pic_num[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.diff_ref_pic_num[1] 0x%08x\n", psSlHParams->diff_ref_pic_num[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.weighted_pred_flag  0x%08x\n", psSlHParams->weighted_pred_flag);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.weighted_bipred_idc 0x%08x\n", psSlHParams->weighted_bipred_idc);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.luma_log2_weight_denom   0x%08x\n", psSlHParams->luma_log2_weight_denom);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chroma_log2_weight_denom 0x%08x\n", psSlHParams->chroma_log2_weight_denom);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.luma_weight_l0_flag[0]  0x%08x\n",  psSlHParams->luma_weight_l0_flag[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.luma_weight_l0_flag[1]  0x%08x\n",  psSlHParams->luma_weight_l0_flag[1]);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.SlHParams.luma_weight_l0[0] 0x%08x\n", psSlHParams->luma_weight_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.SlHParams.luma_weight_l0[1] 0x%08x\n", psSlHParams->luma_weight_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.luma_offset_l0[0]           0x%08x\n", psSlHParams->luma_offset_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.luma_offset_l0[1]           0x%08x\n", psSlHParams->luma_offset_l0[1]);

  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chroma_weight_l0_flag[0]    0x%08x\n", psSlHParams->chroma_weight_l0_flag[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chroma_weight_l0_flag[1]    0x%08x\n", psSlHParams->chroma_weight_l0_flag[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaB_weight_l0[0]        0x%08x\n", psSlHParams->chromaB_weight_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaB_offset_l0[0]        0x%08x\n", psSlHParams->chromaB_offset_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaR_weight_l0[0]        0x%08x\n", psSlHParams->chromaR_weight_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaR_offset_l0[0]        0x%08x\n", psSlHParams->chromaR_offset_l0[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaB_weight_l0[1]        0x%08x\n", psSlHParams->chromaB_weight_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaB_offset_l0[1]        0x%08x\n", psSlHParams->chromaB_offset_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaR_weight_l0[1]        0x%08x\n", psSlHParams->chromaR_weight_l0[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.chromaR_offset_l0[1]        0x%08x\n", psSlHParams->chromaR_offset_l0[1]);
 
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.ui16MvcViewIdx    0x%08x\n", psSlHParams->ui16MvcViewIdx);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.bIsLongTermRef    0x%08x\n", psSlHParams->bIsLongTermRef);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.uLongTermRefNum   0x%08x\n", psSlHParams->uLongTermRefNum);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.bRefIsLongTermRef[0]  0x%08x\n", psSlHParams->bRefIsLongTermRef[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.bRefIsLongTermRef[1]  0x%08x\n", psSlHParams->bRefIsLongTermRef[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.uRefLongTermRefNum[0] 0x%08x\n", psSlHParams->uRefLongTermRefNum[0]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("SlHParams.uRefLongTermRefNum[1] 0x%08x\n", psSlHParams->uRefLongTermRefNum[1]);
  drv_debug_msg(VIDEO_DEBUG_GENERAL, ("%s: end \n", __FUNCTION__);
}
#endif

void ptg__H264ES_notforsims_prepare_sliceheader(
    IMG_UINT8      *slice_mem_p,
    IMG_UINT32      ui32SliceType,
    IMG_UINT8       ui8DisableDeblockingFilterIDC,
    IMG_UINT32      uiFirst_MB_Address,
    IMG_UINT32      uiMBSkipRun,
    IMG_BOOL        bCabacEnabled,
    IMG_BOOL        bIsInterlaced,
    IMG_UINT16      ui16MvcViewIdx,
    IMG_BOOL        bIsLongTermRef
)
{
    SLICE_PARAMS *slice_temp_p = (SLICE_PARAMS*)slice_mem_p;
    MTX_HEADER_PARAMS *pMTX_Header = (MTX_HEADER_PARAMS *) &(slice_temp_p->sSliceHdrTmpl);
    H264_SLICE_HEADER_PARAMS SlHParams;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    IMG_FRAME_TEMPLATE_TYPE eSliceType = (IMG_FRAME_TEMPLATE_TYPE)ui32SliceType;
    IMG_BOOL bIntraSlice = ((eSliceType == IMG_FRAME_IDR) || (eSliceType == IMG_FRAME_INTRA));
    IMG_BOOL bInterBSlice = (eSliceType == IMG_FRAME_INTER_B);
    IMG_BOOL bIsIDR = ((eSliceType == IMG_FRAME_IDR) || (eSliceType == IMG_FRAME_INTER_P_IDR));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n", __FUNCTION__);
    memset(&SlHParams, 0, sizeof(H264_SLICE_HEADER_PARAMS));

    SlHParams.ui8Start_Code_Prefix_Size_Bytes = 4;
    /* pcb -  I think that this is more correct now  -- This should also work for IDR-P frames which will be marked as SLHP_P_SLICEFRAME_TYPE */
    SlHParams.SliceFrame_Type = bIntraSlice ? (bIsIDR ? SLHP_IDR_SLICEFRAME_TYPE : SLHP_I_SLICEFRAME_TYPE) : (bInterBSlice ? SLHP_B_SLICEFRAME_TYPE : SLHP_P_SLICEFRAME_TYPE);

    SlHParams.First_MB_Address = uiFirst_MB_Address;
    SlHParams.Disable_Deblocking_Filter_Idc = (IMG_UINT8) ui8DisableDeblockingFilterIDC;
    SlHParams.bPiCInterlace = bIsInterlaced;
    SlHParams.iDebAlphaOffsetDiv2 = 0;
    SlHParams.iDebBetaOffsetDiv2  = 0;
    // setup the new flags used for B frame as reference
    SlHParams.bReferencePicture = bInterBSlice ? 0 : 1;
    SlHParams.ui16MvcViewIdx = ui16MvcViewIdx;
    SlHParams.bIsLongTermRef = bIsLongTermRef;
    SlHParams.log2_max_pic_order_cnt = 2;
    SlHParams.uLongTermRefNum = 0;
    SlHParams.bRefIsLongTermRef[0] = 0;
    SlHParams.uRefLongTermRefNum[0] = 0;
    SlHParams.bRefIsLongTermRef[1] = 0;
    SlHParams.uRefLongTermRefNum[1] = 0;
    // Builds a single slice header from the given parameters (mid frame)
    // Essential we initialise our header structures before building
//    memset(pMTX_Header, 0, 128);
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

#ifdef _TOPAZHP_PDUMP_SLICE_
  ptg_trace_slice_header_params(&SlHParams);
#endif

    ptg__H264ES_notforsims_writebits_slice_header(pMTX_Header, aui32ElementPointers, &SlHParams, bCabacEnabled, bIsIDR);
    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
}

/**************************************************************************************************
* Function:             H263_NOTFORSIMS_PrepareGOBSliceHeader
* Description:  Generates the slice params template
*
***************************************************************************************************/

void ptg__H263ES_notforsims_prepare_gobsliceheader(
    IMG_UINT8       *slice_mem_p)
{
    // Essential we initialise our header structures before building
    SLICE_PARAMS *slice_temp_p = (SLICE_PARAMS*)slice_mem_p;
    MTX_HEADER_PARAMS *     pMTX_Header = (MTX_HEADER_PARAMS *) & (slice_temp_p->sSliceHdrTmpl);
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;
    H263_NOTFORSIMS_WriteBits_GOBSliceHeader(pMTX_Header, aui32ElementPointers);
    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
}

/**************************************************************************************************
* Function:             MPEG2_PrepareSliceHeader
* Description:  Generates the slice params template
*
***************************************************************************************************/
void ptg__MPEG2_prepare_sliceheader(
    IMG_UINT8               *slice_mem_p)
{
    // Essential we initialise our header structures before building
    SLICE_PARAMS *slice_temp_p = (SLICE_PARAMS*)slice_mem_p;
    MTX_HEADER_PARAMS *     pMTX_Header = (MTX_HEADER_PARAMS *) & (slice_temp_p->sSliceHdrTmpl);
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->ui32Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;
//      MPEG2_WriteBits_SliceHeader(pMTX_Header, aui32ElementPointers);
    pMTX_Header->ui32Elements++; //Has been used as an index, so need to add 1 for a valid element count
}


