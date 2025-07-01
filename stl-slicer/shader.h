// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// edit: removing QFuture stuff

#ifndef SHADER_H
#define SHADER_H

#include <QVulkanInstance>

struct ShaderData
{
    bool isValid() const { return shaderModule != VK_NULL_HANDLE; }
    VkShaderModule shaderModule = VK_NULL_HANDLE;
};

class Shader
{
public:
    void load(QVulkanInstance *inst, VkDevice dev, const QString &fn);
    ShaderData *data();
    bool isValid() { return m_data.isValid(); }

private:
    ShaderData m_data;
};

#endif
