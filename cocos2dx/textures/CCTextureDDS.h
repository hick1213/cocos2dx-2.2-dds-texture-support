/****************************************************************************
Copyright (c) 2011 Jirka Fajfr for cocos2d-x
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#ifndef __CCDDSTEXTURE_H__
#define __CCDDSTEXTURE_H__

#include "CCStdC.h"
#include "CCGL.h"
#include "cocoa/CCObject.h"
#include "cocoa/CCArray.h"
#include "CCTexture2D.h"
#include <map>

//struct and data for s3tc(dds) struct
namespace
{
	typedef struct _MipmapInfo
	{
		unsigned char* address;
		int len;
		_MipmapInfo() :address(NULL), len(0){}
	}MipmapInfo;

	struct PixelFormatInfo {

		PixelFormatInfo(GLenum anInternalFormat, GLenum aFormat, GLenum aType, int aBpp, bool aCompressed, bool anAlpha)
			: internalFormat(anInternalFormat)
			, format(aFormat)
			, type(aType)
			, bpp(aBpp)
			, compressed(aCompressed)
			, alpha(anAlpha)
		{}

		GLenum internalFormat;
		GLenum format;
		GLenum type;
		int bpp;
		bool compressed;
		bool alpha;
	};

	typedef std::map<cocos2d::CCTexture2DPixelFormat, const PixelFormatInfo> PixelFormatInfoMap;


	struct DDColorKey
	{
		uint32_t colorSpaceLowValue;
		uint32_t colorSpaceHighValue;
	};

	struct DDSCaps
	{
		uint32_t caps;
		uint32_t caps2;
		uint32_t caps3;
		uint32_t caps4;
	};

	struct DDPixelFormat
	{
		uint32_t size;
		uint32_t flags;
		uint32_t fourCC;
		uint32_t RGBBitCount;
		uint32_t RBitMask;
		uint32_t GBitMask;
		uint32_t BBitMask;
		uint32_t ABitMask;
	};


	struct DDSURFACEDESC2
	{
		uint32_t size;
		uint32_t flags;
		uint32_t height;
		uint32_t width;

		union
		{
			uint32_t pitch;
			uint32_t linearSize;
		} DUMMYUNIONNAMEN1;

		union
		{
			uint32_t backBufferCount;
			uint32_t depth;
		} DUMMYUNIONNAMEN5;

		union
		{
			uint32_t mipMapCount;
			uint32_t refreshRate;
			uint32_t srcVBHandle;
		} DUMMYUNIONNAMEN2;

		uint32_t alphaBitDepth;
		uint32_t reserved;
		uint32_t surface;

		union
		{
			DDColorKey ddckCKDestOverlay;
			uint32_t emptyFaceColor;
		} DUMMYUNIONNAMEN3;

		DDColorKey ddckCKDestBlt;
		DDColorKey ddckCKSrcOverlay;
		DDColorKey ddckCKSrcBlt;

		union
		{
			DDPixelFormat ddpfPixelFormat;
			uint32_t FVF;
		} DUMMYUNIONNAMEN4;

		DDSCaps ddsCaps;
		uint32_t textureStage;
	};

#pragma pack(push,1)

	struct S3TCTexHeader
	{
		char fileCode[4];
		DDSURFACEDESC2 ddsd;
	};

#pragma pack(pop)

}
//s3tc struct end

namespace
{
	static const uint32_t makeFourCC(char ch0, char ch1, char ch2, char ch3)
	{
		const uint32_t fourCC = ((uint32_t)(char)(ch0) | ((uint32_t)(char)(ch1) << 8) | ((uint32_t)(char)(ch2) << 16) | ((uint32_t)(char)(ch3) << 24));
		return fourCC;
	}
}


NS_CC_BEGIN

/**
 * @addtogroup textures
 * @{
 */


/**
 @brief Determine how many mipmaps can we have. 
 Its same as define but it respects namespaces
*/
enum {
    CC_MIPMAP_MAX = 16,
};


class CCTextureDDS : public CCObject
{
public:
    CCTextureDDS();
    virtual ~CCTextureDDS();


	bool isS3TC(const unsigned char * data, long dataLen)
	{

		S3TCTexHeader *header = (S3TCTexHeader *)data;

		if (strncmp(header->fileCode, "DDS", 3) != 0)
		{
			return false;
		}
		return true;
	}

    /** initializes a CCTextureDDS with a path */
    bool initWithContentsOfFile(const char* path);

    /** creates and initializes a CCTextureDDS with a path */
    static CCTextureDDS* create(const char* path);
    
    // properties 
    
    /** texture id name */
    inline unsigned int getName() { return m_uName; }
    /** texture width */
    inline unsigned int getWidth() { return m_uWidth; }
    /** texture height */
    inline unsigned int getHeight() { return m_uHeight; }
    /** whether or not the texture has alpha */
    inline bool hasAlpha() { return m_bHasAlpha; }
    /** whether or not the texture has premultiplied alpha */
    inline bool hasPremultipliedAlpha() { return m_bHasPremultipliedAlpha; }
    /** how many mipmaps the texture has. 1 means one level (level 0 */
    inline unsigned int getNumberOfMipmaps() { return m_uNumberOfMipmaps; }
	inline CCTexture2DPixelFormat getFormat() { return m_eRenderFormat; }

    
protected:
	bool initWithMipmaps(MipmapInfo* mipmaps, int mipmapsNum, CCTexture2DPixelFormat pixelFormat, int pixelsWide, int pixelsHigh);
	bool initWithData(const void *data, long dataLen, CCTexture2DPixelFormat pixelFormat, int pixelsWide, int pixelsHigh, const CCSize& contentSize);

	MipmapInfo m_asMipmaps[CC_MIPMAP_MAX];   // pointer to mipmap images    
    unsigned int m_uNumberOfMipmaps;                    // number of mipmap used
    
    unsigned int m_uWidth, m_uHeight;

	CCSize m_obContentSize;

	unsigned int m_uDataLen;

	unsigned char *m_pData;

	CCTexture2DPixelFormat m_eRenderFormat;

    GLuint m_uName;
	/** texture max S */
	GLfloat m_nMaxS;

	/** texture max T */
	GLfloat m_nMaxT;
	bool m_bHasAlpha;
    bool m_bHasPremultipliedAlpha;
	bool m_bHasMipmaps;
    

	CC_SYNTHESIZE(bool, m_bAntialiasEnabled, AntialiasEnabled)
};

// end of textures group
/// @}

NS_CC_END


#endif //__CCDDSTEXTURE_H__