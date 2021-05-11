#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#if (defined(__GNUC__) && (__GNUC__ < 8)) || (defined(__clang__) && (__clang_major__ < 7)) 
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

// Visual Studio 2015 and GCC 7 work-around ...
// std::filesystem was incorporated into C++-17 (which is obviously after VS
// 2015 was released). However, Microsoft implemented the draft standard in
// the std::exerimental namespace. To avoid nasty ripple effects when the
// compiler is updated, make it look like the standard here
#if (defined(_MSC_VER) && (_MSC_VER < 1900)) || (defined(__GNUC__) && (__GNUC__ < 8))
namespace std
{
    namespace filesystem = experimental::filesystem::v1;
}
#elif (defined(__clang__))
namespace std
{
    namespace filesystem = experimental::filesystem;
}
#endif

namespace fs = std::filesystem;

struct input_t
{
    std::string path;
    std::string filename;
    std::string header_name;
    std::string variable_name;
};

std::vector<std::string> get_spvs(const std::string& spv_folder)
{
    const fs::path ext = ".spv";
    // return the filenames of all files that have the specified extension
    // in the specified directory and all subdirectories
    if (!fs::exists(spv_folder) || !fs::is_directory(spv_folder))
    {
        throw std::runtime_error("No models spv found\n");
    }

    std::vector<std::string> spv_paths;
    fs::recursive_directory_iterator it(spv_folder);
    fs::recursive_directory_iterator endit;

    while (it != endit)
    {
        if (fs::is_regular_file(*it) && it->path().extension() == ext)
        {
            spv_paths.push_back(it->path().string());
        }
        ++it;
    }

    return spv_paths;
}

std::vector<input_t> process_paths(std::vector<std::string> const& spv_paths)
{
    std::vector<input_t> inputs;
    for (auto const& spv_path : spv_paths)
    {
        input_t input;
        input.path = spv_path;
        fs::path p(spv_path.c_str());
        input.filename = p.filename().string();
        input.header_name = p.stem().string() + ".h";
        input.variable_name = input.filename;
        std::replace(input.variable_name.begin(), input.variable_name.end(), '.', '_');
        inputs.push_back(input);
    }
    return inputs;
}


int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cout << "No folder specified" << std::endl;
        std::cout << "Correct use:" << std::endl;
        std::cout << argv[0] << " [folder path to spv]" << " [headers path]" 
                  << " [headers path to include]"
                  << " [path to common header]"
                  << " [path to common mapping header]"  << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> spvs;
    try
    {
        spvs = get_spvs(argv[1]);
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<input_t> inputs = process_paths(spvs);

    std::string const& headers_out = argv[4];
    std::ofstream headers_write_out(headers_out, std::ios::out | std::ios::trunc);
    std::string const& mapping_headers_out = argv[5];
    std::ofstream mapping_headers_write_out(mapping_headers_out, std::ios::out | std::ios::trunc);
    if (mapping_headers_write_out)
    {
        mapping_headers_write_out << "#pragma once" << std::endl << "#include " << '"' << "compiled_spv.h"<< '"' << std::endl;
        mapping_headers_write_out << "#include <map>" << std::endl;
        mapping_headers_write_out << "#include <string>" << std::endl;
        mapping_headers_write_out << "namespace rt::vulkan::shaders {" << std::endl;
        mapping_headers_write_out << "std::map<std::string, std::pair<std::uint32_t*, size_t>> const& GetStringToCode() {" << std::endl;
        mapping_headers_write_out << std::endl << "    static std::map<std::string, std::pair<std::uint32_t*, size_t>> string_to_code = {" << std::endl;
    }
    else
    {
        std::cout << "Unable to open file to map headers" << std::endl;
        return EXIT_FAILURE;
    }

    for (auto const& input : inputs)
    {
        std::string const& fin = input.path;
        std::string const& name = input.variable_name;
        std::string const& fout = std::string(argv[2]) + "/" + input.header_name;
        std::string const& header_name = std::string(argv[3]) + "/" + input.header_name;

        std::ifstream in(fin, std::ios::in | std::ios::binary);
        std::vector<std::uint32_t> code;

        if (in)
        {
            std::streamoff beg = in.tellg();
            in.seekg(0, std::ios::end);
            std::streamoff file_size = in.tellg() - beg;
            in.seekg(0, std::ios::beg);
            code.resize(static_cast<unsigned>((file_size + sizeof(std::uint32_t) - 1) / sizeof(std::uint32_t)));
            in.read((char*)&code[0], file_size);
        }
        else
        {
            std::cout << "Unable to open file to read: " << fin.c_str() << std::endl;
            continue;
        }

        std::ofstream out(fout, std::ios::out | std::ios::trunc);
        if (out)
        {
            out << "#pragma once" << std::endl;
            out << "#include <cstdint>" << std::endl;
            out << "namespace  rt::vulkan::shaders {" << std::endl;
            out << std::endl;
            out << "std::uint32_t " << name << "[] = {" << std::endl;
            size_t line_breaker = 0;
            for (auto const& code_entry : code)
            {
                out << code_entry << ", ";
                line_breaker++;
                if (line_breaker % 10 == 0)
                {
                    out << std::endl;
                }
            }
            out << "};" << std::endl;
            out << std::endl;
            out << "size_t " << name << "_size = " << code.size() << ";" << std::endl;
            out << "}" << std::endl;
        }
        else
        {
            std::cout << "Unable to open file to write" << std::endl;
            return EXIT_FAILURE;
        }

        
        if (headers_write_out)
        {
            headers_write_out << "#include " << '"' << header_name << '"'<< std::endl;
        }
        else
        {
            std::cout << "Unable to open file to write headers" << std::endl;
            return EXIT_FAILURE;
        }

        if (mapping_headers_write_out)
        {
            mapping_headers_write_out << "{ " << '"' << input.filename << '"' << ", {"<< input.variable_name << ", " << input.variable_name <<"_size } },"<< std::endl;
        }
        else
        {
            std::cout << "Unable to open file to map headers" << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (mapping_headers_write_out)
    {
        mapping_headers_write_out << "};" << std::endl;
        mapping_headers_write_out << "return string_to_code;" << std::endl;
        mapping_headers_write_out << "} }" << std::endl;
    }
    else
    {
        std::cout << "Unable to open file to map headers" << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
