/****************************************************************************
Copyright (c) 2011        Jirka Fajfr for cocos2d-x
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008      Apple Inc. All Rights Reserved.

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


#include "CCTextureDDS.h"
#include "ccMacros.h"
#include "CCConfiguration.h"
#include "support/ccUtils.h"
#include "CCStdC.h"
#include "platform/CCFileUtils.h"
#include "support/zip_support/ZipUtils.h"
#include "shaders/ccGLStateCache.h"
#include <ctype.h>
#include <cctype>

#include "s3tc.h"

NS_CC_BEGIN


typedef std::map<cocos2d::CCTexture2DPixelFormat, const PixelFormatInfo> PixelFormatInfoMap;
typedef PixelFormatInfoMap::value_type PixelFormatInfoMapValue;
static const PixelFormatInfoMapValue TexturePixelFormatInfoTablesValue[] =
{
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kTexture2DPixelFormat_RGBA8888, PixelFormatInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 32, false, true)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kTexture2DPixelFormat_RGBA4444, PixelFormatInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 16, false, true)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kTexture2DPixelFormat_RGB5A1, PixelFormatInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 16, false, true)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kTexture2DPixelFormat_RGB565, PixelFormatInfo(GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 16, false, false)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kTexture2DPixelFormat_RGB888, PixelFormatInfo(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 24, false, false)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kTexture2DPixelFormat_A8, PixelFormatInfo(GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, 8, false, false)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_I8, PixelFormatInfo(GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, 8, false, false)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_AI88, PixelFormatInfo(GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 16, false, true)),

#ifdef GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_PVRTC2, PixelFormatInfo(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, 0xFFFFFFFF, 0xFFFFFFFF, 2, true, false)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_PVRTC2A, PixelFormatInfo(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, 0xFFFFFFFF, 0xFFFFFFFF, 2, true, true)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_PVRTC4, PixelFormatInfo(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, 0xFFFFFFFF, 0xFFFFFFFF, 4, true, false)),
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_PVRTC4A, PixelFormatInfo(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, 0xFFFFFFFF, 0xFFFFFFFF, 4, true, true)),
#endif

#ifdef GL_ETC1_RGB8_OES
	PixelFormatInfoMapValue(kCCTexture2DPixelFormat_ETC, PixelFormatInfo(GL_ETC1_RGB8_OES, 0xFFFFFFFF, 0xFFFFFFFF, 4, true, false)),
#endif

#ifdef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_S3TC_DXT1, PixelFormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0xFFFFFFFF, 0xFFFFFFFF, 4, true, false)),
#endif

#ifdef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_S3TC_DXT3, PixelFormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0xFFFFFFFF, 0xFFFFFFFF, 8, true, false)),
#endif

#ifdef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
	PixelFormatInfoMapValue(CCTexture2DPixelFormat::kCCTexture2DPixelFormat_S3TC_DXT5, PixelFormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0xFFFFFFFF, 0xFFFFFFFF, 8, true, false)),
#endif

};
//The PixpelFormat corresponding information
const PixelFormatInfoMap _pixelFormatInfoTables(TexturePixelFormatInfoTablesValue,
	TexturePixelFormatInfoTablesValue + sizeof(TexturePixelFormatInfoTablesValue) / sizeof(TexturePixelFormatInfoTablesValue[0]));


CCTextureDDS::CCTextureDDS() 
: m_uNumberOfMipmaps(0)
, m_uWidth(0)
, m_uHeight(0)
, m_uName(0)
, m_bHasAlpha(false)
, m_bHasPremultipliedAlpha(true)
, m_eRenderFormat(kCCTexture2DPixelFormat_Default)
, m_pData(NULL)
, m_uDataLen(0)
{
}

CCTextureDDS::~CCTextureDDS()
{
	if(m_pData)
		free(m_pData);
}

bool CCTextureDDS::initWithContentsOfFile(const char* path)
{
    unsigned char* ddsdata = NULL;
    int ddslen = 0;
    
    std::string lowerCase(path);
    for (unsigned int i = 0; i < lowerCase.length(); ++i)
    {
        lowerCase[i] = tolower(lowerCase[i]);
    }
        
    if (lowerCase.find(".ccz") != std::string::npos)
    {
		ddslen = ZipUtils::ccInflateCCZFile(path, &ddsdata);
    }
    else if (lowerCase.find(".gz") != std::string::npos)
    {
		ddslen = ZipUtils::ccInflateGZipFile(path, &ddsdata);
    }
    else
    {
		ddsdata = CCFileUtils::sharedFileUtils()->getFileData(path, "rb", (unsigned long *)(&ddslen));
    }
    
	if (ddslen < 0 || !isS3TC(ddsdata,ddslen))
    {
		free(ddsdata);
        this->release();
        return false;
    }
    
	const uint32_t FOURCC_DXT1 = makeFourCC('D', 'X', 'T', '1');
	const uint32_t FOURCC_DXT3 = makeFourCC('D', 'X', 'T', '3');
	const uint32_t FOURCC_DXT5 = makeFourCC('D', 'X', 'T', '5');

	/* load the .dds file */

	S3TCTexHeader *header = (S3TCTexHeader *)ddsdata;
	unsigned char *pixelData = static_cast<unsigned char*>(malloc((ddslen - sizeof(S3TCTexHeader)) * sizeof(unsigned char)));
	memcpy((void *)pixelData, ddsdata + sizeof(S3TCTexHeader), ddslen - sizeof(S3TCTexHeader));

	m_uWidth = header->ddsd.width;
	m_uHeight = header->ddsd.height;
	m_uNumberOfMipmaps = MAX(1, header->ddsd.DUMMYUNIONNAMEN2.mipMapCount); //if dds header reports 0 mipmaps, set to 1 to force correct software decoding (if needed).
	m_uDataLen = 0;
	int blockSize = (FOURCC_DXT1 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC) ? 8 : 16;

	/* calculate the dataLen */

	int width = m_uWidth;
	int height = m_uHeight;

	if (CCConfiguration::sharedConfiguration()->supportsS3TC())  //compressed data length
	{
		m_uDataLen = ddslen - sizeof(S3TCTexHeader);
		m_pData = static_cast<unsigned char*>(malloc(m_uDataLen * sizeof(unsigned char)));
		memcpy((void *)m_pData, (void *)pixelData, m_uDataLen);
	}
	else                                               //decompressed data length
	{
		for (int i = 0; i < m_uNumberOfMipmaps && (width || height); ++i)
		{
			if (width == 0) width = 1;
			if (height == 0) height = 1;

			m_uDataLen += (height * width * 4);

			width >>= 1;
			height >>= 1;
		}
		m_pData = static_cast<unsigned char*>(malloc(m_uDataLen * sizeof(unsigned char)));
	}

	/* if hardware supports s3tc, set pixelformat before loading mipmaps, to support non-mipmapped textures  */
	if (CCConfiguration::sharedConfiguration()->supportsS3TC())
	{   //decode texture throught hardware

		if (FOURCC_DXT1 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC)
		{
			m_eRenderFormat = kCCTexture2DPixelFormat_S3TC_DXT1;
		}
		else if (FOURCC_DXT3 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC)
		{
			m_eRenderFormat = kCCTexture2DPixelFormat_S3TC_DXT3;
		}
		else if (FOURCC_DXT5 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC)
		{
			m_eRenderFormat = kCCTexture2DPixelFormat_S3TC_DXT5;
		}
	}
	else { //will software decode
		m_eRenderFormat = kCCTexture2DPixelFormat_RGBA8888;
	}

	/* load the mipmaps */

	int encodeOffset = 0;
	int decodeOffset = 0;
	width = m_uWidth;  height = m_uHeight;

	for (int i = 0; i < m_uNumberOfMipmaps && (width || height); ++i)
	{
		if (width == 0) width = 1;
		if (height == 0) height = 1;

		int size = ((width + 3) / 4)*((height + 3) / 4)*blockSize;

		if (CCConfiguration::sharedConfiguration()->supportsS3TC())
		{   //decode texture throught hardware
			m_asMipmaps[i].address = (unsigned char *)m_pData + encodeOffset;
			m_asMipmaps[i].len = size;
		}
		else
		{   //if it is not gles or device do not support S3TC, decode texture by software

			CCLOG("cocos2d: Hardware S3TC decoder not present. Using software decoder");

			int bytePerPixel = 4;
			unsigned int stride = width * bytePerPixel;

			std::vector<unsigned char> decodeImageData(stride * height);
			if (FOURCC_DXT1 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC)
			{
				s3tc_decode(pixelData + encodeOffset, &decodeImageData[0], width, height, S3TCDecodeFlag::DXT1);
			}
			else if (FOURCC_DXT3 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC)
			{
				s3tc_decode(pixelData + encodeOffset, &decodeImageData[0], width, height, S3TCDecodeFlag::DXT3);
			}
			else if (FOURCC_DXT5 == header->ddsd.DUMMYUNIONNAMEN4.ddpfPixelFormat.fourCC)
			{
				s3tc_decode(pixelData + encodeOffset, &decodeImageData[0], width, height, S3TCDecodeFlag::DXT5);
			}

			m_asMipmaps[i].address = (unsigned char *)m_pData + decodeOffset;
			m_asMipmaps[i].len = (stride * height);
			memcpy((void *)m_asMipmaps[i].address, (void *)&decodeImageData[0], m_asMipmaps[i].len);
			decodeOffset += stride * height;
		}

		encodeOffset += size;
		width >>= 1;
		height >>= 1;
	}

	/* end load the mipmaps */

	if (pixelData != nullptr)
	{
		free(pixelData);
	}


	int imageWidth = m_uHeight;
	int imageHeight = m_uWidth;

	CCConfiguration *conf = CCConfiguration::sharedConfiguration();

	int maxTextureSize = conf->getMaxTextureSize();
	if (imageWidth > maxTextureSize || imageHeight > maxTextureSize)
	{
		CCLOG("cocos2d: WARNING: Image (%u x %u) is bigger than the supported %u x %u", imageWidth, imageHeight, maxTextureSize, maxTextureSize);
		return false;
	}

	unsigned char*   tempData = m_pData;
	CCSize             imageSize = CCSize((float)imageWidth, (float)imageHeight);
	size_t	         tempDataLen = m_uDataLen;

	free(ddsdata);
	if (m_uNumberOfMipmaps > 1)
	{
		initWithMipmaps(m_asMipmaps, m_uNumberOfMipmaps, m_eRenderFormat, imageWidth, imageHeight);
		return true;
	}
	else
	{
		initWithData(tempData, tempDataLen, m_eRenderFormat, imageWidth, imageHeight, imageSize);
		return true;
	}
	

	return false;

}

bool CCTextureDDS::initWithData(const void *data, long dataLen, CCTexture2DPixelFormat pixelFormat, int pixelsWide, int pixelsHigh, const CCSize& contentSize)
{
	CCAssert(dataLen>0 && pixelsWide>0 && pixelsHigh>0, "Invalid size");

	//if data has no mipmaps, we will consider it has only one mipmap
	MipmapInfo mipmap;
	mipmap.address = (unsigned char*)data;
	mipmap.len = static_cast<int>(dataLen);
	return initWithMipmaps(&mipmap, 1, pixelFormat, pixelsWide, pixelsHigh);
}


bool CCTextureDDS::initWithMipmaps(MipmapInfo* mipmaps, int mipmapsNum, CCTexture2DPixelFormat pixelFormat, int pixelsWide, int pixelsHigh)
{

	if (mipmapsNum <= 0)
	{
		CCLOG("cocos2d: WARNING: mipmap number is less than 1");
		return false;
	}

	const PixelFormatInfo& info = _pixelFormatInfoTables.at(pixelFormat);

	if (info.compressed && !CCConfiguration::sharedConfiguration()->supportsPVRTC()
		&& !CCConfiguration::sharedConfiguration()->supportsS3TC())
	{
		CCLOG("cocos2d: WARNING: PVRTC/ETC images are not supported");
		return false;
	}

	//Set the row align only when mipmapsNum == 1 and the data is uncompressed
	if (mipmapsNum == 1 && !info.compressed)
	{
		unsigned int bytesPerRow = pixelsWide * info.bpp / 8;

		if (bytesPerRow % 8 == 0)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
		}
		else if (bytesPerRow % 4 == 0)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}
		else if (bytesPerRow % 2 == 0)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
		}
		else
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		}
	}
	else
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	if (m_uName != 0)
	{
		ccGLDeleteTexture(m_uName);
		m_uName = 0;
	}

	glGenTextures(1, &m_uName);
	ccGLBindTexture2D(m_uName);

	if (mipmapsNum == 1)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_bAntialiasEnabled ? GL_LINEAR : GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_bAntialiasEnabled ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_bAntialiasEnabled ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if CC_ENABLE_CACHE_TEXTURE_DATA
	if (_antialiasEnabled)
	{
		TexParams texParams = { (GLuint)(_hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR), GL_LINEAR, GL_NONE, GL_NONE };
		VolatileTextureMgr::setTexParameters(this, texParams);
	}
	else
	{
		TexParams texParams = { (GLuint)(_hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST), GL_NEAREST, GL_NONE, GL_NONE };
		VolatileTextureMgr::setTexParameters(this, texParams);
	}
#endif

	// clean possible GL error
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		CCLog("OpenGL error 0x%04X in %s %s %d\n", err, __FILE__, __FUNCTION__, __LINE__);
	}

	// Specify OpenGL texture image
	int width = pixelsWide;
	int height = pixelsHigh;

	for (int i = 0; i < mipmapsNum; ++i)
	{
		unsigned char *data = mipmaps[i].address;
		GLsizei datalen = mipmaps[i].len;

		if (info.compressed)
		{
			glCompressedTexImage2D(GL_TEXTURE_2D, i, info.internalFormat, (GLsizei)width, (GLsizei)height, 0, datalen, data);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, i, info.internalFormat, (GLsizei)width, (GLsizei)height, 0, info.format, info.type, data);
		}

		if (i > 0 && (width != height || ccNextPOT(width) != width))
		{
			CCLOG("cocos2d: Texture2D. WARNING. Mipmap level %u is not squared. Texture won't render correctly. width=%d != height=%d", i, width, height);
		}

		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
		{
			CCLOG("cocos2d: Texture2D: Error uploading compressed texture level: %u . glError: 0x%04X", i, err);
			return false;
		}

		width = MAX(width >> 1, 1);
		height = MAX(height >> 1, 1);
	}

	m_obContentSize = CCSize((float)pixelsWide, (float)pixelsHigh);
	
	m_nMaxS = 1;
	m_nMaxT = 1;

	m_bHasPremultipliedAlpha = false;
	m_bHasMipmaps = mipmapsNum > 1;

	return true;
}

CCTextureDDS * CCTextureDDS::create(const char* path)
{
    CCTextureDDS * pTexture = new CCTextureDDS();
    if (pTexture)
    {
        if (pTexture->initWithContentsOfFile(path))
        {
            pTexture->autorelease();
        }
        else
        {
            delete pTexture;
            pTexture = NULL;
        }
    }

    return pTexture;
}

NS_CC_END

