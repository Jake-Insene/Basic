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

        "Basic/Resource/Font.cpp"
        "Basic/Resource/Image.cpp"
        "Basic/Resource/Resource.cpp"
        "Basic/Resource/ResourceManager.cpp"
        "Basic/Resource/Sound.cpp"
        "Basic/Resource/Texture.cpp"

        ${EXTERNAL_SRC}
)
