from conan import ConanFile
from conan.tools.cmake import cmake_layout

class QuillScribeConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    
    def requirements(self):
        # Google Test for unit testing
        self.requires("gtest/1.14.0")
        
        # JSON library for configuration and API responses
        self.requires("nlohmann_json/3.11.2")
        
        # HTTP client for Gemini API
        self.requires("libcurl/8.4.0")
        
        # Audio processing dependencies
        self.requires("openssl/3.2.0")  # For secure API calls
    
    def layout(self):
        cmake_layout(self)

