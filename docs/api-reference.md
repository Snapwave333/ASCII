## FrameData

```cpp
struct FrameData {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> pixel_data; // BGRA pixel data (B8G8R8A8)
};
```

Important: Pixel data must be BGRA to match `VK_FORMAT_B8G8R8A8_UNORM`. Incorrect byte order causes red/blue inversion and blue-tinted frames.