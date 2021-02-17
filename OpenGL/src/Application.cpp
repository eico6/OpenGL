#include <GL/glew.h> 

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Application.h"
#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"

Application::Application()
    : m_Shader{0}, m_Increment{0.05f}, m_UniformLoc{0}, va{nullptr}, vb{nullptr}, ib{nullptr}
{

}

Application::~Application()
{
    delete vb;
    delete ib;
}

Application::ShaderProgramSource Application::ParseShader(const std::string& filepath)
{
    ShaderProgramSource::ShaderType type = ShaderProgramSource::ShaderType::NONE;
    std::ifstream stream(filepath); // The content of the file we read
    std::string line;               // Will contain what word we currently are reading
    std::stringstream ss[2];        // Will contain the "extracted" source code
    while (getline(stream, line))   // While there is more to read, and then fill up 'line' with current word
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
                type = ShaderProgramSource::ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderProgramSource::ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }

    // Will return an object of two components. 
    // The default operator overload for '=' will assign these correctly in the ShaderProgramSource struct.
    return {ss[(int)ShaderProgramSource::ShaderType::VERTEX].str(), ss[(int)ShaderProgramSource::ShaderType::FRAGMENT].str()};
}

unsigned int Application::CompileShader(unsigned int type, const std::string& source)
{
    // Creates an empty shader object.
    unsigned int id = glCreateShader(type);

    // source.c_str() = &source[0]
    const char* src = source.c_str();

    // Sets the source code for the 'id' shader (will replace all previous source code).
    // @shader - specifies the shader
    // @count  - specifies the number of elements in the input string
    // @string - a pointer to the actual input string
    // @length - specifies an array of string lengths (?)
    CALL(glShaderSource(id, 1, &src, nullptr));

    // Compiles a shader object
    CALL(glCompileShader(id));

    // Checks if shader was created succesfully.
    int result;
    CALL(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if (result == GL_FALSE) // 'GL_FALSE' = 0
    {
        int length;
        CALL(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        // 'alloca' = Most compilers come with this function. Dynamically allocates memory on the stack.
        char* message = (char*)_malloca(length * sizeof(char));
        CALL(glGetShaderInfoLog(id, length, &length, message));
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                                         << " shader!" << std::endl;
        std::cout << message << std::endl;
        CALL(glDeleteShader(id));
        return 0;
    }

    return id;
}

unsigned int Application::CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    // Creates an empty program object
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    // Attaches a shader object to a program object. I.e. it puts the puzzle pieces together.
    CALL(glAttachShader(program, vs));
    CALL(glAttachShader(program, fs));

    // Will link the program. Any shaders that are attached will take part in
    // this linking proccess. Will create executables. Can be thought of as compiling the program.
    CALL(glLinkProgram(program));

    // Checks to see whether the executables contained in 'program' can actually execute.
    // The information generated by the validation process will be stored in the program's information log.
    CALL(glValidateProgram(program));

    // We are done using the initial shaders (are now "compiled" into the program), so we can delete them. 
    // This will not delete the source code though, so it can be used later for debugging purposes.
    // 'glDetachShader()' is used to delete the actual source code.
    CALL(glDeleteShader(vs));
    CALL(glDeleteShader(fs));

    return program;
}

void Application::init()
{
    // Vertex buffer
    float positions[] = {
        -0.5f, -0.5f, // 0
         0.5f, -0.5f, // 1
         0.5f,  0.5f, // 2
        -0.5f,  0.5f, // 3
    };

    // Index buffer
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // --- VERTEX BUFFER ---
    vb = new VertexBuffer(positions, 4 * 2 * sizeof(float));
    
    VertexBufferLayout layout;
    layout.Push<float>(2);
    va->AddBuffer(*vb, layout); // dereferenced!!!!

    // --- INDEX BUFFER ---
    ib = new IndexBuffer(indices, 6);

    // --- SHADERS ---
    // Fill up 'source' with the source code of the shaders.
    // Written in the OpenGL Shading Language (GLSL). The source code explained:
    // @layout(location = 0) - sets the 'position' variable equal to the attribute you have specified
    //                         at index 0 in your vertex buffer (in this case, the position attribute).
    //                         'position' has to be of type vec4, so it casts it implicitly.
    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
    m_Shader = CreateShader(source.VertexSource, source.FragmentSource);
    CALL(glUseProgram(m_Shader));

    // --- UNIFORMS ---
    // After binding the program, I send the following values to the shader using a uniform.
    CALL(m_UniformLoc = glGetUniformLocation(m_Shader, "u_Color"));
    ASSERT(m_UniformLoc != -1); // Just check that it found it.
    CALL(glUniform4f(m_UniformLoc, 0.0f, 0.3f, 0.8f, 1.0f)); // ...4f = 4 floats = vec4

    // Unbind everything
    va->Unbind();
    CALL(glUseProgram(0));
    CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Application::render() 
{
    static float r{0.0f};

    CALL(glUseProgram(m_Shader));
    CALL(glUniform4f(m_UniformLoc, r, 0.3f, 0.8f, 1.0f));

    va->Bind();
    ib->Bind();

    // glDrawArrays()   uses the buffer that is bound at slot 'GL_ARRAY_BUFFER'         = Vertex buffer
    // glDrawElements() uses the buffer that is bound at slot 'GL_ELEMENT_ARRAY_BUFFER' = Index buffer
    CALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

    // Color cycle calculation
    if (r > 1.0f)
        m_Increment = -0.01f;
    if (r < 0.0f)
        m_Increment =  0.01f;
    r += m_Increment;
}

