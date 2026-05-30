#include <iostream>
#include <string>
#include <vector>
#include "primitives/plane.h"
#include "primitives/box.h"
#include "primitives/sphere.h"
#include "primitives/cone.h"
#include "primitives/cylinder.h"
#include "primitives/torus.h"
#include "primitives/bezier.h"

static void usage() {
    std::cout
        << "Uso:\n"
        << "  generator plane    <length> <divisions> <output.3d>\n"
        << "  generator box      <size> <divisions> <output.3d>\n"
        << "  generator sphere   <radius> <slices> <stacks> <output.3d>\n"
        << "  generator cone     <radius> <height> <slices> <stacks> <output.3d>\n"
        << "  generator cylinder <radius> <height> <slices> <stacks> <output.3d>\n"
        << "  generator torus    <outerRadius> <innerRadius> <sides> <rings> <output.3d>\n"
        << "  generator bezier   <patch.patch> <tessellation> <output.3d>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { usage(); return 1; }

    std::vector<std::string> args(argv + 1, argv + argc);
    const std::string& cmd = args[0];

    try {
        if (cmd == "plane" && argc == 5) {
            auto m = generatePlane(std::stof(args[1]), std::stoi(args[2]));
            return saveToFile(m, args[3]) ? 0 : 1;

        } else if (cmd == "box" && argc == 5) {
            auto m = generateBox(std::stof(args[1]), std::stoi(args[2]));
            return saveToFile(m, args[3]) ? 0 : 1;

        } else if (cmd == "sphere" && argc == 6) {
            auto m = generateSphere(std::stof(args[1]), std::stoi(args[2]), std::stoi(args[3]));
            return saveToFile(m, args[4]) ? 0 : 1;

        } else if (cmd == "skybox" && argc == 6) {
            auto m = generateSphere(std::stof(args[1]), std::stoi(args[2]), std::stoi(args[3]), true);
            return saveToFile(m, args[4]) ? 0 : 1;

        } else if (cmd == "cone" && argc == 7) {
            auto m = generateCone(std::stof(args[1]), std::stof(args[2]),
                                  std::stoi(args[3]), std::stoi(args[4]));
            return saveToFile(m, args[5]) ? 0 : 1;

        } else if (cmd == "cylinder" && argc == 7) {
            auto m = generateCylinder(std::stof(args[1]), std::stof(args[2]),
                                      std::stoi(args[3]), std::stoi(args[4]));
            return saveToFile(m, args[5]) ? 0 : 1;

        } else if (cmd == "torus" && argc == 7) {
            auto m = generateTorus(std::stof(args[1]), std::stof(args[2]),
                                   std::stoi(args[3]), std::stoi(args[4]));
            return saveToFile(m, args[5]) ? 0 : 1;

        } else if (cmd == "bezier" && argc == 5) {
            auto m = generateBezier(args[1], std::stoi(args[2]));
            return saveToFile(m, args[3]) ? 0 : 1;
 
        } else {
            std::cerr << "Comando ou numero de argumentos invalido.\n";
            usage(); return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
}