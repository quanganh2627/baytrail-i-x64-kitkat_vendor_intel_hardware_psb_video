/*
 * INTEL CONFIDENTIAL
 * Copyright 2007 Intel Corporation. All Rights Reserved.
 * Copyright 2005-2007 Imagination Technologies Limited. All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

/*!
******************************************************************************
@file   : ../firmware/msvdx/mtxlib/include/fwrk_msg_mem_io.h

@brief

@Author <Autogenerated>

<b>Description:</b>\n
                This file contains the FWRK_MSG_MEM_IO_H Definitions.

<b>Platform:</b>\n
                ?

@Version
                1.0

******************************************************************************/

#if !defined (__FWRK_MSG_MEM_IO_H__)
#define __FWRK_MSG_MEM_IO_H__

#ifdef __cplusplus
extern "C" {
#endif


#define FWRK_GENMSG_SIZE                (2)

// FWRK_GENMSG     SIZE
#define FWRK_GENMSG_SIZE_ALIGNMENT              (1)
#define FWRK_GENMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_GENMSG_SIZE_MASK           (0xFF)
#define FWRK_GENMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_GENMSG_SIZE_OFFSET         (0x0000)
#define FWRK_GENMSG_SIZE_SHIFT          (0)

// FWRK_GENMSG     ID
#define FWRK_GENMSG_ID_ALIGNMENT                (1)
#define FWRK_GENMSG_ID_TYPE             IMG_UINT8
#define FWRK_GENMSG_ID_MASK             (0xFF)
#define FWRK_GENMSG_ID_LSBMASK          (0xFF)
#define FWRK_GENMSG_ID_OFFSET           (0x0001)
#define FWRK_GENMSG_ID_SHIFT            (0)

#define FWRK_PADMSG_SIZE                (2)

// FWRK_PADMSG     SIZE
#define FWRK_PADMSG_SIZE_ALIGNMENT              (1)
#define FWRK_PADMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_PADMSG_SIZE_MASK           (0xFF)
#define FWRK_PADMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_PADMSG_SIZE_OFFSET         (0x0000)
#define FWRK_PADMSG_SIZE_SHIFT          (0)

// FWRK_PADMSG     ID
#define FWRK_PADMSG_ID_ALIGNMENT                (1)
#define FWRK_PADMSG_ID_TYPE             IMG_UINT8
#define FWRK_PADMSG_ID_MASK             (0xFF)
#define FWRK_PADMSG_ID_LSBMASK          (0xFF)
#define FWRK_PADMSG_ID_OFFSET           (0x0001)
#define FWRK_PADMSG_ID_SHIFT            (0)

#define FWRK_RSTMSG_SIZE                (2)

// FWRK_RSTMSG     SIZE
#define FWRK_RSTMSG_SIZE_ALIGNMENT              (1)
#define FWRK_RSTMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_RSTMSG_SIZE_MASK           (0xFF)
#define FWRK_RSTMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_RSTMSG_SIZE_OFFSET         (0x0000)
#define FWRK_RSTMSG_SIZE_SHIFT          (0)

// FWRK_RSTMSG     ID
#define FWRK_RSTMSG_ID_ALIGNMENT                (1)
#define FWRK_RSTMSG_ID_TYPE             IMG_UINT8
#define FWRK_RSTMSG_ID_MASK             (0xFF)
#define FWRK_RSTMSG_ID_LSBMASK          (0xFF)
#define FWRK_RSTMSG_ID_OFFSET           (0x0001)
#define FWRK_RSTMSG_ID_SHIFT            (0)

#define FWRK_SRQMSG_SIZE                (2)

// FWRK_SRQMSG     SIZE
#define FWRK_SRQMSG_SIZE_ALIGNMENT              (1)
#define FWRK_SRQMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_SRQMSG_SIZE_MASK           (0xFF)
#define FWRK_SRQMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_SRQMSG_SIZE_OFFSET         (0x0000)
#define FWRK_SRQMSG_SIZE_SHIFT          (0)

// FWRK_SRQMSG     ID
#define FWRK_SRQMSG_ID_ALIGNMENT                (1)
#define FWRK_SRQMSG_ID_TYPE             IMG_UINT8
#define FWRK_SRQMSG_ID_MASK             (0xFF)
#define FWRK_SRQMSG_ID_LSBMASK          (0xFF)
#define FWRK_SRQMSG_ID_OFFSET           (0x0001)
#define FWRK_SRQMSG_ID_SHIFT            (0)

#define FWRK_STAMSG_SIZE                (3)

// FWRK_STAMSG     SIZE
#define FWRK_STAMSG_SIZE_ALIGNMENT              (1)
#define FWRK_STAMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_STAMSG_SIZE_MASK           (0xFF)
#define FWRK_STAMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_STAMSG_SIZE_OFFSET         (0x0000)
#define FWRK_STAMSG_SIZE_SHIFT          (0)

// FWRK_STAMSG     ID
#define FWRK_STAMSG_ID_ALIGNMENT                (1)
#define FWRK_STAMSG_ID_TYPE             IMG_UINT8
#define FWRK_STAMSG_ID_MASK             (0xFF)
#define FWRK_STAMSG_ID_LSBMASK          (0xFF)
#define FWRK_STAMSG_ID_OFFSET           (0x0001)
#define FWRK_STAMSG_ID_SHIFT            (0)

// FWRK_STAMSG     IDLE
#define FWRK_STAMSG_IDLE_ALIGNMENT              (1)
#define FWRK_STAMSG_IDLE_TYPE           IMG_UINT8
#define FWRK_STAMSG_IDLE_MASK           (0x01)
#define FWRK_STAMSG_IDLE_LSBMASK                (0x01)
#define FWRK_STAMSG_IDLE_OFFSET         (0x0002)
#define FWRK_STAMSG_IDLE_SHIFT          (0)

// FWRK_STAMSG     QUEUED_EVENTS
#define FWRK_STAMSG_QUEUED_EVENTS_ALIGNMENT             (1)
#define FWRK_STAMSG_QUEUED_EVENTS_TYPE          IMG_UINT8
#define FWRK_STAMSG_QUEUED_EVENTS_MASK          (0x02)
#define FWRK_STAMSG_QUEUED_EVENTS_LSBMASK               (0x01)
#define FWRK_STAMSG_QUEUED_EVENTS_OFFSET                (0x0002)
#define FWRK_STAMSG_QUEUED_EVENTS_SHIFT         (1)

#define FWRK_SPRMSG_SIZE                (3)

// FWRK_SPRMSG     SIZE
#define FWRK_SPRMSG_SIZE_ALIGNMENT              (1)
#define FWRK_SPRMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_SPRMSG_SIZE_MASK           (0xFF)
#define FWRK_SPRMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_SPRMSG_SIZE_OFFSET         (0x0000)
#define FWRK_SPRMSG_SIZE_SHIFT          (0)

// FWRK_SPRMSG     ID
#define FWRK_SPRMSG_ID_ALIGNMENT                (1)
#define FWRK_SPRMSG_ID_TYPE             IMG_UINT8
#define FWRK_SPRMSG_ID_MASK             (0xFF)
#define FWRK_SPRMSG_ID_LSBMASK          (0xFF)
#define FWRK_SPRMSG_ID_OFFSET           (0x0001)
#define FWRK_SPRMSG_ID_SHIFT            (0)

// FWRK_SPRMSG     PROF
#define FWRK_SPRMSG_PROF_ALIGNMENT              (1)
#define FWRK_SPRMSG_PROF_TYPE           IMG_UINT8
#define FWRK_SPRMSG_PROF_MASK           (0xFF)
#define FWRK_SPRMSG_PROF_LSBMASK                (0xFF)
#define FWRK_SPRMSG_PROF_OFFSET         (0x0002)
#define FWRK_SPRMSG_PROF_SHIFT          (0)

#define FWRK_PRRMSG_SIZE                (3)

// FWRK_PRRMSG     SIZE
#define FWRK_PRRMSG_SIZE_ALIGNMENT              (1)
#define FWRK_PRRMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_PRRMSG_SIZE_MASK           (0xFF)
#define FWRK_PRRMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_PRRMSG_SIZE_OFFSET         (0x0000)
#define FWRK_PRRMSG_SIZE_SHIFT          (0)

// FWRK_PRRMSG     ID
#define FWRK_PRRMSG_ID_ALIGNMENT                (1)
#define FWRK_PRRMSG_ID_TYPE             IMG_UINT8
#define FWRK_PRRMSG_ID_MASK             (0xFF)
#define FWRK_PRRMSG_ID_LSBMASK          (0xFF)
#define FWRK_PRRMSG_ID_OFFSET           (0x0001)
#define FWRK_PRRMSG_ID_SHIFT            (0)

// FWRK_PRRMSG     SUPPORTED
#define FWRK_PRRMSG_SUPPORTED_ALIGNMENT         (1)
#define FWRK_PRRMSG_SUPPORTED_TYPE              IMG_UINT8
#define FWRK_PRRMSG_SUPPORTED_MASK              (0x01)
#define FWRK_PRRMSG_SUPPORTED_LSBMASK           (0x01)
#define FWRK_PRRMSG_SUPPORTED_OFFSET            (0x0002)
#define FWRK_PRRMSG_SUPPORTED_SHIFT             (0)

#define FWRK_ERRMSG_SIZE                (3)

// FWRK_ERRMSG     SIZE
#define FWRK_ERRMSG_SIZE_ALIGNMENT              (1)
#define FWRK_ERRMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_ERRMSG_SIZE_MASK           (0xFF)
#define FWRK_ERRMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_ERRMSG_SIZE_OFFSET         (0x0000)
#define FWRK_ERRMSG_SIZE_SHIFT          (0)

// FWRK_ERRMSG     ID
#define FWRK_ERRMSG_ID_ALIGNMENT                (1)
#define FWRK_ERRMSG_ID_TYPE             IMG_UINT8
#define FWRK_ERRMSG_ID_MASK             (0xFF)
#define FWRK_ERRMSG_ID_LSBMASK          (0xFF)
#define FWRK_ERRMSG_ID_OFFSET           (0x0001)
#define FWRK_ERRMSG_ID_SHIFT            (0)

// FWRK_ERRMSG     ERROR
#define FWRK_ERRMSG_ERROR_ALIGNMENT             (1)
#define FWRK_ERRMSG_ERROR_TYPE          IMG_UINT8
#define FWRK_ERRMSG_ERROR_MASK          (0xFF)
#define FWRK_ERRMSG_ERROR_LSBMASK               (0xFF)
#define FWRK_ERRMSG_ERROR_OFFSET                (0x0002)
#define FWRK_ERRMSG_ERROR_SHIFT         (0)

#define FWRK_GBEMSG_SIZE                (2)

// FWRK_GBEMSG     SIZE
#define FWRK_GBEMSG_SIZE_ALIGNMENT              (1)
#define FWRK_GBEMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_GBEMSG_SIZE_MASK           (0xFF)
#define FWRK_GBEMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_GBEMSG_SIZE_OFFSET         (0x0000)
#define FWRK_GBEMSG_SIZE_SHIFT          (0)

// FWRK_GBEMSG     ID
#define FWRK_GBEMSG_ID_ALIGNMENT                (1)
#define FWRK_GBEMSG_ID_TYPE             IMG_UINT8
#define FWRK_GBEMSG_ID_MASK             (0xFF)
#define FWRK_GBEMSG_ID_LSBMASK          (0xFF)
#define FWRK_GBEMSG_ID_OFFSET           (0x0001)
#define FWRK_GBEMSG_ID_SHIFT            (0)

#define FWRK_BENMSG_SIZE                (8)

// FWRK_BENMSG     SIZE
#define FWRK_BENMSG_SIZE_ALIGNMENT              (1)
#define FWRK_BENMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_BENMSG_SIZE_MASK           (0xFF)
#define FWRK_BENMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_BENMSG_SIZE_OFFSET         (0x0000)
#define FWRK_BENMSG_SIZE_SHIFT          (0)

// FWRK_BENMSG     ID
#define FWRK_BENMSG_ID_ALIGNMENT                (1)
#define FWRK_BENMSG_ID_TYPE             IMG_UINT8
#define FWRK_BENMSG_ID_MASK             (0xFF)
#define FWRK_BENMSG_ID_LSBMASK          (0xFF)
#define FWRK_BENMSG_ID_OFFSET           (0x0001)
#define FWRK_BENMSG_ID_SHIFT            (0)

// FWRK_BENMSG     ERRNUM
#define FWRK_BENMSG_ERRNUM_ALIGNMENT            (4)
#define FWRK_BENMSG_ERRNUM_TYPE         IMG_UINT32
#define FWRK_BENMSG_ERRNUM_MASK         (0xFFFFFFFF)
#define FWRK_BENMSG_ERRNUM_LSBMASK              (0xFFFFFFFF)
#define FWRK_BENMSG_ERRNUM_OFFSET               (0x0004)
#define FWRK_BENMSG_ERRNUM_SHIFT                (0)

#define FWRK_EXTMSG_SIZE                (2)

// FWRK_EXTMSG     SIZE
#define FWRK_EXTMSG_SIZE_ALIGNMENT              (1)
#define FWRK_EXTMSG_SIZE_TYPE           IMG_UINT8
#define FWRK_EXTMSG_SIZE_MASK           (0xFF)
#define FWRK_EXTMSG_SIZE_LSBMASK                (0xFF)
#define FWRK_EXTMSG_SIZE_OFFSET         (0x0000)
#define FWRK_EXTMSG_SIZE_SHIFT          (0)

// FWRK_EXTMSG     ID
#define FWRK_EXTMSG_ID_ALIGNMENT                (1)
#define FWRK_EXTMSG_ID_TYPE             IMG_UINT8
#define FWRK_EXTMSG_ID_MASK             (0xFF)
#define FWRK_EXTMSG_ID_LSBMASK          (0xFF)
#define FWRK_EXTMSG_ID_OFFSET           (0x0001)
#define FWRK_EXTMSG_ID_SHIFT            (0)



#ifdef __cplusplus
}
#endif

#endif /* __FWRK_MSG_MEM_IO_H__ */
