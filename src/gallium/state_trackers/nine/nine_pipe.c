/*
 * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
 * Copyright 2013 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "device9.h"
#include "nine_pipe.h"

#include "cso_cache/cso_context.h"

void
nine_convert_dsa_state(struct cso_context *ctx, const DWORD *rs)
{
    struct pipe_depth_stencil_alpha_state dsa;

    memset(&dsa, 0, sizeof(dsa)); /* memcmp safety */

    dsa.depth.enabled = !!rs[D3DRS_ZENABLE];
    dsa.depth.writemask = !!rs[D3DRS_ZWRITEENABLE];
    dsa.depth.func = d3dcmpfunc_to_pipe_func(rs[D3DRS_ZFUNC]);

    dsa.stencil[0].enabled = !!rs[D3DRS_STENCILENABLE];
    dsa.stencil[0].func = d3dcmpfunc_to_pipe_func(rs[D3DRS_STENCILFUNC]);
    dsa.stencil[0].fail_op = d3dstencilop_to_pipe_stencil_op(rs[D3DRS_STENCILFAIL]);
    dsa.stencil[0].zpass_op = d3dstencilop_to_pipe_stencil_op(rs[D3DRS_STENCILPASS]);
    dsa.stencil[0].zfail_op = d3dstencilop_to_pipe_stencil_op(rs[D3DRS_STENCILZFAIL]);
    dsa.stencil[0].valuemask = rs[D3DRS_STENCILMASK];
    dsa.stencil[0].writemask = rs[D3DRS_STENCILWRITEMASK];

    dsa.stencil[1].enabled = !!rs[D3DRS_TWOSIDEDSTENCILMODE];
    if (dsa.stencil[1].enabled) {
        dsa.stencil[1].func = d3dcmpfunc_to_pipe_func(rs[D3DRS_CCW_STENCILFUNC]);
        dsa.stencil[1].fail_op = d3dstencilop_to_pipe_stencil_op(rs[D3DRS_CCW_STENCILFAIL]);
        dsa.stencil[1].zpass_op = d3dstencilop_to_pipe_stencil_op(rs[D3DRS_CCW_STENCILPASS]);
        dsa.stencil[1].zfail_op = d3dstencilop_to_pipe_stencil_op(rs[D3DRS_CCW_STENCILZFAIL]);
        dsa.stencil[1].valuemask = dsa.stencil[0].valuemask;
        dsa.stencil[1].writemask = dsa.stencil[0].writemask;
    }

    dsa.alpha.enabled = !!rs[D3DRS_ALPHATESTENABLE];
    dsa.alpha.func = d3dcmpfunc_to_pipe_func(rs[D3DRS_ALPHAFUNC]);
    dsa.alpha.ref_value = (float)rs[D3DRS_ALPHAREF] / 255.0f;

    cso_set_depth_stencil_alpha(ctx, &dsa);
}

/* TODO: Keep a static copy in device so we don't have to memset every time ? */
void
nine_convert_rasterizer_state(struct cso_context *ctx, const DWORD *rs)
{
    struct pipe_rasterizer_state rast;

    memset(&rast, 0, sizeof(rast)); /* memcmp safety */

    rast.flatshade = rs[D3DRS_SHADEMODE] == D3DSHADE_FLAT;
 /* rast.light_twoside = 0; */
    rast.clamp_vertex_color = 1;
    rast.clamp_fragment_color = 1; /* XXX */
 /* rast.front_ccw = 0; */
    rast.cull_face = d3dcull_to_pipe_face(rs[D3DRS_CULLMODE]);
    rast.fill_front = d3dfillmode_to_pipe_polygon_mode(rs[D3DRS_FILLMODE]);
    rast.fill_back = rast.fill_front;
 /* rast.offset_point = 0; */ /* XXX */
 /* rast.offset_line = 0; */ /* XXX */
    rast.offset_tri = 1;
    rast.scissor = !!rs[D3DRS_SCISSORTESTENABLE];
 /* rast.poly_smooth = 0; */
 /* rast.poly_stipple_enable = 0; */
 /* rast.point_smooth = 0; */
    rast.sprite_coord_mode = PIPE_SPRITE_COORD_UPPER_LEFT;
    rast.point_quad_rasterization = !!rs[D3DRS_POINTSPRITEENABLE];
    rast.point_size_per_vertex = 1; /* XXX */
    rast.multisample = !!rs[D3DRS_MULTISAMPLEANTIALIAS];
    rast.line_smooth = !!rs[D3DRS_ANTIALIASEDLINEENABLE];
 /* rast.line_stipple_enable = 0; */
    rast.line_last_pixel = !!rs[D3DRS_LASTPIXEL];
    rast.flatshade_first = 1;
 /* rast.half_pixel_center = 0; */
 /* rast.lower_left_origin = 0; */
 /* rast.bottom_edge_rule = 0; */
 /* rast.rasterizer_discard = 0; */
    rast.depth_clip = 1;
    rast.clip_halfz = 1;
    rast.clip_plane_enable = rs[D3DRS_CLIPPLANEENABLE];
 /* rast.line_stipple_factor = 0; */
 /* rast.line_stipple_pattern = 0; */
 /* rast.sprite_coord_enable = 0x00; */
    rast.line_width = 1.0f;
    rast.point_size = asfloat(rs[D3DRS_POINTSIZE]); /* XXX: D3DRS_POINTSIZE_MIN/MAX */
    rast.offset_units = asfloat(rs[D3DRS_DEPTHBIAS]);
    rast.offset_scale = asfloat(rs[D3DRS_SLOPESCALEDEPTHBIAS]);
 /* rast.offset_clamp = 0.0f; */

    cso_set_rasterizer(ctx, &rast);
}

void
nine_convert_blend_state(struct cso_context *ctx, const DWORD *rs)
{
    struct pipe_blend_state blend;

    memset(&blend, 0, sizeof(blend)); /* memcmp safety */

    blend.dither = !!rs[D3DRS_DITHERENABLE];

 /* blend.alpha_to_one = 0; */
 /* blend.alpha_to_coverage = 0; */ /* XXX */

    blend.rt[0].blend_enable = !!rs[D3DRS_ALPHABLENDENABLE];
    if (blend.rt[0].blend_enable) {
        blend.rt[0].rgb_func = d3dblendop_to_pipe_blend(rs[D3DRS_BLENDOP]);
        blend.rt[0].rgb_src_factor = d3dblend_color_to_pipe_blendfactor(rs[D3DRS_SRCBLEND]);
        blend.rt[0].rgb_dst_factor = d3dblend_color_to_pipe_blendfactor(rs[D3DRS_DESTBLEND]);
        if (rs[D3DRS_SEPARATEALPHABLENDENABLE]) {
            blend.rt[0].alpha_func = d3dblendop_to_pipe_blend(rs[D3DRS_BLENDOPALPHA]);
            blend.rt[0].alpha_src_factor = d3dblend_alpha_to_pipe_blendfactor(rs[D3DRS_SRCBLENDALPHA]);
            blend.rt[0].alpha_dst_factor = d3dblend_alpha_to_pipe_blendfactor(rs[D3DRS_DESTBLENDALPHA]);
        } else {
            blend.rt[0].alpha_func = blend.rt[0].rgb_func;
            blend.rt[0].alpha_src_factor = d3dblend_alpha_to_pipe_blendfactor(rs[D3DRS_SRCBLEND]);
            blend.rt[0].alpha_dst_factor = d3dblend_alpha_to_pipe_blendfactor(rs[D3DRS_DESTBLEND]);
        }
    }
    blend.rt[0].colormask = rs[D3DRS_COLORWRITEENABLE];

    if (rs[D3DRS_COLORWRITEENABLE1] != rs[D3DRS_COLORWRITEENABLE] ||
        rs[D3DRS_COLORWRITEENABLE2] != rs[D3DRS_COLORWRITEENABLE] ||
        rs[D3DRS_COLORWRITEENABLE3] != rs[D3DRS_COLORWRITEENABLE]) {
        unsigned i;
        blend.independent_blend_enable = TRUE;
        for (i = 1; i < 4; ++i)
            blend.rt[i] = blend.rt[0];
        blend.rt[1].colormask = rs[D3DRS_COLORWRITEENABLE1];
        blend.rt[2].colormask = rs[D3DRS_COLORWRITEENABLE2];
        blend.rt[3].colormask = rs[D3DRS_COLORWRITEENABLE3];
    }

    /* blend.force_srgb = !!rs[D3DRS_SRGBWRITEENABLE]; */

    cso_set_blend(ctx, &blend);
}

void
nine_convert_sampler_state(struct cso_context *ctx, int idx, const DWORD *ss)
{
    struct pipe_sampler_state samp;

    memset(&samp, 0, sizeof(samp)); /* memcmp safety */

    samp.wrap_s = d3dtextureaddress_to_pipe_tex_wrap(ss[D3DSAMP_ADDRESSU]);
    samp.wrap_t = d3dtextureaddress_to_pipe_tex_wrap(ss[D3DSAMP_ADDRESSV]);
    samp.wrap_r = d3dtextureaddress_to_pipe_tex_wrap(ss[D3DSAMP_ADDRESSW]);
    samp.min_mip_filter = d3dtexturefiltertype_to_pipe_tex_mipfilter(ss[D3DSAMP_MIPFILTER]);
    samp.min_img_filter = d3dtexturefiltertype_to_pipe_tex_filter(ss[D3DSAMP_MINFILTER]);
    samp.mag_img_filter = d3dtexturefiltertype_to_pipe_tex_filter(ss[D3DSAMP_MAGFILTER]);
    samp.compare_mode = PIPE_TEX_COMPARE_NONE;
    samp.compare_func = PIPE_FUNC_NEVER;
    samp.normalized_coords = 1;
    samp.max_anisotropy = ss[D3DSAMP_MAXANISOTROPY];
    samp.seamless_cube_map = 1;
    samp.lod_bias = asfloat(ss[D3DSAMP_MIPMAPLODBIAS]);
    samp.min_lod = 0.0f;
    samp.max_lod = ss[D3DSAMP_MAXMIPLEVEL];
    samp.border_color.f[0] = ((ss[D3DSAMP_BORDERCOLOR] >>  0) & 0xff) / 255.0f;
    samp.border_color.f[1] = ((ss[D3DSAMP_BORDERCOLOR] >>  8) & 0xff) / 255.0f;
    samp.border_color.f[2] = ((ss[D3DSAMP_BORDERCOLOR] >> 16) & 0xff) / 255.0f;
    samp.border_color.f[3] = ((ss[D3DSAMP_BORDERCOLOR] >> 24) & 0xff) / 255.0f;

    cso_single_sampler(ctx, PIPE_SHADER_VERTEX, idx, &samp);
}

const enum pipe_format nine_d3d9_to_pipe_format_map[120] =
{
   [D3DFMT_UNKNOWN] = PIPE_FORMAT_NONE,

   [D3DFMT_A8R8G8B8]     = PIPE_FORMAT_B8G8R8A8_UNORM,
   [D3DFMT_X8R8G8B8]     = PIPE_FORMAT_B8G8R8X8_UNORM,
   [D3DFMT_R5G6B5]       = PIPE_FORMAT_B5G6R5_UNORM,
   [D3DFMT_X1R5G5B5]     = PIPE_FORMAT_B5G5R5X1_UNORM,
   [D3DFMT_A1R5G5B5]     = PIPE_FORMAT_B5G5R5A1_UNORM,
   [D3DFMT_A4R4G4B4]     = PIPE_FORMAT_B4G4R4A4_UNORM,
   [D3DFMT_A8]           = PIPE_FORMAT_A8_UNORM,
   [D3DFMT_X4R4G4B4]     = PIPE_FORMAT_B4G4R4X4_UNORM,
   [D3DFMT_R3G3B2]       = PIPE_FORMAT_B2G3R3_UNORM,
   [D3DFMT_A2B10G10R10]  = PIPE_FORMAT_R10G10B10A2_UNORM,
   [D3DFMT_A8B8G8R8]     = PIPE_FORMAT_R8G8B8A8_UNORM,
   [D3DFMT_X8B8G8R8]     = PIPE_FORMAT_R8G8B8X8_UNORM,
   [D3DFMT_G16R16]       = PIPE_FORMAT_R16G16_UNORM,
   [D3DFMT_A2R10G10B10]  = PIPE_FORMAT_B10G10R10A2_UNORM,
   [D3DFMT_A16B16G16R16] = PIPE_FORMAT_R16G16B16A16_UNORM,

   [D3DFMT_P8] = PIPE_FORMAT_R8_UINT,

   [D3DFMT_L8]   = PIPE_FORMAT_L8_UNORM,
   [D3DFMT_A8L8] = PIPE_FORMAT_L8A8_UNORM,
   [D3DFMT_A4L4] = PIPE_FORMAT_L4A4_UNORM,

   [D3DFMT_V8U8]        = PIPE_FORMAT_R8G8_SNORM,
   [D3DFMT_Q8W8V8U8]    = PIPE_FORMAT_R8G8B8A8_SNORM,
   [D3DFMT_V16U16]      = PIPE_FORMAT_R16G16_SNORM,
   [D3DFMT_A2W10V10U10] = PIPE_FORMAT_R10SG10SB10SA2U_NORM,

   /* [D3DFMT_UYVY] = PIPE_FORMAT_YUYV, fourcc */
   /* [D3DFMT_YUY2] = PIPE_FORMAT_NONE, fourcc */

   /* XXX: DXT2, DXT4 */
   /* fourcc
   [D3DFMT_DXT1] = PIPE_FORMAT_DXT1_RGBA,
   [D3DFMT_DXT2] = PIPE_FORMAT_DXT3_RGBA,
   [D3DFMT_DXT3] = PIPE_FORMAT_DXT3_RGBA,
   [D3DFMT_DXT4] = PIPE_FORMAT_DXT5_RGBA,
   [D3DFMT_DXT5] = PIPE_FORMAT_DXT5_RGBA,
   */

   /* XXX: order ? */
   /* fourcc
   [D3DFMT_G8R8_G8B8] = PIPE_FORMAT_G8R8_G8B8_UNORM,
   [D3DFMT_R8G8_B8G8] = PIPE_FORMAT_R8G8_B8G8_UNORM,
   */

   [D3DFMT_D16_LOCKABLE]  = PIPE_FORMAT_Z16_UNORM,
   [D3DFMT_D32]           = PIPE_FORMAT_Z32_UNORM,
   [D3DFMT_D24S8]         = PIPE_FORMAT_Z24_UNORM_S8_UINT,
   [D3DFMT_D24X8]         = PIPE_FORMAT_Z24X8_UNORM,
   [D3DFMT_D16]           = PIPE_FORMAT_Z16_UNORM,
   [D3DFMT_L16]           = PIPE_FORMAT_L16_UNORM,
   [D3DFMT_D32F_LOCKABLE] = PIPE_FORMAT_Z32_FLOAT,

   [D3DFMT_INDEX16]      = PIPE_FORMAT_R16_UINT,
   [D3DFMT_INDEX32]      = PIPE_FORMAT_R32_UINT,
   [D3DFMT_Q16W16V16U16] = PIPE_FORMAT_R16G16B16A16_SNORM,

   [D3DFMT_R16F]          = PIPE_FORMAT_R16_FLOAT,
   [D3DFMT_R32F]          = PIPE_FORMAT_R32_FLOAT,
   [D3DFMT_G16R16F]       = PIPE_FORMAT_R16G16_FLOAT,
   [D3DFMT_G32R32F]       = PIPE_FORMAT_R32G32_FLOAT,
   [D3DFMT_A16B16G16R16F] = PIPE_FORMAT_R16G16B16A16_FLOAT,
   [D3DFMT_A32B32G32R32F] = PIPE_FORMAT_R32G32B32A32_FLOAT,

   /* non-1:1 formats */
   [D3DFMT_R8G8B8]   = PIPE_FORMAT_B8G8R8X8_UNORM,
   [D3DFMT_A8R3G3B2] = PIPE_FORMAT_B8G8R8A8_UNORM,

   [D3DFMT_A8P8]     = PIPE_FORMAT_R8A8_UINT,

   [D3DFMT_D15S1]    = PIPE_FORMAT_Z24_UNORM_S8_UINT,
   [D3DFMT_D24X4S4]  = PIPE_FORMAT_Z24_UNORM_S8_UINT,
   [D3DFMT_D24FS8]   = PIPE_FORMAT_Z32_FLOAT_S8X24_UINT,

   /* not really formats */
   [D3DFMT_VERTEXDATA]   = PIPE_FORMAT_NONE,
   /* [D3DFMT_BINARYBUFFER] = PIPE_FORMAT_NONE, too large */

   /* unsupported formats */
   [D3DFMT_L6V5U5]      = PIPE_FORMAT_NONE,
   [D3DFMT_X8L8V8U8]    = PIPE_FORMAT_NONE,

   /* [D3DFMT_MULTI2_ARGB8] = PIPE_FORMAT_NONE, fourcc, MET */

   [D3DFMT_CxV8U8]              = PIPE_FORMAT_NONE,
   [D3DFMT_A1]                  = PIPE_FORMAT_NONE, /* XXX: add this ? */
   [D3DFMT_A2B10G10R10_XR_BIAS] = PIPE_FORMAT_NONE, /* XXX ? */
};

const D3DFORMAT nine_pipe_to_d3d9_format_map[PIPE_FORMAT_COUNT] =
{
   [PIPE_FORMAT_NONE]               = D3DFMT_UNKNOWN,

/* [PIPE_FORMAT_B8G8R8_UNORM]       = D3DFMT_R8G8B8, */
   [PIPE_FORMAT_B8G8R8A8_UNORM]     = D3DFMT_A8R8G8B8,
   [PIPE_FORMAT_B8G8R8X8_UNORM]     = D3DFMT_X8R8G8B8,
   [PIPE_FORMAT_B5G6R5_UNORM]       = D3DFMT_R5G6B5,
   [PIPE_FORMAT_B5G5R5X1_UNORM]     = D3DFMT_X1R5G5B5,
   [PIPE_FORMAT_B5G5R5A1_UNORM]     = D3DFMT_A1R5G5B5,
   [PIPE_FORMAT_B4G4R4A4_UNORM]     = D3DFMT_A4R4G4B4,
   [PIPE_FORMAT_B2G3R3_UNORM]       = D3DFMT_R3G3B2,
   [PIPE_FORMAT_A8_UNORM]           = D3DFMT_A8,
/* [PIPE_FORMAT_B2G3R3A8_UNORM]     = D3DFMT_A8R3G3B2, */
   [PIPE_FORMAT_B4G4R4X4_UNORM]     = D3DFMT_X4R4G4B4,
   [PIPE_FORMAT_R10G10B10A2_UNORM]  = D3DFMT_A2B10G10R10,
   [PIPE_FORMAT_R8G8B8A8_UNORM]     = D3DFMT_A8B8G8R8,
   [PIPE_FORMAT_R8G8B8X8_UNORM]     = D3DFMT_X8B8G8R8,
   [PIPE_FORMAT_R16G16_UNORM]       = D3DFMT_G16R16,
   [PIPE_FORMAT_B10G10R10A2_UNORM]  = D3DFMT_A2R10G10B10,
   [PIPE_FORMAT_R16G16B16A16_UNORM] = D3DFMT_A16B16G16R16,

   [PIPE_FORMAT_R8_UINT]            = D3DFMT_P8,
   [PIPE_FORMAT_R8A8_UINT]          = D3DFMT_A8P8,

   [PIPE_FORMAT_L8_UNORM]           = D3DFMT_L8,
   [PIPE_FORMAT_L8A8_UNORM]         = D3DFMT_A8L8,
   [PIPE_FORMAT_L4A4_UNORM]         = D3DFMT_A4L4,

   [PIPE_FORMAT_R8G8_SNORM]           = D3DFMT_V8U8,
/* [PIPE_FORMAT_?]                    = D3DFMT_L6V5U5, */
/* [PIPE_FORMAT_?]                    = D3DFMT_X8L8V8U8, */
   [PIPE_FORMAT_R8G8B8A8_SNORM]       = D3DFMT_Q8W8V8U8,
   [PIPE_FORMAT_R16G16_SNORM]         = D3DFMT_V16U16,
   [PIPE_FORMAT_R10SG10SB10SA2U_NORM] = D3DFMT_A2W10V10U10,

   [PIPE_FORMAT_YUYV]               = D3DFMT_UYVY,
/* [PIPE_FORMAT_YUY2]               = D3DFMT_YUY2, */
   [PIPE_FORMAT_DXT1_RGBA]          = D3DFMT_DXT1,
/* [PIPE_FORMAT_DXT2_RGBA]          = D3DFMT_DXT2, */
   [PIPE_FORMAT_DXT3_RGBA]          = D3DFMT_DXT3,
/* [PIPE_FORMAT_DXT4_RGBA]          = D3DFMT_DXT4, */
   [PIPE_FORMAT_DXT5_RGBA]          = D3DFMT_DXT5,
/* [PIPE_FORMAT_?]                  = D3DFMT_MULTI2_ARGB8, (MET) */
   [PIPE_FORMAT_R8G8_B8G8_UNORM]    = D3DFMT_R8G8_B8G8, /* XXX: order */
   [PIPE_FORMAT_G8R8_G8B8_UNORM]    = D3DFMT_G8R8_G8B8,

   [PIPE_FORMAT_Z16_UNORM]          = D3DFMT_D16_LOCKABLE,
   [PIPE_FORMAT_Z32_UNORM]          = D3DFMT_D32,
/* [PIPE_FORMAT_Z15_UNORM_S1_UINT]  = D3DFMT_D15S1, */
   [PIPE_FORMAT_Z24_UNORM_S8_UINT]  = D3DFMT_D24S8,
   [PIPE_FORMAT_Z24X8_UNORM]        = D3DFMT_D24X8,
   [PIPE_FORMAT_L16_UNORM]          = D3DFMT_L16,
   [PIPE_FORMAT_Z32_FLOAT]          = D3DFMT_D32F_LOCKABLE,
/* [PIPE_FORMAT_Z24_FLOAT_S8_UINT]  = D3DFMT_D24FS8, */

   [PIPE_FORMAT_R16_UINT]           = D3DFMT_INDEX16,
   [PIPE_FORMAT_R32_UINT]           = D3DFMT_INDEX32,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = D3DFMT_Q16W16V16U16,

   [PIPE_FORMAT_R16_FLOAT]          = D3DFMT_R16F,
   [PIPE_FORMAT_R32_FLOAT]          = D3DFMT_R32F,
   [PIPE_FORMAT_R16G16_FLOAT]       = D3DFMT_G16R16F,
   [PIPE_FORMAT_R32G32_FLOAT]       = D3DFMT_G32R32F,
   [PIPE_FORMAT_R16G16B16A16_FLOAT] = D3DFMT_A16B16G16R16F,
   [PIPE_FORMAT_R32G32B32A32_FLOAT] = D3DFMT_A32B32G32R32F,

/* [PIPE_FORMAT_?]                  = D3DFMT_CxV8U8, */
};