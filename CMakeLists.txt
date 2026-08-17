# Dependencies
# External sources
file(GLOB EXTERNAL_SRC
    # stb
    external/external.cpp
)

bread_package(
    NAME
        "Basic"
    ASSETS
        "assets"
    INCLUDES
        "."
    SOURCES
        "Basic/2D/GeometryBatch.cpp"
        "Basic/2D/SpriteBatch.cpp"

        "Basic/Core/CommandPool.cpp"
        "Basic/Core/GPUResourceManager.cpp"
        "Basic/Core/RenderCore.cpp"
        "Basic/Core/RenderDevice.cpp"
        "Basic/Core/Renderer.cpp"
        "Basic/Core/RenderGraph.cpp"
        "Basic/Core/RenderTarget.cpp"
        "Basic/Core/Shader.cpp"
        "Basic/Core/SwapChain.cpp"

        "Basic/Memory/GenericGPUMemoryAllocator.cpp"

        "Basic/Resource/font.cpp"
        "Basic/Resource/image.cpp"
        "Basic/Resource/resource.cpp"
        "Basic/Resource/resource_manager.cpp"
        "Basic/Resource/sound.cpp"
        "Basic/Resource/texture.cpp"

        ${EXTERNAL_SRC}
)
