from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class PlayerLabConan(ConanFile):
    name = "playerlab"
    version = "0.1.0"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "ffmpeg/7.1.1",
        "spdlog/1.14.1",
        "fmt/10.2.1",
        "nlohmann_json/3.11.3",
        "stb/cci.20240531",
        "yaml-cpp/0.8.0",
    )

    default_options = {
        "ffmpeg/*:shared": True,
        "spdlog/*:header_only": True,
        "spdlog/*:use_std_fmt": True,
    }

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.generate()
