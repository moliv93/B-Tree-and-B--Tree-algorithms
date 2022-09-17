#include <iostream>
#include <fstream>
#include <vector>

uint createShader(std::string filePath, GLenum shaderType) {
    std::ifstream shaderFile(filePath, std::ios::binary | std::ios::ate);
    std::streamsize size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);

    std::vector<char> shaderCode(size);
    if (shaderFile.read(shaderCode.data(), size)) { }

    // char c;
    // std::string shaderCode;
    // while (shaderFile.get(c))
    // shaderCode.push_back(c);
    // const char* codes = shaderCode.c_str;

    // const char* data = shaderCode.data();
    int codeLength = shaderCode.size();
    char data[codeLength+1];
    for (int i=0; i<codeLength; i++) {
        data[i] = shaderCode[i];
    }
    data[codeLength] = '\0';

    const char* codeData = data;

    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &codeData, NULL);
    glCompileShader(shader);
    return shader;
}
