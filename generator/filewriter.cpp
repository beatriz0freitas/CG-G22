#include "filewriter.h"
#include <fstream>
#include <iostream>

bool saveToFile(const Model& model, const std::string& filename) {
    std::ofstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Erro: nao foi possivel criar '" << filename << "'\n";
        return false;
    }

    // Primeira linha: número total de vértices
    f << model.size() << "\n";

    // Uma linha por vértice com todos os campos.
    for (const auto& v : model)
        f << v.x  << " " << v.y  << " " << v.z  << " "
          << v.nx << " " << v.ny << " " << v.nz << " "
          << v.u  << " " << v.v  << "\n";

    std::cout << "Guardado: '" << filename << "' ("
              << model.size() / 3 << " triangulos)\n";
    return true;
}