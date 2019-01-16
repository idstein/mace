// Copyright 2018 The MACE Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#if defined(MACE_ENABLE_NEON)
#include "mace/ops/arm/common_neon.h"
#endif

#include "mace/core/macros.h"
#include "mace/ops/arm/depthwise_conv2d_neon.h"

namespace mace {
namespace ops {

namespace {
void DepthwiseConv2dPixel(const float *in_base,
                          const float *filter,
                          const index_t out_h,
                          const index_t out_w,
                          const index_t in_h_start,
                          const index_t in_w_start,
                          const index_t out_width,
                          const index_t in_height,
                          const index_t in_width,
                          int filter_height,
                          int filter_width,
                          float *out_base) {
  float sum = 0;
  for (int i = 0; i < filter_height; ++i) {
    for (int j = 0; j < filter_width; ++j) {
      index_t in_h = in_h_start + i;
      index_t in_w = in_w_start + j;
      if (in_h >= 0 && in_h < in_height && in_w >= 0 && in_w < in_width) {
        sum += in_base[in_h * in_width + in_w] * filter[i * filter_width + j];
      }
    }
  }
  out_base[out_h * out_width + out_w] = sum;
}
}  // namespace

// Ho = 2, Wo = 4, Co = 1
void DepthwiseConv2dNeonK3x3S1(const float *input,
                               const float *filter,
                               const index_t *in_shape,
                               const index_t *out_shape,
                               const int *pad_hw,
                               const index_t valid_h_start,
                               const index_t valid_h_stop,
                               const index_t valid_w_start,
                               const index_t valid_w_stop,
                               float *output) {
#if !defined(MACE_ENABLE_NEON)
  MACE_UNUSED(valid_w_start);
  MACE_UNUSED(valid_w_stop);
#endif
  const index_t multiplier = out_shape[1] / in_shape[1];
  const index_t in_image_size = in_shape[2] * in_shape[3];
  const index_t out_image_size = out_shape[2] * out_shape[3];
  const index_t in_batch_size = in_shape[1] * in_image_size;
  const index_t out_batch_size = out_shape[1] * out_image_size;

#pragma omp parallel for collapse(2) schedule(runtime)
  for (index_t b = 0; b < in_shape[0]; ++b) {
    for (index_t m = 0; m < out_shape[1]; ++m) {
      index_t c = m / multiplier;
      index_t multi_index = m % multiplier;
      const float *in_base = input + b * in_batch_size + c * in_image_size;
      const float *filter_ptr = filter + multi_index * in_shape[1] * 9 + c * 9;
      float *out_base = output + b * out_batch_size + m * out_image_size;
      index_t h, w;
      const index_t pad_top = pad_hw[0];
      const index_t pad_left = pad_hw[1];
      const index_t out_width = out_shape[3];
      const index_t in_height = in_shape[2];
      const index_t in_width = in_shape[3];

      // top
      for (h = 0; h < valid_h_start; ++h) {
        for (w = 0; w < out_shape[3]; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h - pad_top,
                               w - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
        }
      }

#if defined(MACE_ENABLE_NEON)
      // load filter (1 outch x 3 height x 3 width): vf_outch_height
      float32x4_t vf00, vf01, vf02;
      vf00 = vld1q_f32(filter_ptr);
      vf01 = vld1q_f32(filter_ptr + 3);
      vf02 = vld1q_f32(filter_ptr + 5);

      for (h = valid_h_start; h + 1 < valid_h_stop; h += 2) {
        // left
        for (w = 0; w < valid_w_start; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h - pad_top,
                               w - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
          DepthwiseConv2dPixel(in_base, filter_ptr, h + 1, w, h + 1 - pad_top,
                               w - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
        }

        for (w = valid_w_start; w + 3 < valid_w_stop; w += 4) {
          // input (4 height x 3 slide): vi_height_slide
          float32x4_t vi00, vi01, vi02, vi0n;
          float32x4_t vi10, vi11, vi12, vi1n;
          float32x4_t vi20, vi21, vi22, vi2n;
          float32x4_t vi30, vi31, vi32, vi3n;

          // output (1 outch x 2 height x 4 width): vo_outch_height
          float32x4_t vo00, vo01;

          // load input
          index_t in_h = h - pad_top;
          index_t in_w = w - pad_left;
          index_t in_offset = in_h * in_width + in_w;
          vi00 = vld1q_f32(in_base + in_offset);
          vi0n = vld1q_f32(in_base + in_offset + 4);
          vi10 = vld1q_f32(in_base + in_offset + in_width);
          vi1n = vld1q_f32(in_base + in_offset + in_width + 4);
          vi20 = vld1q_f32(in_base + in_offset + 2 * in_width);
          vi2n = vld1q_f32(in_base + in_offset + 2 * in_width + 4);
          vi30 = vld1q_f32(in_base + in_offset + 3 * in_width);
          vi3n = vld1q_f32(in_base + in_offset + 3 * in_width + 4);

          vi01 = vextq_f32(vi00, vi0n, 1);
          vi02 = vextq_f32(vi00, vi0n, 2);
          vi11 = vextq_f32(vi10, vi1n, 1);
          vi12 = vextq_f32(vi10, vi1n, 2);
          vi21 = vextq_f32(vi20, vi2n, 1);
          vi22 = vextq_f32(vi20, vi2n, 2);
          vi31 = vextq_f32(vi30, vi3n, 1);
          vi32 = vextq_f32(vi30, vi3n, 2);

          // load ouptut
          index_t out_offset = h * out_width + w;
          vo00 = vld1q_f32(out_base + out_offset);
          vo01 = vld1q_f32(out_base + out_offset + out_width);

#if defined(__aarch64__)
          // outch 0, height 0
          vo00 = vfmaq_laneq_f32(vo00, vi00, vf00, 0);
          vo00 = vfmaq_laneq_f32(vo00, vi01, vf00, 1);
          vo00 = vfmaq_laneq_f32(vo00, vi02, vf00, 2);
          vo00 = vfmaq_laneq_f32(vo00, vi10, vf01, 0);
          vo00 = vfmaq_laneq_f32(vo00, vi11, vf01, 1);
          vo00 = vfmaq_laneq_f32(vo00, vi12, vf01, 2);
          vo00 = vfmaq_laneq_f32(vo00, vi20, vf02, 1);
          vo00 = vfmaq_laneq_f32(vo00, vi21, vf02, 2);
          vo00 = vfmaq_laneq_f32(vo00, vi22, vf02, 3);

          // outch 0, height 1
          vo01 = vfmaq_laneq_f32(vo01, vi10, vf00, 0);
          vo01 = vfmaq_laneq_f32(vo01, vi11, vf00, 1);
          vo01 = vfmaq_laneq_f32(vo01, vi12, vf00, 2);
          vo01 = vfmaq_laneq_f32(vo01, vi20, vf01, 0);
          vo01 = vfmaq_laneq_f32(vo01, vi21, vf01, 1);
          vo01 = vfmaq_laneq_f32(vo01, vi22, vf01, 2);
          vo01 = vfmaq_laneq_f32(vo01, vi30, vf02, 1);
          vo01 = vfmaq_laneq_f32(vo01, vi31, vf02, 2);
          vo01 = vfmaq_laneq_f32(vo01, vi32, vf02, 3);
#else
          // outch 0, height 0
          vo00 = vmlaq_lane_f32(vo00, vi00, vget_low_f32(vf00), 0);
          vo00 = vmlaq_lane_f32(vo00, vi01, vget_low_f32(vf00), 1);
          vo00 = vmlaq_lane_f32(vo00, vi02, vget_high_f32(vf00), 0);
          vo00 = vmlaq_lane_f32(vo00, vi10, vget_low_f32(vf01), 0);
          vo00 = vmlaq_lane_f32(vo00, vi11, vget_low_f32(vf01), 1);
          vo00 = vmlaq_lane_f32(vo00, vi12, vget_high_f32(vf01), 0);
          vo00 = vmlaq_lane_f32(vo00, vi20, vget_low_f32(vf02), 1);
          vo00 = vmlaq_lane_f32(vo00, vi21, vget_high_f32(vf02), 0);
          vo00 = vmlaq_lane_f32(vo00, vi22, vget_high_f32(vf02), 1);

          // outch 0, height 1
          vo01 = vmlaq_lane_f32(vo01, vi10, vget_low_f32(vf00), 0);
          vo01 = vmlaq_lane_f32(vo01, vi11, vget_low_f32(vf00), 1);
          vo01 = vmlaq_lane_f32(vo01, vi12, vget_high_f32(vf00), 0);
          vo01 = vmlaq_lane_f32(vo01, vi20, vget_low_f32(vf01), 0);
          vo01 = vmlaq_lane_f32(vo01, vi21, vget_low_f32(vf01), 1);
          vo01 = vmlaq_lane_f32(vo01, vi22, vget_high_f32(vf01), 0);
          vo01 = vmlaq_lane_f32(vo01, vi30, vget_low_f32(vf02), 1);
          vo01 = vmlaq_lane_f32(vo01, vi31, vget_high_f32(vf02), 0);
          vo01 = vmlaq_lane_f32(vo01, vi32, vget_high_f32(vf02), 1);
#endif
          vst1q_f32(out_base + out_offset, vo00);
          vst1q_f32(out_base + out_offset + out_width, vo01);
        }  // w

        // right
        for (; w < out_width; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h - pad_top,
                               w - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
          DepthwiseConv2dPixel(in_base, filter_ptr, h + 1, w, h + 1 - pad_top,
                               w - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
        }
      }  // h
#else
      for (index_t ih = valid_h_start; ih < valid_h_stop; ++ih) {
        for (index_t iw = 0; iw < out_shape[3]; ++iw) {
          DepthwiseConv2dPixel(in_base, filter_ptr, ih, iw, ih - pad_top,
                               iw - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
        }
      }
#endif

      // bottom
      for (; h < out_shape[2]; ++h) {
        for (w = 0; w < out_shape[3]; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h - pad_top,
                               w - pad_left, out_width, in_height, in_width, 3,
                               3, out_base);
        }
      }
    }  // m
  }    // b
}

void DepthwiseConv2dNeonK3x3S2(const float *input,
                               const float *filter,
                               const index_t *in_shape,
                               const index_t *out_shape,
                               const int *pad_hw,
                               const index_t valid_h_start,
                               const index_t valid_h_stop,
                               const index_t valid_w_start,
                               const index_t valid_w_stop,
                               float *output) {
#if !defined(MACE_ENABLE_NEON)
  MACE_UNUSED(valid_w_start);
  MACE_UNUSED(valid_w_stop);
#endif
  const index_t multiplier = out_shape[1] / in_shape[1];
  const index_t in_image_size = in_shape[2] * in_shape[3];
  const index_t out_image_size = out_shape[2] * out_shape[3];
  const index_t in_batch_size = in_shape[1] * in_image_size;
  const index_t out_batch_size = out_shape[1] * out_image_size;

#pragma omp parallel for collapse(2) schedule(runtime)
  for (index_t b = 0; b < in_shape[0]; ++b) {
    for (index_t m = 0; m < out_shape[1]; ++m) {
      index_t c = m / multiplier;
      index_t multi_index = m % multiplier;
      const float *in_base = input + b * in_batch_size + c * in_image_size;
      const float *filter_ptr = filter + multi_index * in_shape[1] * 9 + c * 9;
      float *out_base = output + b * out_batch_size + m * out_image_size;
      index_t h, w;
      const index_t pad_top = pad_hw[0];
      const index_t pad_left = pad_hw[1];
      const index_t out_width = out_shape[3];
      const index_t in_height = in_shape[2];
      const index_t in_width = in_shape[3];
      // top
      for (h = 0; h < valid_h_start; ++h) {
        for (w = 0; w < out_width; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h * 2 - pad_top,
                               w * 2 - pad_left, out_width, in_height, in_width,
                               3, 3, out_base);
        }
      }

#if defined(MACE_ENABLE_NEON)
      // load filter (1 outch x 3 height x 3 width): vf_outch_height
      float32x4_t vf00, vf01, vf02;
      vf00 = vld1q_f32(filter_ptr);
      vf01 = vld1q_f32(filter_ptr + 3);
      vf02 = vld1q_f32(filter_ptr + 5);

      for (h = valid_h_start; h < valid_h_stop; ++h) {
        // left
        for (w = 0; w < valid_w_start; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h * 2 - pad_top,
                               w * 2 - pad_left, out_width, in_height, in_width,
                               3, 3, out_base);
        }

        for (w = valid_w_start; w + 3 < valid_w_stop; w += 4) {
          float32x4x2_t vi0, vi1, vi2;
          float32x4_t vi0n, vi1n, vi2n;

          // input (3 height x 3 slide): vi_height_slide
          float32x4_t vi00, vi01, vi02;
          float32x4_t vi10, vi11, vi12;
          float32x4_t vi20, vi21, vi22;

          // output (1 outch x 1 height x 4 width): vo
          float32x4_t vo;

          // load input
          index_t in_h = h * 2 - pad_top;
          index_t in_w = w * 2 - pad_left;
          index_t in_offset = in_h * in_width + in_w;
          vi0 = vld2q_f32(in_base + in_offset);  // [0.2.4.6, 1.3.5.7]
          vi1 = vld2q_f32(in_base + in_offset + in_width);
          vi2 = vld2q_f32(in_base + in_offset + 2 * in_width);

          vi0n = vld1q_f32(in_base + in_offset + 8);  // [8.9.10.11]
          vi1n = vld1q_f32(in_base + in_offset + in_width + 8);
          vi2n = vld1q_f32(in_base + in_offset + 2 * in_width + 8);

          // load ouptut
          index_t out_offset = h * out_width + w;
          vo = vld1q_f32(out_base + out_offset);

          vi00 = vi0.val[0];                // [0.2.4.6]
          vi01 = vi0.val[1];                // [1.3.5.7]
          vi02 = vextq_f32(vi00, vi0n, 1);  // [2.4.6.8]
          vi10 = vi1.val[0];
          vi11 = vi1.val[1];
          vi12 = vextq_f32(vi10, vi1n, 1);
          vi20 = vi2.val[0];
          vi21 = vi2.val[1];
          vi22 = vextq_f32(vi20, vi2n, 1);

#if defined(__aarch64__)
          // outch 0, height 0
          vo = vfmaq_laneq_f32(vo, vi00, vf00, 0);
          vo = vfmaq_laneq_f32(vo, vi01, vf00, 1);
          vo = vfmaq_laneq_f32(vo, vi02, vf00, 2);
          vo = vfmaq_laneq_f32(vo, vi10, vf01, 0);
          vo = vfmaq_laneq_f32(vo, vi11, vf01, 1);
          vo = vfmaq_laneq_f32(vo, vi12, vf01, 2);
          vo = vfmaq_laneq_f32(vo, vi20, vf02, 1);
          vo = vfmaq_laneq_f32(vo, vi21, vf02, 2);
          vo = vfmaq_laneq_f32(vo, vi22, vf02, 3);
#else
          // outch 0, height 0
          vo = vmlaq_lane_f32(vo, vi00, vget_low_f32(vf00), 0);
          vo = vmlaq_lane_f32(vo, vi01, vget_low_f32(vf00), 1);
          vo = vmlaq_lane_f32(vo, vi02, vget_high_f32(vf00), 0);
          vo = vmlaq_lane_f32(vo, vi10, vget_low_f32(vf01), 0);
          vo = vmlaq_lane_f32(vo, vi11, vget_low_f32(vf01), 1);
          vo = vmlaq_lane_f32(vo, vi12, vget_high_f32(vf01), 0);
          vo = vmlaq_lane_f32(vo, vi20, vget_low_f32(vf02), 1);
          vo = vmlaq_lane_f32(vo, vi21, vget_high_f32(vf02), 0);
          vo = vmlaq_lane_f32(vo, vi22, vget_high_f32(vf02), 1);
#endif
          vst1q_f32(out_base + out_offset, vo);
        }  // w

        // right
        for (; w < out_width; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h * 2 - pad_top,
                               w * 2 - pad_left, out_width, in_height, in_width,
                               3, 3, out_base);
        }
      }  // h
#else
      for (index_t ih = valid_h_start; ih < valid_h_stop; ++ih) {
        for (index_t iw = 0; iw < out_width; ++iw) {
          DepthwiseConv2dPixel(in_base, filter_ptr, ih, iw, ih * 2 - pad_top,
                               iw * 2 - pad_left, out_width, in_height,
                               in_width, 3, 3, out_base);
        }
      }
#endif

      // bottom
      for (; h < out_shape[2]; ++h) {
        for (w = 0; w < out_shape[3]; ++w) {
          DepthwiseConv2dPixel(in_base, filter_ptr, h, w, h * 2 - pad_top,
                               w * 2 - pad_left, out_width, in_height, in_width,
                               3, 3, out_base);
        }
      }
    }  // m
  }    // b
}

}  // namespace ops
}  // namespace mace
