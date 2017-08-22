/*******************************************************************************
* Copyright 2017 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "mkldnn.h"

#include "mkldnn_common.hpp"

#include "conv/conv.hpp"

namespace conv {

/* cfgs definition
 * arrays: SRC, WEI, BIA, DST, ACC
 * params: {data_type, min, max, f_min, f_max, f_base, f_step, f_sparsity, eps}
 */

const int int_max_exact = 1<<24;
const _dt_conf_t conf_f32 = {
    {mkldnn_f32, -int_max_exact, int_max_exact,  -64,  64, 0, 1, .25, 0.},
    {mkldnn_f32, -int_max_exact, int_max_exact,  -32,  32, 0, 1, 1.0, 0.},
    {mkldnn_f32, -int_max_exact, int_max_exact, -512, 512, 0, 1, 1.0, 0.},
    {mkldnn_f32, -int_max_exact, int_max_exact,  -64,  64, 0, 1, .25, 0.},
    {mkldnn_f32,},
};

const _dt_conf_t conf_f32_full = {
    {mkldnn_f32, -int_max_exact, int_max_exact,  -64,  64, 0, 1, 1.0, 0.},
    {mkldnn_f32, -int_max_exact, int_max_exact,  -32,  32, 0, 1, 1.0, 0.},
    {mkldnn_f32, -int_max_exact, int_max_exact, -512, 512, 0, 1, 1.0, 0.},
    {mkldnn_f32, -int_max_exact, int_max_exact,  -64,  64, 0, 1, 1.0, 0.},
    {mkldnn_f32,},
};

const _dt_conf_t conf_f32_wino = {
    {mkldnn_f32, -int_max_exact, int_max_exact,   0,  16, 3, 1, .25, 1e-5},
    {mkldnn_f32, -int_max_exact, int_max_exact,   2,  64, 2, 1, .75, 5e-5},
    {mkldnn_f32, -int_max_exact, int_max_exact,   1, 128, 1, 1, .25,   0.},
    {mkldnn_f32, -int_max_exact, int_max_exact,   0,  16, 3, 1, .25, 1e-5},
    {mkldnn_f32,},
};

const _dt_conf_t conf_s16s16s32s32 = {
    {mkldnn_s16, INT16_MIN, INT16_MAX, -4,  4, 0, 1, .25, 0.},
    {mkldnn_s16, INT16_MIN, INT16_MAX, -5,  5, 0, 1, .25, 0.},
    {mkldnn_s32, INT32_MIN, INT32_MAX, -8, 32, 0, 1, .25, 0.},
    {mkldnn_s32, INT32_MIN, INT32_MAX, -4,  4, 0, 1, .25, 0.},
    {mkldnn_s32,},
};

const _dt_conf_t conf_u8s8s32s32 = {
    {mkldnn_u8,          0, UINT8_MAX,    0,   4, 0, 1, .25, 0.},
    {mkldnn_s8,   INT8_MIN,  INT8_MAX,   -5,   5, 0, 1, .25, 0.},
    {mkldnn_s32, INT32_MIN, INT32_MAX,   -8,  32, 0, 1, .25, 0.},
    {mkldnn_s32, INT32_MIN, INT32_MAX, -255, 255, 0, 1, .25, 0.},
    {mkldnn_s32,},
};

const _dt_conf_t conf_u8s8s8s32 = {
    {mkldnn_u8,          0, UINT8_MAX,    0,   4, 0, 1, .25, 0.},
    {mkldnn_s8,   INT8_MIN,  INT8_MAX,   -5,   5, 0, 1, .25, 0.},
    {mkldnn_s32, INT32_MIN, INT32_MAX,   -8,  32, 0, 1, .25, 0.},
    {mkldnn_s8,   INT8_MIN,  INT8_MAX, -127, 127, 0, 1, .25, 0.},
    {mkldnn_s32,},
};

const _dt_conf_t conf_u8s8u8s32 = {
    {mkldnn_u8,          0, UINT8_MAX,    0,   4, 0, 1, .25, 0.},
    {mkldnn_s8,   INT8_MIN,  INT8_MAX,   -5,   5, 0, 1, .25, 0.},
    {mkldnn_s32, INT32_MIN, INT32_MAX,   -8,  32, 0, 1, .25, 0.},
    {mkldnn_u8,          0, UINT8_MAX,    0, 255, 0, 1, .25, 0.},
    {mkldnn_s32,},
};

const dt_conf_t *str2cfg(const char *str) {
#define CASE(cfg) \
    if (!strcasecmp(STRINGIFY(cfg), str)) return CONCAT2(conf_,cfg)
    CASE(f32);
    CASE(f32_full);
    CASE(f32_wino);
    CASE(s16s16s32s32);
    CASE(u8s8s32s32);
    CASE(u8s8s8s32);
    CASE(u8s8u8s32);
#undef CASE
    []() { SAFE(FAIL, CRIT); return 0; }();
    return (const dt_conf_t *)1;
}

const char *cfg2str(const dt_conf_t *cfg) {
#define CASE(_cfg) if (cfg == CONCAT2(conf_,_cfg)) return STRINGIFY(_cfg)
    CASE(f32);
    CASE(f32_full);
    CASE(f32_wino);
    CASE(s16s16s32s32);
    CASE(u8s8s32s32);
    CASE(u8s8s8s32);
    CASE(u8s8u8s32);
#undef CASE
    []() { SAFE(FAIL, CRIT); return 0; }();
    return NULL;
}

}
