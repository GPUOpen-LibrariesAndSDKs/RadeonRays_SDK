/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#include "shader_manager.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>

static void LoadFileContents(std::string const& name, std::vector<char>& contents, bool binary = false)
{
    std::ifstream in(name, std::ios::in | (std::ios_base::openmode)(binary?std::ios::binary : 0));

    if (in)
    {
        contents.clear();

        std::streamoff beg = in.tellg();

        in.seekg(0, std::ios::end);

        std::streamoff fileSize = in.tellg() - beg;

        in.seekg(0, std::ios::beg);

        contents.resize(static_cast<unsigned>(fileSize));

        in.read(&contents[0], fileSize);
    }
    else
    {
        throw std::runtime_error("Cannot read the contents of a file");
    }
}

static GLuint CompileShader(std::vector<GLchar> const& shader_source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    
    GLint len = static_cast<GLint>(shader_source.size());
    GLchar const* source_array = &shader_source[0];
    
    glShaderSource(shader, 1, &source_array, &len);
    glCompileShader(shader);
    
    GLint result = GL_TRUE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    
    if(result == GL_FALSE)
    {
        std::vector<char> log;
        
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        
        log.resize(len);
        
        glGetShaderInfoLog(shader, len, &result, &log[0]);
        
        glDeleteShader(shader);
        
        throw std::runtime_error(std::string(log.begin(), log.end()));
        
        return 0;
    }
    
    return shader;
}


ShaderManager::ShaderManager()
{
}


ShaderManager::~ShaderManager()
{
    for (auto citer = shadercache_.cbegin(); citer != shadercache_.cend(); ++citer)
    {
        glDeleteProgram(citer->second);
    }
}

GLuint ShaderManager::CompileProgram(std::string const& name)
{
    std::string vsname = name + ".vsh";
    std::string fsname = name + ".fsh";
    
    // Need to wrap the shader program here to be exception-safe
    std::vector<GLchar> sourcecode;
    
    LoadFileContents(vsname, sourcecode);
    GLuint vertex_shader = CompileShader(sourcecode, GL_VERTEX_SHADER);
    
    /// This part is not exception safe:
    /// if the VS compilation succeeded
    /// and PS compilation fails then VS object will leak
    /// fix this by wrapping the shaders into a class
    LoadFileContents(fsname, sourcecode);
    GLuint frag_shader = CompileShader(sourcecode, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    
    glAttachShader(program, vertex_shader);
    glAttachShader(program, frag_shader);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);
    
    glLinkProgram(program);
    
    GLint result = GL_TRUE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    
    if(result == GL_FALSE)
    {
        GLint length = 0;
        std::vector<char> log;
        
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        
        log.resize(length);
        
        glGetProgramInfoLog(program, length, &result, &log[0]);
        
        glDeleteProgram(program);
        
        throw std::runtime_error(std::string(log.begin(), log.end()));
    }
    
    return program;
}

GLuint ShaderManager::GetProgram(std::string const& name)
{
    auto iter = shadercache_.find(name);
    
    if (iter != shadercache_.end())
    {
        return iter->second;
    }
    else
    {
        GLuint program = CompileProgram(name);
        shadercache_[name] = program;
        return program;
    }
}