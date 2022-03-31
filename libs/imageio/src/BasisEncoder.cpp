/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <imageio/BasisEncoder.h>

#include <image/ColorTransform.h>
#include <image/ImageOps.h>
#include <basisu_comp.h>

namespace image {

using Builder = BasisEncoder::Builder;

struct BasisEncoderBuilderImpl {
    basisu::basis_compressor_params params;
    bool grayscale = false;
    bool linear = false;
    bool normals = false;
    bool error = false;
};

struct BasisEncoderImpl {
    basisu::basis_compressor* encoder;
};

Builder::Builder(size_t mipCount) noexcept : mImpl(new BasisEncoderBuilderImpl) {
    const size_t imageCount = 1;
    const size_t mipsPerImage = mipCount > 0 ? mipCount : 1;
    mImpl->params.m_source_images.resize(imageCount);
    mImpl->params.m_source_mipmap_images.resize(imageCount);
    for (size_t imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        mImpl->params.m_source_mipmap_images[imageIndex].resize(mipCount - 1);
    }
}

Builder::~Builder() noexcept { delete mImpl; }

Builder::Builder(Builder&& that) noexcept {
    std::swap(mImpl, that.mImpl);
}

Builder& Builder::operator=(Builder&& that) noexcept {
    std::swap(mImpl, that.mImpl);
    return *this;
}

Builder& Builder::miplevel(size_t level, const LinearImage& floatImage) noexcept {
    const size_t imageIndex = 0;
    auto& basisBaseLevel = mImpl->params.m_source_images[imageIndex];
    auto& basisMipmaps = mImpl->params.m_source_mipmap_images[imageIndex];

    if (level >= basisMipmaps.size() + 1) {
        mImpl->error = true;
        return;
    }
    basisu::image* basisImage = level == 0 ? &basisBaseLevel : &basisMipmaps[level - 1];

    LinearImage sourceImage = mImpl->normals ? vectorsToColors(floatImage) : floatImage;

    std::unique_ptr<uint8_t[]> data;
    if (mImpl->linear) {
        data = fromLinearToRGB<uint8_t, 4>(sourceImage);
    } else {
        data = fromLinearTosRGB<uint8_t, 4>(sourceImage);
    }

    basisImage->resize(floatImage.getWidth(), floatImage.getHeight());
    memcpy(basisImage->get_ptr(), data.get(), basisImage->get_pitch() * basisImage->get_height());

    return *this;
}

Builder& Builder::grayscale(bool enabled) noexcept {
    mImpl->grayscale = enabled;
    return *this;
}

Builder& Builder::linear(bool enabled) noexcept {
    mImpl->linear = enabled;
    return *this;
}

Builder& Builder::normals(bool enabled) noexcept {
    mImpl->normals = enabled;
    return *this;
}

BasisEncoder* Builder::build() {
    if (mImpl->error) {
        return nullptr;
    }

    auto& params = mImpl->params;

    params.m_create_ktx2_file = true;
    params.m_ktx2_uastc_supercompression = basist::KTX2_SS_ZSTANDARD;

    // The incoming data is always linearized. If it were srgb, this would be true:
    params.m_ktx2_srgb_transfer_func = false;

    // This is the default zstd compression level used by the basisu cmdline cool.
    params.m_ktx2_zstd_supercompression_level = 6;

    // We do not want basis to read from files, we want it to read from "m_source_images"
    params.m_read_source_images = false;

    // We do not want basis to write the file, we want to manually dump "m_output_ktx2_file"
    params.m_write_output_basis_files = false;

    // TODO: pGlobal_codebook_data ? &pGlobal_codebook_data->m_transcoder.get_lowlevel_etc1s_decoder() : nullptr; 
    params.m_pGlobal_codebooks = nullptr;

    basisu::basis_compressor* encoder = new basisu::basis_compressor();

    return new BasisEncoder(new BasisEncoderImpl({encoder}));
}

BasisEncoder::BasisEncoder(BasisEncoderImpl* impl) noexcept : mImpl(impl) {}

BasisEncoder::~BasisEncoder() noexcept {
    delete mImpl->encoder;
    delete mImpl;
}

BasisEncoder::BasisEncoder(BasisEncoder&& that) noexcept {
    std::swap(mImpl, that.mImpl);
}

BasisEncoder& BasisEncoder::operator=(BasisEncoder&& that) noexcept {
    std::swap(mImpl, that.mImpl);
    return *this;
}

void BasisEncoder::startEncoding(uint32_t jobsCount) {
    // TODO
}

size_t BasisEncoder::getKtx2ByteCount() const noexcept {
    // TODO
    return 0;
}

uint8_t const* BasisEncoder::getKtx2Data() const noexcept {
    // TODO
    return 0;
}

} // namespace image
