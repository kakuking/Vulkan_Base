#pragma once
#include "utils.h"

#include "buffer.h"
#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class Texture{
    public:
        std::vector<const char*> filenames;
        std::vector<VkImage> textureImages;
        std::vector<VkDeviceMemory> textureImageMemories;
        std::vector<VkImageView> textureImageViews;
        std::vector<VkSampler> textureSamplers;

        Texture(){}

        void setupTexture(VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice, CommandBuffer& commandBufferManager){
            size_t numberOftextures = filenames.size();
            textureImages.resize(numberOftextures);
            textureImageViews.resize(numberOftextures);
            textureImageMemories.resize(numberOftextures);
            textureSamplers.resize(numberOftextures);

            for (size_t i = 0; i < filenames.size(); i++)
            {
                createTextureImage(i, graphicsQueue, device, physicalDevice, commandBufferManager);
                createTextureImageView(i, device);
                createTextureSampler(i, device, physicalDevice);
            }
        }

        void cleanupTexture(VkDevice device){
            for (size_t i = 0; i < filenames.size(); i++)
            {
                vkDestroyImageView(device, textureImageViews[i], nullptr);
                vkDestroyImage(device, textureImages[i], nullptr);
                vkFreeMemory(device, textureImageMemories[i], nullptr);
                vkDestroySampler(device, textureSamplers[i], nullptr);
            }
        }
    
    private:
        void createTextureImage(size_t currentImage, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice, CommandBuffer commandBufferManager){
            int texWidth, texHeight, texChannels;
            // stbi_uc* pixels = stbi_load("static\\texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            stbi_uc* pixels = stbi_load(filenames[currentImage], &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!pixels) {
                throw std::runtime_error("failed to load texture image!");
            }

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            Buffer::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, device, physicalDevice);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
                memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device, stagingBufferMemory);

            stbi_image_free(pixels);

            Image::createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImages[currentImage], textureImageMemories[currentImage], physicalDevice, device);

            Image::transitionImageLayout(textureImages[currentImage], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device, graphicsQueue, commandBufferManager);

            Image::copyBufferToImage(stagingBuffer, textureImages[currentImage], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), device, graphicsQueue, commandBufferManager);

            Image::transitionImageLayout(textureImages[currentImage], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, device, graphicsQueue,  commandBufferManager);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        void createTextureImageView(size_t currentImage, VkDevice device){
            textureImageViews[currentImage] = Image::createImageView(textureImages[currentImage], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, device);
        }

        void createTextureSampler(size_t currentImage, VkDevice device, VkPhysicalDevice physicalDevice){
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;

            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

            samplerInfo.unnormalizedCoordinates = VK_FALSE;

            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSamplers[currentImage]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }
        }
};