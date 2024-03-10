#if !defined(XI_XIA_H_)
#define XI_XIA_H_

#define XIA_HEADER_SIGNATURE FourCC('X', 'I', 'A', 'F')
#define XIA_HEADER_VERSION   2

typedef U32 Xia_AssetType;
enum {
    XIA_ASSET_TYPE_NONE = 0,
    XIA_ASSET_TYPE_IMAGE,
    XIA_ASSET_TYPE_SOUND,
    XIA_ASSET_TYPE_COUNT
};

#pragma pack(push, 1)

typedef struct Xia_Header Xia_Header;
struct Xia_Header {
    U32 signature;
    U32 version;

    U32 num_assets;

    U32 pad0[13];

    // we only need the info_offset, the str_table follows directly after and thus will be at
    // info_offset + (asset_count * sizeof(Xia_AssetInfo))
    //
    U64 info_offset;
    U64 str_table_size;

    U64 pad1[6];
};

StaticAssert(sizeof(Xia_Header) == 128);

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

typedef struct Xia_ImageInfo Xia_ImageInfo;
struct Xia_ImageInfo {
    U32 width;
    U32 height;
    U16 flags;       // reserved, maybe this should become a xi_u8 and the frame_count can be xi_u16
    U8  frame_count; // > 0 if animation
    U8  mip_levels;  // > 0 if sprite @unused: remove, not needed
};

typedef struct Xia_SoundInfo Xia_SoundInfo;
struct Xia_SoundInfo {
    U32 num_channels; // only 2 supported at the moment
    U32 num_samples;  // total number of samples
};

typedef struct Xia_AssetInfo Xia_AssetInfo;
struct Xia_AssetInfo {
    U64 offset;
    U32 size;
    U32 name_offset; // from the beginning of the string table, if 0 there is no name associated

    Xia_AssetType type;
    union {
        Xia_ImageInfo image;
        Xia_SoundInfo sound;
        U8 pad[36];
    };

    U64 write_time; // last write time of imported file
};

StaticAssert(sizeof(Xia_AssetInfo) == 64);

#pragma pack(pop)

#endif  // XI_XIA_H_
