#include "main.hpp"

void appvk::createUniformBuffers() {
    for (thing& t : things) {
        t.ubos = createBuffers(sizeof(ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, swapImages.size());
    }
}

void appvk::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    // images and samplers can actually be bound separately!
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = t.maps.size(); // descriptors for different kinds of maps
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(dev, &createInfo, nullptr, &t.layout) != VK_SUCCESS) {
        throw std::runtime_error("cannot create obj descriptor set!");
    }

    if (vkCreateDescriptorSetLayout(dev, &createInfo, nullptr, &flr.layout) != VK_SUCCESS) {
        throw std::runtime_error("cannot create floor descriptor set!");
    }
}

void appvk::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes;

    // reserve worst-case pool memory
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = things.size() * swapImages.size();

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = things.size() * t.maps.size() * swapImages.size();

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = swapImages.size() * things.size();
    createInfo.poolSizeCount = poolSizes.size();
    createInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(dev, &createInfo, nullptr, &dPool) != VK_SUCCESS) {
        throw std::runtime_error("cannot create descriptor pool!");
    }
}

void appvk::allocDescriptorSets(VkDescriptorPool pool, thing& t) {
    std::vector<VkDescriptorSetLayout> dLayout(swapImages.size(), t.layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = swapImages.size();
    allocInfo.pSetLayouts = dLayout.data();
    
    t.dsets.resize(swapImages.size());
    if (vkAllocateDescriptorSets(dev, &allocInfo, t.dsets.data()) != VK_SUCCESS) {
        throw std::runtime_error("cannot create descriptor set!");
    }
}

void appvk::allocDescriptorSetUniform(thing& t) {
    for (size_t i = 0; i < swapImages.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = t.ubos.bufs[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ubo);

        VkWriteDescriptorSet set{};
        set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        set.dstSet = t.dsets[i];
        set.dstBinding = 0;
        set.dstArrayElement = 0;
        set.descriptorCount = 1;
        set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        set.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(dev, 1, &set, 0, nullptr);
    }
}

void appvk::allocDescriptorSetTexture(thing& t, texture tex, size_t index) {
    for (size_t i = 0; i < swapImages.size(); i++) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = tex.samp;
        imageInfo.imageView = tex.view;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet set{};
        set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        set.dstSet = t.dsets[i];
        set.dstBinding = 1;
        set.dstArrayElement = index;
        set.descriptorCount = 1;
        set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        set.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(dev, 1, &set, 0, nullptr);
    }
}