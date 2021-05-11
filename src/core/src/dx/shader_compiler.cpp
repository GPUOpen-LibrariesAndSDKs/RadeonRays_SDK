#include "shader_compiler.h"

#include <atomic>
#include <stdexcept>

//#define DUMP_BLOBS
#ifdef DUMP_BLOBS
#include <fstream>
#endif

#ifdef RR_EMBEDDED_KERNELS
#include "src/dx/mapped_kernels.h"
#endif

#include "dx/common.h"

namespace
{
std::string GenerateKey(std::string const& filename, std::string const& entry, std::vector<std::string> const& defines)
{
    std::string key = filename + ", " + entry + ", ";
    for (const auto& define : defines)
    {
        key += define;
    }
    return key;
}
}  // namespace

namespace rt::dx
{
ShaderCompiler& ShaderCompiler::instance()
{
    static ShaderCompiler inst;
    return inst;
}

ShaderCompiler::ShaderCompiler()
{
    hdll_ = LoadLibraryA("dxcompiler.dll");

    if (!hdll_)
    {
        throw std::runtime_error("Cannot load dxcompiler.dll");
    }

    auto create_instance_fn = (DxcCreateInstanceProc)GetProcAddress(hdll_, "DxcCreateInstance");

    auto result = create_instance_fn(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler_));
    if (FAILED(result))
    {
        throw std::runtime_error("Cannot create DXC instance");
    }

    result = create_instance_fn(CLSID_DxcLibrary, IID_PPV_ARGS(&library_));
    if (FAILED(result))
    {
        throw std::runtime_error("Cannot create DXC library instance");
    }

    result = library_->CreateIncludeHandler(&include_handler_);
    if (FAILED(result))
    {
        throw std::runtime_error("Cannot create DXC include handler");
    }
}

ShaderCompiler::~ShaderCompiler()
{
    library_ = nullptr;
    compiler_ = nullptr;
    FreeLibrary(hdll_);
}

Shader ShaderCompiler::CompileFromFile(const std::string& file_name,
                                       const std::string& shader_model,
                                       const std::string& entry_point)
{
    std::vector<std::string> defs;
    return CompileFromFile(file_name, shader_model, entry_point, defs);
}

Shader ShaderCompiler::CompileFromFile(const std::string&              file_name,
                                       const std::string&              shader_model,
                                       const std::string&              entry_point,
                                       const std::vector<std::string>& defines)
{
#ifdef RR_EMBEDDED_KERNELS
    std::string key = GenerateKey(file_name, entry_point, defines);
    try
    {
        auto const& code = shaders::GetShaderCode(key);
        // return embed kernel instead of compiling
        return Shader{nullptr, code.data(), code.size() * sizeof(uint32_t)};

    } catch (...)
    {
        // do nothing since there are no embed kernel for requested file
    }

#endif

    ComPtr<IDxcBlobEncoding> source = nullptr;

    auto result = library_->CreateBlobFromFile(StringToWideString(file_name).c_str(), nullptr, &source);

    if (FAILED(result))
    {
        throw std::runtime_error("Shader source not found: " + file_name);
    }

    std::vector<std::wstring> wdefines;
    wdefines.reserve(defines.size());

    std::vector<DxcDefine> temp;
    for (auto& d : defines)
    {
        wdefines.push_back(StringToWideString(d));
        DxcDefine dxcdef = {};
        dxcdef.Name      = wdefines.back().c_str();
        dxcdef.Value     = L"1";
        temp.push_back(dxcdef);
    }

    ComPtr<IDxcOperationResult> compiler_output = nullptr;

    result = compiler_->Compile(source.Get(),
                                StringToWideString(file_name).c_str(),
                                StringToWideString(entry_point).c_str(),
                                StringToWideString(shader_model).c_str(),
                                nullptr,
                                0u,
                                temp.size() ? temp.data() : nullptr,
                                (UINT32)temp.size(),
                                include_handler_,
                                &compiler_output);

    if (FAILED(result))
    {
        throw std::runtime_error("Shader compiler failure: " + file_name);
    }

    if (FAILED(compiler_output->GetStatus(&result)) || FAILED(result))
    {
        ComPtr<IDxcBlobEncoding> error;
        compiler_output->GetErrorBuffer(&error);
        std::string error_string(static_cast<char const*>(error->GetBufferPointer()));
        throw std::runtime_error(error_string);
    }

    IDxcBlob* blob = nullptr;
    compiler_output->GetResult(&blob);

#ifdef DUMP_BLOBS
    std::ofstream ofs("mapped_kernels.h", std::ios_base::app);
    std::string key = GenerateKey(file_name, entry_point, defines);
    ofs << "{ "
        << "\"" << key << "\", " << '{' << std::endl;
    auto* code = (uint32_t*)blob->GetBufferPointer();
    if (blob->GetBufferSize() % sizeof(uint32_t))
    {
        throw std::runtime_error("Incorrect buffer size");
    }
    auto size = blob->GetBufferSize() / sizeof(uint32_t);
    for (size_t i = 0; i < size; i++)
    {
        ofs << code[i];
        if (i < size - 1)
        {
            ofs << ", ";
        }
    }
    ofs << "}}," << std::endl;
#endif

    return Shader{blob};
}

Shader ShaderCompiler::CompileFromString(const std::string& source_string,
                                         const std::string& shader_model,
                                         const std::string& entry_point)
{
    std::vector<std::string> defs;
    return CompileFromString(source_string, shader_model, entry_point, defs);
}

Shader ShaderCompiler::CompileFromString(const std::string&              source_string,
                                         const std::string&              shader_model,
                                         const std::string&              entry_point,
                                         const std::vector<std::string>& defines)
{
    ComPtr<IDxcBlobEncoding> source = nullptr;

    auto result =
        library_->CreateBlobWithEncodingFromPinned(source_string.c_str(), (UINT)source_string.size(), 0, &source);

    if (FAILED(result))
    {
        throw std::runtime_error("Cannot create blob from memory");
    }

    std::vector<std::wstring> wdefines;
    wdefines.reserve(defines.size());

    std::vector<DxcDefine> temp;
    for (auto& d : defines)
    {
        wdefines.push_back(StringToWideString(d));
        DxcDefine dxcdef = {};
        dxcdef.Name      = wdefines.back().c_str();
        dxcdef.Value     = L"1";
        temp.push_back(dxcdef);
    }

    ComPtr<IDxcOperationResult> compiler_output = nullptr;

    result = compiler_->Compile(source.Get(),
                                L"",
                                StringToWideString(entry_point).c_str(),
                                StringToWideString(shader_model).c_str(),
                                nullptr,
                                0u,
                                temp.size() ? temp.data() : nullptr,
                                (UINT32)temp.size(),
                                include_handler_,
                                &compiler_output);

    if (FAILED(result))
    {
        throw std::runtime_error("Shader compiler failure");
    }

    if (FAILED(compiler_output->GetStatus(&result)) || FAILED(result))
    {
        ComPtr<IDxcBlobEncoding> error;
        compiler_output->GetErrorBuffer(&error);
        std::string error_string(static_cast<char const*>(error->GetBufferPointer()));
        throw std::runtime_error(error_string);
    }

    IDxcBlob* blob = nullptr;
    compiler_output->GetResult(&blob);

    return Shader{blob};
}

void CompileShader(const char*  path,
                   const char*  entry_point,
                   const char** defines,
                   uint32_t     num_defines,
                   void*        data,
                   size_t*      data_size)
{
    std::vector<std::string> temp_defs;
    for (auto i = 0u; i < num_defines; ++i)
    {
        temp_defs.push_back(defines[i]);
    }

    auto shader = ShaderCompiler::instance().CompileFromFile(path, "cs_6_2", entry_point, temp_defs);

    if (data)
    {
        std::memcpy(data, shader.dxc_blob->GetBufferPointer(), shader.dxc_blob->GetBufferSize());
    }

    if (data_size)
    {
        *data_size = shader.dxc_blob->GetBufferSize();
    }
}
}  // namespace rt::dx
