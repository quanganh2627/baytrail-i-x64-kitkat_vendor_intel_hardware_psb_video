/*
 * INTEL CONFIDENTIAL
 * Copyright 2007 Intel Corporation. All Rights Reserved.
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

#ifndef _PSB_WS_DRIVER_H_
#define _PSB_WS_DRIVER_H_

#include <linux/types.h>
#include <wsbm/wsbm_util.h>
#include <wsbm/wsbm_driver.h>
#include "psb_drm.h"

struct _PsbDrmValidateNode {
    struct _ValidateNode base;
    struct psb_validate_arg val_arg;
};

extern struct _WsbmVNodeFuncs *psbVNodeFuncs(void);

static inline struct psb_validate_req *
            psbValReq(struct _ValidateNode *node) {
    return &(containerOf(node, struct _PsbDrmValidateNode, base)->
             val_arg.d.req);
}


#endif	/* _PSB_WS_DRIVER_H_ */
