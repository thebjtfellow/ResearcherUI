#ifndef STLVULKANWINDOW_H
#define STLVULKANWINDOW_H

#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
#include <QVulkanDeviceFunctions>
#include <QVulkanInstance>
#include <QWidget>
#include "shader.h"
#include "/usr/include/glm/glm.hpp"
#include "/usr/include/glm/gtc/constants.hpp"
#include "stlcamera.h"

struct p3f{
    float x, y, z;
    bool operator==(const p3f& oth){return x==oth.x && y==oth.y && z==oth.z;}
    p3f& operator=(const p3f& oth){ x=oth.x; y=oth.y; z=oth.z; return *this;}
};

struct vertex3f{
    p3f position;
    p3f normal;
    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    vertex3f& operator=(const vertex3f& oth){position = oth.position; normal=oth.normal; return *this;}
};

struct TransformComponent {
    glm::vec3 translation{};
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
    glm::vec3 rotation{};

    glm::mat4 mat4() {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        return glm::mat4{
            {
                scale.x * (c1 * c3 + s1 * s2 * s3),
                scale.x * (c2 * s3),
                scale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f,
            },
            {
                scale.y * (c3 * s1 * s2 - c1 * s3),
                scale.y * (c2 * c3),
                scale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f,
            },
            {
                scale.z * (c2 * s1),
                scale.z * (-s2),
                scale.z * (c1 * c2),
                0.0f,
            },
            {translation.x, translation.y, translation.z, 1.0f} };
    }
};

struct PushConstantData{
    glm::mat4 transform{1.0f};
    glm::mat4 modelMatrix{1.0f};
};

class STLVulkanWindow;

struct PipelineConfigInfo { // Convenience structure, no copies allowed
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo() = default;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    std::vector<VkDynamicState> dynamicStateEnables{};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    VkPipelineVertexInputStateCreateInfo vertexInfo{};

    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;


};

struct STLFileData {
    bool bindData=false;
    uint32_t triangleCount=0;
    uint32_t vertexCount=0;
    float minX=1000.0f;
    float minY=1000.0f;
    float minZ=1000.0f;
    float maxX=-1000.0f;
    float maxY=-1000.0f;
    float maxZ=-1000.0f;
    float width=0.0f; // X Width
    float length=0.0f; // Y Length
    float height=0.0f; // Z Height

    TransformComponent transform;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    std::vector<vertex3f> vertices;
    void reset();
};

class STLVulkanWindowRenderer : public QVulkanWindowRenderer
{
public:
    STLVulkanWindowRenderer(STLVulkanWindow * viewWindow);
    ~STLVulkanWindowRenderer();

    void preInitResources() override;
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;
    void destroySTLData(STLFileData &stl);

private:

    STLVulkanWindow* view;
    VkDevice dev;
    QVulkanDeviceFunctions * devFuncs=nullptr;
    QVulkanFunctions *physdevFuncs=nullptr;
    QVulkanInstance * vulkanInst=nullptr;


    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    uint32_t m_vertexCount=0;
    QMatrix4x4 m_proj;

    void makeSTLBuffer(uint32_t index);
    void defaultPipelineConfigInfo();
    Shader fragShader{};
    Shader vertShader{};

    PipelineConfigInfo m_configInfo{};
    VkGraphicsPipelineCreateInfo m_pipelineInfo{};

    VkPipeline m_pipeline;
    VkVertexInputBindingDescription bindingDescription{};
    VkVertexInputAttributeDescription attributeDescriptions[2]{};
    void createBuffer(    VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

class STLVulkanWindow : public QVulkanWindow
{
    Q_OBJECT
public:
    STLVulkanWindow();
    void predelete();
    ~STLVulkanWindow();
    void setValid() {valid = true;}
    bool loadSTLFile(QString filePath);
    bool unloadAllFiles();
    uint32_t getTriangleCount(){ return m_triangleCount; }
    STLCamera m_camera;
    TransformComponent m_STLTransform;
    TransformComponent m_cameraTransform;
    float ortho_left = -32.512f;
    float ortho_right = 32.512f;
    float ortho_top = -32.512f;
    float ortho_bottom = 32.512f;

    QVulkanWindowRenderer *createRenderer() override;

    std::vector<STLFileData> stlFiles;

    uint32_t m_triangleCount=0;
    float m_sliceHeight = -0.1f;
    float m_stlMinX=1000.0f;
    float m_stlMinY=1000.0f;
    float m_stlMinZ=1000.0f;
    float m_stlMaxX=-1000.0f;
    float m_stlMaxY=-1000.0f;
    float m_stlMaxZ=-1000.0f;
    bool m_singleRefresh=false;
    bool m_loading = false;
    void exposeEvent(QExposeEvent *e) override;
    void paintEvent(QPaintEvent *p) override;
    void showEvent(QShowEvent *s) override;
private:
    QWidget * holder=nullptr;
    bool valid = false;
    //unsigned char * stldata=nullptr;

    STLVulkanWindowRenderer * m_renderer = nullptr;


};

#endif // STLVULKANWINDOW_H
