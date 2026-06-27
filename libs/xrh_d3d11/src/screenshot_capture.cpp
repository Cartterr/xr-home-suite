#include <xrh/d3d11/d3d11_renderer.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace xrh {

namespace {

struct FormatLayout {
    bool supported{false};
    bool bgra{false};
};

FormatLayout layoutFor(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return {true, false};
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return {true, true};
        default:
            return {};
    }
}

void writeLe16(std::ofstream& file, uint16_t value) {
    file.put(static_cast<char>(value & 0xff));
    file.put(static_cast<char>((value >> 8) & 0xff));
}

void writeLe32(std::ofstream& file, uint32_t value) {
    writeLe16(file, static_cast<uint16_t>(value & 0xffff));
    writeLe16(file, static_cast<uint16_t>((value >> 16) & 0xffff));
}

uint8_t composite(uint8_t value, uint8_t alpha, uint8_t background) {
    return static_cast<uint8_t>((static_cast<uint32_t>(value) * alpha +
                                static_cast<uint32_t>(background) * (255 - alpha)) / 255);
}

bool writeBmp(const std::filesystem::path& path,
              const D3D11_MAPPED_SUBRESOURCE& mapped,
              uint32_t sourceWidth,
              uint32_t sourceHeight,
              FormatLayout layout,
              uint32_t maxWidth) {
    const uint32_t outputWidth = std::max(1u, std::min(sourceWidth, maxWidth));
    const uint32_t outputHeight = std::max(1u, (sourceHeight * outputWidth) / sourceWidth);
    const uint32_t rowBytes = outputWidth * 3;
    const uint32_t paddedRowBytes = (rowBytes + 3u) & ~3u;
    const uint32_t pixelBytes = paddedRowBytes * outputHeight;
    const uint32_t fileHeaderBytes = 14;
    const uint32_t dibHeaderBytes = 40;
    const uint32_t pixelOffset = fileHeaderBytes + dibHeaderBytes;

    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.put('B');
    file.put('M');
    writeLe32(file, pixelOffset + pixelBytes);
    writeLe16(file, 0);
    writeLe16(file, 0);
    writeLe32(file, pixelOffset);
    writeLe32(file, dibHeaderBytes);
    writeLe32(file, outputWidth);
    writeLe32(file, outputHeight);
    writeLe16(file, 1);
    writeLe16(file, 24);
    writeLe32(file, 0);
    writeLe32(file, pixelBytes);
    writeLe32(file, 2835);
    writeLe32(file, 2835);
    writeLe32(file, 0);
    writeLe32(file, 0);

    std::vector<uint8_t> row(paddedRowBytes, 0);
    const auto* source = static_cast<const uint8_t*>(mapped.pData);
    for (uint32_t outY = outputHeight; outY-- > 0;) {
        std::fill(row.begin(), row.end(), 0);
        const uint32_t srcY = (outY * sourceHeight) / outputHeight;
        const uint8_t* sourceRow = source + static_cast<size_t>(srcY) * mapped.RowPitch;
        for (uint32_t outX = 0; outX < outputWidth; ++outX) {
            const uint32_t srcX = (outX * sourceWidth) / outputWidth;
            const uint8_t* pixel = sourceRow + static_cast<size_t>(srcX) * 4;
            const uint8_t r = layout.bgra ? pixel[2] : pixel[0];
            const uint8_t g = pixel[1];
            const uint8_t b = layout.bgra ? pixel[0] : pixel[2];
            const uint8_t a = pixel[3];
            row[outX * 3 + 0] = composite(b, a, 28);
            row[outX * 3 + 1] = composite(g, a, 30);
            row[outX * 3 + 2] = composite(r, a, 34);
        }
        file.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    return true;
}

std::filesystem::path capturePath(const ScreenshotCaptureConfig& config, uint32_t eye, uint64_t frameIndex) {
    std::ostringstream name;
    name << "openxr_probe-eye" << eye << "-frame" << std::setw(6) << std::setfill('0') << frameIndex << ".bmp";
    return config.directory / name.str();
}

}  // namespace

void D3D11Renderer::captureEyeIfNeeded(uint32_t eye,
                                       uint64_t frameIndex,
                                       double elapsedSeconds,
                                       ID3D11Texture2D* texture) {
    if (eye != 0 || screenshotConfig_.directory.empty() || screenshotConfig_.maxCaptures == 0) {
        return;
    }
    if (screenshotPaths_.size() >= screenshotConfig_.maxCaptures || elapsedSeconds < nextScreenshotAtSeconds_) {
        return;
    }

    try {
        D3D11_TEXTURE2D_DESC sourceDesc{};
        texture->GetDesc(&sourceDesc);
        const DXGI_FORMAT captureFormat =
            swapchainFormat_ == DXGI_FORMAT_UNKNOWN ? sourceDesc.Format : swapchainFormat_;
        const FormatLayout layout = layoutFor(captureFormat);
        if (!layout.supported) {
            std::cout << "Warning: screenshot skipped for unsupported format "
                      << static_cast<int>(captureFormat) << "\n";
            return;
        }

        D3D11_TEXTURE2D_DESC stagingDesc = sourceDesc;
        stagingDesc.Format = captureFormat;
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.SampleDesc.Quality = 0;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.MiscFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
        if (FAILED(device_->CreateTexture2D(&stagingDesc, nullptr, &staging))) {
            std::cout << "Warning: screenshot staging texture creation failed.\n";
            return;
        }

        if (sourceDesc.SampleDesc.Count > 1) {
            D3D11_TEXTURE2D_DESC resolveDesc = stagingDesc;
            resolveDesc.Usage = D3D11_USAGE_DEFAULT;
            resolveDesc.CPUAccessFlags = 0;
            Microsoft::WRL::ComPtr<ID3D11Texture2D> resolved;
            if (FAILED(device_->CreateTexture2D(&resolveDesc, nullptr, &resolved))) {
                std::cout << "Warning: screenshot resolve texture creation failed.\n";
                return;
            }
            context_->ResolveSubresource(resolved.Get(), 0, texture, 0, captureFormat);
            context_->CopyResource(staging.Get(), resolved.Get());
        } else {
            context_->CopyResource(staging.Get(), texture);
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
            std::cout << "Warning: screenshot texture map failed.\n";
            return;
        }
        const std::filesystem::path path = capturePath(screenshotConfig_, eye, frameIndex);
        const bool wrote = writeBmp(path,
                                    mapped,
                                    sourceDesc.Width,
                                    sourceDesc.Height,
                                    layout,
                                    std::max(64u, screenshotConfig_.maxWidth));
        context_->Unmap(staging.Get(), 0);

        if (wrote) {
            screenshotPaths_.push_back(path.string());
            nextScreenshotAtSeconds_ = elapsedSeconds + screenshotConfig_.intervalSeconds;
            std::cout << "Screenshot captured: " << path.string() << "\n";
        } else {
            std::cout << "Warning: screenshot write failed.\n";
        }
    } catch (const std::exception& error) {
        std::cout << "Warning: screenshot capture failed: " << error.what() << "\n";
    }
}

}  // namespace xrh
