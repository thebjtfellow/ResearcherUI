#include "stlvulkanwindow.h"
#include <QFile>
#include <QLayout>
#include <QObject>
#include <QExposeEvent>
#include <QPaintEvent>

std::vector<VkVertexInputBindingDescription>vertex3f::getBindingDescriptions(){
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(vertex3f);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}
std::vector<VkVertexInputAttributeDescription>vertex3f::getAttributeDescriptions(){
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = sizeof(p3f);
    return attributeDescriptions;
}

STLVulkanWindowRenderer::STLVulkanWindowRenderer(STLVulkanWindow* viewer){
    view = viewer;
    vulkanInst = viewer->vulkanInstance();

}
STLVulkanWindowRenderer::~STLVulkanWindowRenderer(){
    vulkanInst = nullptr;
    view = nullptr;
}

void STLVulkanWindowRenderer::preInitResources(){
   // qDebug() << "-- preInitResources --";
    return;
}

void STLVulkanWindowRenderer::initResources(){
   // qDebug() << "-- initResources --";
    dev = view->device();
    devFuncs = vulkanInst->deviceFunctions(dev);
    physdevFuncs = vulkanInst->functions();

    // Initialize the pipeline info to default values
    // Also attach pipeline info to create gfx pipeline info struct
    defaultPipelineConfigInfo();

    // Now fill in the missing parts.
    // Add the shader information
    fragShader.load(vulkanInst, view->device(), QStringLiteral("fshader.spv"));
    vertShader.load( vulkanInst, view->device(), QStringLiteral("vshader.spv"));

    VkPipelineShaderStageCreateInfo shaderStages[2] =
    { { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShader.data()->shaderModule, "main", nullptr},
      { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShader.data()->shaderModule, "main", nullptr}
    };
    m_pipelineInfo.stageCount = 2;
    m_pipelineInfo.pStages = shaderStages;
   // m_pipelineInfo.stageCount = 0;
   // m_pipelineInfo.pStages = nullptr;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = VK_NULL_HANDLE;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;


    devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_pipelineInfo.layout);

    m_pipelineInfo.renderPass = view->defaultRenderPass();
    m_pipelineInfo.subpass = 0;
    QMatrix4x4 qm; qm.setToIdentity();
    VkResult vkr = devFuncs->vkCreateGraphicsPipelines(dev, nullptr, 1, &m_pipelineInfo, nullptr, &m_pipeline);

  //  qDebug() << "\n ** Size Of Push Constant Data" << sizeof(PushConstantData) << "  **\n";
  //  qDebug() << "\n ** Size Of QMatrix4x4.data()" << sizeof(QMatrix4x4) << "  **\n";
  //  qDebug() << "\n ** Size Of glm::mat4" << sizeof(glm::mat4) << "  **\n";
}

void STLVulkanWindowRenderer::initSwapChainResources(){
    //qDebug() << "-- initSwapChain --";

    return;
}
void STLVulkanWindowRenderer::releaseSwapChainResources(){
    //qDebug() << "-- releaseSwapChainResources --";
    return;
}
void STLVulkanWindowRenderer::releaseResources(){
    //qDebug() << "-- releaseResources --";
    devFuncs->vkDestroyPipeline(dev, m_pipeline, nullptr);
    devFuncs->vkDestroyPipelineLayout(dev, m_pipelineInfo.layout, nullptr);
    for( uint32_t objCt = 0; objCt < view->stlFiles.size(); objCt++ ){
        if( view->stlFiles[objCt].vertexBuffer != VK_NULL_HANDLE ){
            devFuncs->vkDestroyBuffer(dev, view->stlFiles[objCt].vertexBuffer, nullptr);
            devFuncs->vkFreeMemory(dev, view->stlFiles[objCt].vertexBufferMemory, nullptr);
        }
    }
    devFuncs->vkDestroyShaderModule(dev, vertShader.data()->shaderModule, nullptr );
    devFuncs->vkDestroyShaderModule(dev, fragShader.data()->shaderModule, nullptr );
    return;
}

void STLVulkanWindowRenderer::startNextFrame(){
    // Clear the background
    static uint64_t framesBy = 0;
 //   qDebug() << "startFrame:" << framesBy;
    if( this->view->m_loading ){
        view->frameReady();
        framesBy++;
 //       qDebug() << "Loading";
        return;
    }
    if( this->view->m_singleRefresh ){
        view->frameReady();
        framesBy++;
 //       qDebug() << "singleRefresh";
        this->view->m_singleRefresh = false;
        return;
    }
    VkClearColorValue clearColor = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    VkClearDepthStencilValue clearDS = { 1.0f, 0 };
    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;
 //   qDebug() << "VkRenderPassBeginInfo" << framesBy;
    VkRenderPassBeginInfo rpBeginInfo{};
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = view->defaultRenderPass();
    rpBeginInfo.framebuffer = view->currentFramebuffer();
    const QSize sz = view->swapChainImageSize();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;

    VkCommandBuffer cmdBuf = view->currentCommandBuffer();

    // See if there's STL data to render
    //if( view->getTriangleCount() > 0) qDebug() << ".";
    if( view->stlFiles.size() > 0 ){
        for( uint32_t ii = 0; ii < view->stlFiles.size(); ii++ )
            if( view->stlFiles[ii].vertexBuffer == VK_NULL_HANDLE ){
 //           qDebug() << "Making Buffer\n";
            makeSTLBuffer(ii);
            }
    }
 //   qDebug() << "vkCmdBeginRenderPass:" << framesBy;
    devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        0.0f, 0.0f,
        float(sz.width()), float(sz.height()),
        0.0f, 1.0f
    };

    devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor = {
        { 0, 0 },
        { uint32_t(sz.width()), uint32_t(sz.height()) }
    };
    devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    // We cleared the screen.
    // To draw the objects, we need to:
    // Bind the graphics pipeline to the command buffer
    // Bind the Vertex Data
    // Bind the Push Constant Data
    // Draw the frame
//    qDebug() << "vkCmdBindPipeline:" << framesBy;
    devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    uint32_t totalVC = 0;
    uint32_t bindCount = 0;

    if( view->stlFiles.size() > 0 )
        if( view->stlFiles[0].vertexBuffer != VK_NULL_HANDLE )
        {
            view->m_camera.setViewYXZ(view->m_cameraTransform.translation, view->m_cameraTransform.rotation);
            // setOrthographicProjection is actually a zoom function.
            view->m_camera.setOrthographicProjection(view->ortho_left,view->ortho_right,
                                                 view->ortho_top,view->ortho_bottom, 0.0f, view->m_sliceHeight );
            auto projectionView = view->m_camera.getProjection() * view->m_camera.getView();

            PushConstantData push{};
            auto modelMatrix = view->stlFiles[0].transform.mat4();

            push.transform = projectionView * modelMatrix;
            push.modelMatrix = modelMatrix;
//        qDebug() << "vkCmdPushConstants:" << framesBy;

            devFuncs->vkCmdPushConstants(cmdBuf, m_pipelineInfo.layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(PushConstantData), &push);
            for( uint32_t uit=0; uit < view->stlFiles.size(); uit++ ){
                if( view->stlFiles[uit].vertexBuffer != VK_NULL_HANDLE && view->stlFiles[uit].bindData )
                {
                    VkBuffer buffers[] = {view->stlFiles[uit].vertexBuffer};
                    VkDeviceSize offsets[] = {0};
                    devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, buffers, offsets);
                    devFuncs->vkCmdDraw(cmdBuf, view->stlFiles[uit].vertexCount, 1, 0, 0);
                }
            }
        }

//    qDebug() << "vkCmdEndRenderPass:" << framesBy;
    devFuncs->vkCmdEndRenderPass(cmdBuf);
//    qDebug() << "frameReady:" << framesBy;
    view->frameReady();
    //view->requestUpdate();
    framesBy++;
    return;
}

void STLVulkanWindowRenderer::makeSTLBuffer(uint32_t index) {

    m_vertexCount = static_cast<uint32_t>(view->stlFiles[index].vertices.size());

    //assert(vertexCount >= 3 && "Vertex count must be at least 3");

    VkDeviceSize bufferSize = sizeof(view->stlFiles[index].vertices[0]) * m_vertexCount;
    //qDebug() << "Making buffer for " << m_vertexCount << " verices of size " << bufferSize;
    void* data;

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        view->stlFiles[index].vertexBuffer,
        view->stlFiles[index].vertexBufferMemory);
    // Now copy the data in

    devFuncs->vkMapMemory(dev, view->stlFiles[index].vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, view->stlFiles[index].vertices.data(), bufferSize);
    devFuncs->vkUnmapMemory(dev, view->stlFiles[index].vertexBufferMemory);
    data = nullptr;

}
void STLVulkanWindowRenderer::destroySTLData(STLFileData &stl){
    if( stl.vertexBuffer != VK_NULL_HANDLE ){
        this->devFuncs->vkDestroyBuffer(this->dev, stl.vertexBuffer, nullptr);
        stl.vertexBuffer = VK_NULL_HANDLE;
    }
    if( stl.vertexBufferMemory != VK_NULL_HANDLE ){
        this->devFuncs->vkFreeMemory(this->dev, stl.vertexBufferMemory, nullptr);
        stl.vertexBufferMemory = VK_NULL_HANDLE;
    }
    return;
}

void STLVulkanWindowRenderer::createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VkDeviceMemory &bufferMemory) {

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if( devFuncs->vkCreateBuffer(dev, &bufferInfo, nullptr, &buffer) != VK_SUCCESS ){
        //qDebug() << "  **  Failed to create buffer\n";
    }

    VkMemoryRequirements memRequirements{};
    devFuncs->vkGetBufferMemoryRequirements(dev, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (devFuncs->vkAllocateMemory(dev, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        qDebug("  **  failed to allocate buffer memory!");
    }

    if ( devFuncs->vkBindBufferMemory(dev, buffer, bufferMemory, 0) != VK_SUCCESS ){
        qDebug("  **  Failed to bind buffer and memory\n");
    }
}
uint32_t STLVulkanWindowRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  physdevFuncs->vkGetPhysicalDeviceMemoryProperties(view->physicalDevice(), &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}
void STLVulkanWindowRenderer::defaultPipelineConfigInfo() {

    m_pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    m_pipelineInfo.pColorBlendState = &m_configInfo.colorBlendInfo;
    m_pipelineInfo.pDepthStencilState = &m_configInfo.depthStencilInfo;
    m_pipelineInfo.pDynamicState = &m_configInfo.dynamicStateInfo;
    m_pipelineInfo.pInputAssemblyState = & m_configInfo.inputAssemblyInfo;
    m_pipelineInfo.pMultisampleState = &m_configInfo.multisampleInfo;
    m_pipelineInfo.pNext = VK_NULL_HANDLE;
    m_pipelineInfo.pRasterizationState = &m_configInfo.rasterizationInfo;
    m_pipelineInfo.pStages = VK_NULL_HANDLE;
    m_pipelineInfo.pTessellationState = VK_NULL_HANDLE;
    m_pipelineInfo.pVertexInputState = &m_configInfo.vertexInfo;
    m_pipelineInfo.pViewportState = &m_configInfo.viewportInfo;
    m_pipelineInfo.renderPass = VK_NULL_HANDLE;
    m_pipelineInfo.stageCount = 0;
    m_pipelineInfo.subpass = 0;
    m_pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    m_pipelineInfo.basePipelineIndex = -1;
    m_pipelineInfo.layout = VK_NULL_HANDLE;
    m_pipelineInfo.flags=0;

    m_configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    m_configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_configInfo.viewportInfo.viewportCount = 1;
    m_configInfo.viewportInfo.pViewports = nullptr;
    m_configInfo.viewportInfo.scissorCount = 1;
    m_configInfo.viewportInfo.pScissors = nullptr;

    m_configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    m_configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    m_configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    m_configInfo.rasterizationInfo.lineWidth = 1.0f;
    m_configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    m_configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
    m_configInfo.rasterizationInfo.depthBiasConstantFactor = 1.0f;
    m_configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
    m_configInfo.rasterizationInfo.depthBiasSlopeFactor = 1.0f;

    m_configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    m_configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_configInfo.multisampleInfo.minSampleShading = 1.0f;
    m_configInfo.multisampleInfo.pSampleMask = nullptr;
    m_configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    m_configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

    m_configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    m_configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    m_configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    m_configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_OR;
    m_configInfo.colorBlendInfo.attachmentCount = 1;
    m_configInfo.colorBlendInfo.pAttachments = &m_configInfo.colorBlendAttachment;
    m_configInfo.colorBlendInfo.blendConstants[0] = 1.0f;
    m_configInfo.colorBlendInfo.blendConstants[1] = 1.0f;
    m_configInfo.colorBlendInfo.blendConstants[2] = 1.0f;
    m_configInfo.colorBlendInfo.blendConstants[3] = 1.0f;

    m_configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    m_configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    m_configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    m_configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    m_configInfo.depthStencilInfo.minDepthBounds = 0.0f;
    m_configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
    m_configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    m_configInfo.depthStencilInfo.front = {};
    m_configInfo.depthStencilInfo.back = {};

    m_configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    m_configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_configInfo.dynamicStateInfo.pDynamicStates = m_configInfo.dynamicStateEnables.data();
    m_configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(m_configInfo.dynamicStateEnables.size());
    m_configInfo.dynamicStateInfo.flags = 0;


    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(vertex3f);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(p3f);

    m_configInfo.vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_configInfo.vertexInfo.vertexBindingDescriptionCount = 1;
    m_configInfo.vertexInfo.pVertexBindingDescriptions = &bindingDescription;
    m_configInfo.vertexInfo.vertexAttributeDescriptionCount = 2;
    m_configInfo.vertexInfo.pVertexAttributeDescriptions = &attributeDescriptions[0];

}

STLVulkanWindow::STLVulkanWindow()
{   // Persistent resources is keeping it from crashing for now
    QVulkanWindow::Flags flags{};
    flags = QVulkanWindow::PersistentResources;
    this->setFlags(flags);
    //m_camera.setViewDirection(glm::vec3(0.0f,0.0f,-5.0f), glm::vec3(0.0f,0.0f,0.0f));
    m_camera.setOrthographicProjection(32.512f, 32.512f, -32.512f, 32.512f, -1.0f, 60.0f);
    m_cameraTransform.translation = {0.0f, 0.0f, 0.0f};
    m_cameraTransform.rotation = { glm::pi<float>(), 0.0f, 0.0f};
    stlFiles.clear();

}
void STLVulkanWindow::predelete(){
    //delete m_renderer;
    return;
}
STLVulkanWindow::~STLVulkanWindow(){

    //delete m_renderer;
}
QVulkanWindowRenderer* STLVulkanWindow::createRenderer() {
    m_renderer = new STLVulkanWindowRenderer(this);
    return m_renderer;
}

#include <QtEndian>
const float magicv = 0.000030517578125;

bool STLVulkanWindow::loadSTLFile(QString filePath) {
    // Load the file data into an undefined blob
    QFile f(filePath);
    if( !f.open(QIODevice::ReadOnly)){
        qWarning("Failed to open file");
        return false;
    }

    float x, y, z;
    char header[80];
    uint16_t trash;
    p3f nread;
    p3f pread;
    STLFileData loadData;

//    vertices.clear();

    f.read(header, 80);
    f.read((char*)&loadData.triangleCount, 4);
    //dataSize = m_triangleCount * sizeof(p3f);
    //qDebug() << "Loading " << loadData.triangleCount << " triangles\n";
    //qDebug() << "sizeof(p3f)" << sizeof(p3f) << "\n";
    for( int i=0; i<loadData.triangleCount; i++){ // Data blob of just raw vertex data from the triangles
        f.read((char*) &x, 4); x = abs(x) < magicv ? 0 : x;
        f.read((char*) &y, 4); y = abs(y) < magicv ? 0 : y;
        f.read((char*) &z, 4); z = abs(z) < magicv ? 0 : z;
        nread.x = x;
        nread.y = y;
        nread.z = z;
        if( this->stlFiles.size() == 0 ){nread.x = 1.0f; nread.y = 0.0f; nread.z = 0.0f;}
        if( this->stlFiles.size() == 1 ){nread.x = 0.0f; nread.y = 1.0f; nread.z = 0.0f;}
        if( this->stlFiles.size() == 2 ){nread.x = 0.0f; nread.y = 0.0f; nread.z = 1.0f;}
        for( int i2=0; i2<3 ; i2++ ){
            vertex3f newV{};
            newV.normal = nread;
            f.read((char*) &x, 4); x = abs(x) < magicv ? 0 : x;
            f.read((char*) &y, 4); y = abs(y) < magicv ? 0 : y;
            f.read((char*) &z, 4); z = abs(z) < magicv ? 0 : z;
            pread.x = x;
            pread.y = y;
            pread.z = z;
            newV.position = pread;
            loadData.vertices.push_back(newV);
            loadData.maxX = pread.x > loadData.maxX ? pread.x : loadData.maxX;
            loadData.maxY = pread.y > loadData.maxY ? pread.y : loadData.maxY;
            loadData.maxZ = pread.z > loadData.maxZ ? pread.z : loadData.maxZ;
            loadData.minX = pread.x < loadData.minX ? pread.x : loadData.minX;
            loadData.minY = pread.y < loadData.minY ? pread.y : loadData.minY;
            loadData.minZ = pread.z < loadData.minZ ? pread.z : loadData.minZ;
//            if( i % 20 == 0 && i2 == 0){
//            qDebug() << "Vertex " << (i*3) << "\n";
//            qDebug() << "Normal " << newV.normal.x << "," << newV.normal.y << "," << newV.normal.z << "\n";
//            qDebug() << "Position " << newV.position.x << "," << newV.position.y << "," << newV.position.z << "\n";
//            }
        }
        f.read((char*)&trash,2);
    }

    // center model shift values
    float x_shift = -1.0f*((loadData.maxX + loadData.minX)/2.0f);
    float y_shift = -1.0f*((loadData.maxY + loadData.minY)/2.0f);

    // translate model into positive Z space
    for( int i = 0; i<loadData.vertices.size(); i++ ){
        loadData.vertices[i].position.x += x_shift;
        loadData.vertices[i].position.y += y_shift;
        loadData.vertices[i].position.z -= loadData.minZ;
    }

    loadData.minX = 1000.0; loadData.maxX = -1000.0;
    loadData.minY = 1000.0; loadData.maxY = -1000.0;
    loadData.minZ = 1000.0; loadData.maxZ = -1000.0;
    for( int i = 0; i<loadData.vertices.size(); i++ ){
        loadData.minX = loadData.vertices[i].position.x < loadData.minX ? loadData.vertices[i].position.x : loadData.minX;
        loadData.minY = loadData.vertices[i].position.y < loadData.minY ? loadData.vertices[i].position.y : loadData.minY;
        loadData.minZ = loadData.vertices[i].position.z < loadData.minZ ? loadData.vertices[i].position.z : loadData.minZ;
        loadData.maxX = loadData.vertices[i].position.x > loadData.maxX ? loadData.vertices[i].position.x : loadData.maxX;
        loadData.maxY = loadData.vertices[i].position.y > loadData.maxY ? loadData.vertices[i].position.y : loadData.maxY;
        loadData.maxZ = loadData.vertices[i].position.z > loadData.maxZ ? loadData.vertices[i].position.z : loadData.maxZ;
    }
    loadData.vertexCount = loadData.vertices.size();
    loadData.bindData = true;
    loadData.width = loadData.maxX - loadData.minX;
    loadData.length = loadData.maxY - loadData.minY;
    loadData.height = loadData.maxZ;

    this->stlFiles.push_back(loadData);

    return true;
}
bool STLVulkanWindow::unloadAllFiles(){
    if( stlFiles.size() > 0 ){
        for( auto& stl : stlFiles ){
            m_renderer->destroySTLData(stl);
            stl.reset();
        }
       stlFiles.clear();
    }
    return true;
}
void STLVulkanWindow::exposeEvent(QExposeEvent *e){
   /* if( this->m_singleRefresh ){
        //qDebug() << "-- Exposed-- ";
        //qDebug() << "-- Type -- :singleRefresh";
        return;
    }
    QVulkanWindow::exposeEvent(e);
    //qDebug() << "-- Exposed-- ";
    //qDebug() << "-- Type -- :" << e->type();*/
    //qDebug() << " -- Valid -- " << this->isValid();
    QVulkanWindow::exposeEvent(e);
}

void STLVulkanWindow::paintEvent(QPaintEvent *p){
   /* if( this->m_singleRefresh ){
        qDebug() << "-- Paint --";
        qDebug() << "-- Type -- :singleRefresh ";
        return;
    }
    QVulkanWindow::paintEvent(p);
    qDebug() << "-- Paint --";
    qDebug() << "-- Type -- :" << p->type(); */
    QVulkanWindow::paintEvent(p);
}

void STLVulkanWindow::showEvent(QShowEvent *s){
    QVulkanWindow::showEvent(s);
  /*  qDebug() << "-- Show --";
    qDebug() << "-- Type -- :" << s->type(); */
    //requestUpdate();
}

void STLFileData::reset(){
    bindData=false;
    triangleCount=0;
    vertexCount=0;
    minX=1000.0f;
    minY=1000.0f;
    minZ=1000.0f;
    maxX=-1000.0f;
    maxY=-1000.0f;
    maxZ=-1000.0f;
    width=0.0f; // X Width
    length=0.0f; // Y Length
    height=0.0f; // Z Height
    vertices.clear();
    return;
}
