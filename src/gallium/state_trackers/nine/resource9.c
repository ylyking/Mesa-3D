/*
 * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
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

#include "resource9.h"
#include "device9.h"
#include "nine_helpers.h"
#include "nine_defines.h"

#include "pipe/p_screen.h"

#include "util/u_hash_table.h"
#include "util/u_inlines.h"
#include "util/u_resource.h"

#define DBG_CHANNEL DBG_RESOURCE

struct pheader
{
    boolean unknown;
    DWORD size;
    char data[1];
};

static int
ht_guid_compare( void *a,
                 void *b )
{
    return GUID_equal(a, b) ? 0 : 1;
}

static unsigned
ht_guid_hash( void *key )
{
    unsigned i, hash = 0;
    const unsigned char *str = key;

    for (i = 0; i < sizeof(GUID); i++) {
        hash = (unsigned)(str[i]) + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

static enum pipe_error
ht_guid_delete( void *key,
                void *value,
                void *data )
{
    struct pheader *header = value;

    if (header->unknown) { IUnknown_Release(*(IUnknown **)header->data); }
    FREE(header);

    return PIPE_OK;
}

HRESULT
NineResource9_ctor( struct NineResource9 *This,
                    struct NineUnknownParams *pParams,
                    struct NineDevice9 *pDevice,
                    struct pipe_resource *pResource,
                    D3DRESOURCETYPE Type,
                    D3DPOOL Pool )
{
    HRESULT hr = NineUnknown_ctor(&This->base, pParams);
    if (FAILED(hr)) { return hr; }

    This->device = pDevice;
    This->screen = NineDevice9_GetScreen(pDevice);

    /* NOTE: Take care to unreference the resource in the caller. */
    pipe_resource_reference(&This->resource, pResource);

    This->type = Type;
    This->pool = Pool;
    This->priority = 0;

    /* owners like VertexBuffer9 might fail to create the appropriate resource,
     * in which case we just chain up the ctor and return an error. */
    if (!This->resource) { return D3DERR_INVALIDCALL; }

    This->pdata = util_hash_table_create(ht_guid_hash, ht_guid_compare);
    if (!This->pdata) { return E_OUTOFMEMORY; }

    return D3D_OK;
}

void
NineResource9_dtor( struct NineResource9 *This )
{
    if (This->pdata) {
        util_hash_table_foreach(This->pdata, ht_guid_delete, NULL);
        util_hash_table_destroy(This->pdata);
    }

    if (This->resource) {
        if (This->resource->flags & NINE_RESOURCE_FLAG_DUMMY)
            FREE(This->resource);
        else
            /* NOTE: We do have to use refcounting, the driver might
             * still hold a reference. */
            pipe_resource_reference(&This->resource, NULL);
    }

#if 0
    switch (This->type) {
    case D3DRTYPE_TEXTURE:
    case D3DRTYPE_VOLUMETEXTURE:
    case D3DRTYPE_CUBETEXTURE:
    case D3DRTYPE_VERTEXBUFFER:
    case D3DRTYPE_INDEXBUFFER:
        /* This object owns the resource. Note that the ctor might have
         * been called with a NULL resource because the toplevel failed to
         * create it. */
        if (This->resource)
            This->screen->resource_destroy(This->screen, This->resource);
        break;

    case D3DRTYPE_SURFACE:
        if (This->resource &&
            (This->resource->flags & NINE_RESOURCE_FLAG_OWNED_BY_SURFACE))
            This->screen->resource_destroy(This->screen, This->resource);

    default:
        /* This object doesn't own the resource. There's also no point in
         * reference counting an object that gets destroyed on a whim. */
        break;
    }
#endif

    if (This->sysmem)
        FREE(This->sysmem);

    NineUnknown_dtor(&This->base);
}

struct pipe_resource *
NineResource9_GetResource( struct NineResource9 *This )
{
    return This->resource;
}

D3DPOOL
NineResource9_GetPool( struct NineResource9 *This )
{
    return This->pool;
}

HRESULT WINAPI
NineResource9_GetDevice( struct NineResource9 *This,
                         IDirect3DDevice9 **ppDevice )
{
    user_assert(ppDevice, E_POINTER);
    NineUnknown_AddRef(NineUnknown(This->device));
    *ppDevice = (IDirect3DDevice9 *)This->device;
    return D3D_OK;
}

HRESULT WINAPI
NineResource9_SetPrivateData( struct NineResource9 *This,
                              REFGUID refguid,
                              const void *pData,
                              DWORD SizeOfData,
                              DWORD Flags )
{
    enum pipe_error err;
    struct pheader *header;
    const void *user_data = pData;

    /* data consists of a header and the actual data. avoiding 2 mallocs */
    header = CALLOC_VARIANT_LENGTH_STRUCT(pheader, SizeOfData-1);
    if (!header) { return E_OUTOFMEMORY; }
    header->unknown = (Flags & D3DSPD_IUNKNOWN) ? TRUE : FALSE;

    /* if the refguid already exists, delete it */
    NineResource9_FreePrivateData(This, refguid);

    /* IUnknown special case */
    if (header->unknown) {
        if (user_error(SizeOfData == sizeof(IUnknown *))) {
            SizeOfData = sizeof(IUnknown *);
        }
        /* here the pointer doesn't point to the data we want, so point at the
         * pointer making what we eventually copy is the pointer itself */
        user_data = &pData;
    }

    header->size = SizeOfData;
    memcpy(header->data, user_data, header->size);

    err = util_hash_table_set(This->pdata, refguid, header);
    if (err == PIPE_OK) {
        if (header->unknown) { IUnknown_AddRef(*(IUnknown **)header->data); }
        return D3D_OK;
    }

    FREE(header);
    if (err == PIPE_ERROR_OUT_OF_MEMORY) { return E_OUTOFMEMORY; }

    return D3DERR_DRIVERINTERNALERROR;
}

HRESULT WINAPI
NineResource9_GetPrivateData( struct NineResource9 *This,
                              REFGUID refguid,
                              void *pData,
                              DWORD *pSizeOfData )
{
    struct pheader *header;

    user_assert(pSizeOfData, E_POINTER);

    header = util_hash_table_get(This->pdata, refguid);
    if (!header) { return D3DERR_NOTFOUND; }

    if (!pData) {
        *pSizeOfData = header->size;
        return D3D_OK;
    }
    if (*pSizeOfData < header->size) {
        return D3DERR_MOREDATA;
    }

    if (header->unknown) { IUnknown_AddRef(*(IUnknown **)header->data); }
    memcpy(pData, header->data, header->size);

    return D3D_OK;
}

HRESULT WINAPI
NineResource9_FreePrivateData( struct NineResource9 *This,
                               REFGUID refguid )
{
    struct pheader *header;

    header = util_hash_table_get(This->pdata, refguid);
    if (!header) { return D3DERR_NOTFOUND; }

    ht_guid_delete(NULL, header, NULL);

    return D3D_OK;
}

DWORD WINAPI
NineResource9_SetPriority( struct NineResource9 *This,
                           DWORD PriorityNew )
{
    DWORD prev = This->priority;
    This->priority = PriorityNew;
    return prev;
}

DWORD WINAPI
NineResource9_GetPriority( struct NineResource9 *This )
{
    return This->priority;
}

void WINAPI
NineResource9_PreLoad( struct NineResource9 *This )
{
    if (This->pool != D3DPOOL_MANAGED) { return; }

    /* XXX can we do anything here? */
    STUB();
}

D3DRESOURCETYPE WINAPI
NineResource9_GetType( struct NineResource9 *This )
{
    return This->type;
}

HRESULT
NineResource9_CreateDummyResource( struct NineResource9 *This,
                                   const struct pipe_resource *templ )
{
    struct pipe_resource *resource = MALLOC_STRUCT(pipe_resource);
    if (!resource)
        return E_OUTOFMEMORY;

    assert(templ->last_level == 0);
    assert(templ->nr_samples <= 1);

    pipe_reference_init(&resource->reference, 1);
    resource->screen = NULL;
    resource->target = templ->target;
    resource->format = templ->format;
    resource->width0 = templ->width0;
    resource->height0 = templ->height0;
    resource->depth0 = templ->depth0;
    resource->last_level = 0;
    resource->array_size = 1;
    resource->nr_samples = 0;
    resource->usage = PIPE_USAGE_DEFAULT;
    resource->bind = PIPE_BIND_TRANSFER_READ | PIPE_BIND_TRANSFER_WRITE;
    resource->flags = NINE_RESOURCE_FLAG_DUMMY;

    This->resource = resource;
    return D3D_OK;
}

HRESULT
NineResource9_AllocateData( struct NineResource9 *This )
{
    struct pipe_screen *screen = This->screen;
    struct pipe_resource *resource = NULL;

    if (This->pool == D3DPOOL_SYSTEMMEM &&
        This->type == D3DRTYPE_SURFACE &&
        This->resource && (This->resource->flags & NINE_RESOURCE_FLAG_DUMMY)) {
        /* Allocate a staging resource to save a copy:
         * user -> staging resource
         * staging resource -> (blit) -> video memory
         *
         * Instead of:
         * user -> system memory
         * system memory -> transfer staging area
         * transfer -> video memory
         *
         * Does this work if we "lose" the device ?
         */
        struct pipe_resource templ;
        templ.target = PIPE_TEXTURE_2D;
        templ.format = This->resource->format;
        templ.width0 = This->resource->width0;
        templ.height0 = This->resource->height0;
        templ.depth0 = 1;
        templ.array_size = 1;
        templ.last_level = 0;
        templ.nr_samples = 0;
        templ.usage = PIPE_USAGE_STAGING;
        templ.bind = PIPE_BIND_SAMPLER_VIEW;
        templ.flags = 0;
        resource = screen->resource_create(screen, &templ);
        if (resource) {
            FREE(This->resource);
            This->resource = resource;
        } else {
            DBG("Failed to allocate staging texture.\n");
        }
    }
    if (!resource) {
        const unsigned size = util_resource_size(This->resource);

        DBG("Allocating %f KiB of system memory.\n", (float)size / 1024.0f);
        assert(size);

        This->sysmem = MALLOC(size);
        if (!This->sysmem)
            return E_OUTOFMEMORY;
    }
    return D3D_OK;
}
