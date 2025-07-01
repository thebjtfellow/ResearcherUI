// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Edit - removing future stuff

#include "shader.h"
#include <QFile>
#include <QVulkanDeviceFunctions>
#include <QDialog>
#include <QFileDialog>

void Shader::load(QVulkanInstance *inst, VkDevice dev, const QString &fn)
{
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        QString shaderfn = QFileDialog::getOpenFileName(nullptr, QString("Could not find %1").arg(fn),"/home/","*.spv");
        if( shaderfn != ""){
            f.setFileName(shaderfn);
            if( !f.open(QIODevice::ReadOnly) ) {
                qWarning("Failed to open %s", qPrintable(fn));
                return;

            }
        }
    }
    QByteArray blob = f.readAll();
    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
    VkResult err = inst->deviceFunctions(dev)->vkCreateShaderModule(dev, &shaderInfo, nullptr, &m_data.shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        return;
    }
    return;
}

ShaderData *Shader::data()
{
    return &m_data;
}


