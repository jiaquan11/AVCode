#ifndef MYPLAYER_HEVC_SPS_PARSER_H_
#define MYPLAYER_HEVC_SPS_PARSER_H_

#include <vector>
#include <cstdint>
#include <iostream>
#include<string.h>
#include<stdio.h>

typedef unsigned char* LPBYTE;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef signed char  int8;
typedef signed short  int16;
typedef signed long  int32;

struct HevcSPSParams {
    int width, height;
    int profile, level;
    int nal_length_size;
    int max_ref_frames;
    void clear() {
        memset(this, 0, sizeof(*this));//clear all the data in the struct
    }
};

class NALBitstream {
public:
    NALBitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) {};
    NALBitstream(void* data, int len) { Init(data, len); };
    void Init(void* data, int len) {
        m_data = (LPBYTE)data;
        m_len = len;
        m_idx = 0;
        m_bits = 0;
        m_byte = 0;
        m_zeros = 0;
    };

    uint8 GetBYTE() {
        if (m_idx >= m_len) {
            return 0;
        }
        uint8 b = m_data[m_idx++];
        // to avoid start-code emulation, a byte 0x03 is inserted
        // after any 00 00 pair. Discard that here.
        if (b == 0) {
            m_zeros++;
            if ((m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03)) {
                m_idx++;
                m_zeros = 0;
            }
        } else {
            m_zeros = 0;
        }
        return b;
    };

    uint32 GetBit() {
        if (m_bits == 0) {
            m_byte = GetBYTE();
            m_bits = 8;
        }
        m_bits--;
        return (m_byte >> m_bits) & 0x1;
    };

    uint32 GetWord(int bits) {
        uint32 u = 0;
        while (bits > 0) {
            u <<= 1;
            u |= GetBit();
            bits--;
        }
        return u;
    };

    uint32 GetUE() {
        // Exp-Golomb entropy coding: leading zeros, then a one, then
        // the data bits. The number of leading zeros is the number of
        // data bits, counting up from that number of 1s as the base.
        // That is, if you see
        //      0001010
        // You have three leading zeros, so there are three data bits (010)
        // counting up from a base of 111: thus 111 + 010 = 1001 = 9
        int zeros = 0;
        while (m_idx < m_len && GetBit() == 0) {
            zeros++;
        }
        return GetWord(zeros) + ((1 << zeros) - 1);
    };

    int32 GetSE() {
        // same as UE but signed.
        // basically the unsigned numbers are used as codes to indicate signed numbers in pairs
        // in increasing value. Thus the encoded values
        //      0, 1, 2, 3, 4
        // mean
        //      0, 1, -1, 2, -2 etc
        uint32 UE = GetUE();
        bool positive = UE & 1;
        int32 SE = (UE + 1) >> 1;
        if (!positive) {
            SE = -SE;
        }
        return SE;
    };

private:
    LPBYTE m_data;
    int m_len;
    int m_idx;
    int m_bits;
    uint8 m_byte;
    int m_zeros;
};

bool ParseHEVCSPS(uint8* data, int size, HevcSPSParams& params) {
    if (size < 20) {
        LOGE("ParseHEVCSPS data size is too small");
        return false;
    }
    NALBitstream bs(data, size);

    // seq_parameter_set_rbsp()
    bs.GetWord(4);// sps_video_parameter_set_id
    int sps_max_sub_layers_minus1 = bs.GetWord(3); // "The value of sps_max_sub_layers_minus1 shall be in the range of 0 to 6, inclusive."
    if (sps_max_sub_layers_minus1 > 6) {
        return false;
    }
    bs.GetWord(1);// sps_temporal_id_nesting_flag
    // profile_tier_level( sps_max_sub_layers_minus1 )
    {
        bs.GetWord(2);// general_profile_space
        bs.GetWord(1);// general_tier_flag
        params.profile = bs.GetWord(5);// general_profile_idc
        bs.GetWord(32);// general_profile_compatibility_flag[32]
        bs.GetWord(1);// general_progressive_source_flag
        bs.GetWord(1);// general_interlaced_source_flag
        bs.GetWord(1);// general_non_packed_constraint_flag
        bs.GetWord(1);// general_frame_only_constraint_flag
        bs.GetWord(44);// general_reserved_zero_44bits
        params.level = bs.GetWord(8);// general_level_idc
        uint8 sub_layer_profile_present_flag[6] = { 0 };
        uint8 sub_layer_level_present_flag[6] = { 0 };
        for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
            sub_layer_profile_present_flag[i] = bs.GetWord(1);
            sub_layer_level_present_flag[i] = bs.GetWord(1);
        }

        if (sps_max_sub_layers_minus1 > 0) {
            for (int i = sps_max_sub_layers_minus1; i < 8; i++) {
                uint8 reserved_zero_2bits = bs.GetWord(2);
            }
        }

        for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
            if (sub_layer_profile_present_flag[i]) {
                bs.GetWord(2);// sub_layer_profile_space[i]
                bs.GetWord(1);// sub_layer_tier_flag[i]
                bs.GetWord(5);// sub_layer_profile_idc[i]
                bs.GetWord(32);// sub_layer_profile_compatibility_flag[i][32]
                bs.GetWord(1);// sub_layer_progressive_source_flag[i]
                bs.GetWord(1);// sub_layer_interlaced_source_flag[i]
                bs.GetWord(1);// sub_layer_non_packed_constraint_flag[i]
                bs.GetWord(1);// sub_layer_frame_only_constraint_flag[i]
                bs.GetWord(44);// sub_layer_reserved_zero_44bits[i]
            }
            if (sub_layer_level_present_flag[i]) {
                bs.GetWord(8);// sub_layer_level_idc[i]
            }
        }
    }

    uint32 sps_seq_parameter_set_id = bs.GetUE(); // "The  value  of sps_seq_parameter_set_id shall be in the range of 0 to 15, inclusive."
    if (sps_seq_parameter_set_id > 15) {
        return false;
    }

    uint32 chroma_format_idc = bs.GetUE(); // "The value of chroma_format_idc shall be in the range of 0 to 3, inclusive."
    if (sps_seq_parameter_set_id > 3) {
        return false;
    }

    if (chroma_format_idc == 3) {
        bs.GetWord(1);// separate_colour_plane_flag
    }

    params.width = bs.GetUE(); // pic_width_in_luma_samples
    params.height = bs.GetUE(); // pic_height_in_luma_samples

    if (bs.GetWord(1)) {// conformance_window_flag
        bs.GetUE();  // conf_win_left_offset
        bs.GetUE();  // conf_win_right_offset
        bs.GetUE();  // conf_win_top_offset
        bs.GetUE();  // conf_win_bottom_offset
    }

    uint32 bit_depth_luma_minus8 = bs.GetUE();
    uint32 bit_depth_chroma_minus8 = bs.GetUE();

    if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
        return false;
    }

    uint32 log2_max_pic_order_cnt_lsb_minus4 = bs.GetUE();
    uint32 sps_sub_layer_ordering_info_present_flag = bs.GetWord(1); // sps_sub_layer_ordering_info_present_flag
    int max_ref_frames = 0;
    int start_layer = sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1;
    for (int i = start_layer; i <= sps_max_sub_layers_minus1; ++i) {
        uint32 sps_max_dec_pic_buffering_minus1 = bs.GetUE();
        uint32 sps_max_num_reorder_pics = bs.GetUE();
        uint32 sps_max_latency_increase_plus1 = bs.GetUE();
        max_ref_frames = std::max(max_ref_frames, static_cast<int>(sps_max_dec_pic_buffering_minus1 + 1));
    }
    // Now we have maxRefFrames, we can add this value to params if needed.
    params.max_ref_frames = max_ref_frames;
    return true;
}

#endif //MYPLAYER_HEVC_SPS_PARSER_H_