#if !defined(XI_XIA_H_)
#define XI_XIA_H_

#define XIA_HEADER_SIGNATURE XI_FOURCC('X', 'I', 'A', 'F')
#define XIA_HEADER_VERSION   1

enum xiaAssetType {
    XIA_ASSET_TYPE_NONE = 0,
    XIA_ASSET_TYPE_IMAGE,
    XIA_ASSET_TYPE_COUNT
};

#pragma pack(push, 1)

typedef struct xiaHeader {
    xi_u32 signature;
    xi_u32 version;

    xi_u32 asset_count;

    xi_u32 pad0[13];

    // we only need the info_offset, the str_table follows directly after and thus will be at
    // info_offset + (asset_count * sizeof(xiaAssetInfo))
    //
    xi_u64 info_offset;
    xi_u64 str_table_size;

    xi_u64 pad1[6];
} xiaHeader;

XI_STATIC_ASSERT(sizeof(xiaHeader) == 128);

// :note we store generated mipmaps directly in the file, the decision for this was made to make
// the loading simpler. we will be loading into gpu mapped memory for our staging buffer which is very
// likely to be write-combined. to generate mipmaps at load time it would require us to read from this
// memory when downsizing which would cause all sorts of performance issues due to said write-combining.
//
// as the size of an asset doesn't really increase that much when considering mipmaps and this extra
// size only applies to sprites which are downsized to fit in the texture array anyway it would be benificial
// to store them and load them directly
//
// i.e. a texture array dimension of 256x256 would allow the max sprite to be 256x256x4 = 256K adding mipmaps
// only increases this to ~341K for the maximum size which is only a 33% increase
//

typedef struct xiaImageInfo {
    xi_u32 width;
    xi_u32 height;
    xi_u16 flags;       // reserved, maybe this should become a xi_u8 and the frame_count can be xi_u16
    xi_u8  frame_count; // > 0 if animation
    xi_u8  mip_levels;  // > 0 if sprite @unused: remove, not needed
} xiaImageInfo;

typedef struct xiaAssetInfo {
    xi_u64 offset;
    xi_u32 size;
    xi_u32 name_offset; // from the beginning of the string table, if 0 there is no name associated

    xi_u32 type;
    union {
        xiaImageInfo image;
        xi_u8 pad[36];
    };

    xi_u64 write_time; // last write time of imported file
} xiaAssetInfo;

XI_STATIC_ASSERT(sizeof(xiaAssetInfo) == 64);

#pragma pack(pop)

#endif  // XI_XIA_H_
