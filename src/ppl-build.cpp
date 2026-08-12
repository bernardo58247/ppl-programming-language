#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

void mostrar_uso() {
    std::cout
        << "PPL Build System\n\n"
        << "uso:\n"
        << "  ppl-build <arquivo.ppls>\n"
        << "  ppl-build <arquivo.ppls> -o <saida>\n"
        << "  ppl-build <arquivo.ppls> --target linux\n"
        << "  ppl-build <arquivo.ppls> --target windows\n"
        << "\n"
        << "opções:\n"
        << "  -o <arquivo>       nome do executável de saída\n"
        << "  --target <alvo>    alvo da compilação\n"
        << "  -h, --help         mostra esta ajuda\n";
}

std::string gerar_nome_temporario() {
    auto agora =
        std::chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count();

    return "ppl-build-" + std::to_string(agora) + ".cpp";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        mostrar_uso();
        return 1;
    }

    std::string arquivo_ppls;
    std::string saida;
    std::string target = "linux";

    for (int i = 1; i < argc; ++i) {
        std::string argumento = argv[i];

        if (argumento == "-h" || argumento == "--help") {
            mostrar_uso();
            return 0;
        }

        if (argumento == "-o") {
            if (i + 1 >= argc) {
                std::cerr
                    << "erro: a opção -o precisa de um nome de arquivo.\n";
                return 1;
            }

            saida = argv[++i];
            continue;
        }

        if (argumento == "--target") {
            if (i + 1 >= argc) {
                std::cerr
                    << "erro: --target precisa de um alvo.\n";
                return 1;
            }

            target = argv[++i];
            continue;
        }

        if (arquivo_ppls.empty()) {
            arquivo_ppls = argumento;
            continue;
        }

        std::cerr
            << "erro: argumento desconhecido: "
            << argumento
            << "\n";

        return 1;
    }

    if (arquivo_ppls.empty()) {
        std::cerr
            << "erro: nenhum arquivo .ppls foi informado.\n";
        return 1;
    }

    fs::path caminho_ppls(arquivo_ppls);

    if (!fs::exists(caminho_ppls)) {
        std::cerr
            << "erro: arquivo '"
            << arquivo_ppls
            << "' não encontrado.\n";
        return 1;
    }

    if (!fs::is_regular_file(caminho_ppls)) {
        std::cerr
            << "erro: '"
            << arquivo_ppls
            << "' não é um arquivo normal.\n";
        return 1;
    }

    if (caminho_ppls.extension() != ".ppls") {
        std::cerr
            << "erro: '"
            << arquivo_ppls
            << "' não possui a extensão .ppls.\n";
        return 1;
    }

    /*
     * Se o usuário não especificou -o,
     * usamos o nome do arquivo .ppls.
     *
     * exemplo:
     *
     *   hello.ppls
     *
     * vira:
     *
     *   hello
     */

    if (saida.empty()) {
        saida = caminho_ppls.stem().string();

        if (target == "windows") {
            saida += ".exe";
        }
    }

    /*
     * Lê o código PPL.
     */

    std::ifstream arquivo(caminho_ppls);

    if (!arquivo.is_open()) {
        std::cerr
            << "erro: não foi possível abrir '"
            << arquivo_ppls
            << "'.\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << arquivo.rdbuf();

    arquivo.close();

    std::string codigo_ppl = buffer.str();

    /*
     * Descobre o diretório temporário do sistema.
     *
     * Linux / Termux:
     *
     *   normalmente /tmp
     *
     * Windows:
     *
     *   normalmente %TEMP%
     */

    fs::path diretorio_temp;

    try {
        diretorio_temp = fs::temp_directory_path();
    }
    catch (const fs::filesystem_error& erro) {
        std::cerr
            << "erro: não foi possível encontrar o diretório temporário.\n"
            << erro.what()
            << "\n";

        return 1;
    }

    /*
     * Cria o caminho do .cpp temporário.
     */

    fs::path arquivo_temporario =
        diretorio_temp / gerar_nome_temporario();

    std::cout
        << "arquivo temporário: "
        << arquivo_temporario
        << "\n";

    /*
     * Cria o C++ temporário.
     *
     * ATENÇÃO:
     *
     * Esta primeira versão pressupõe que o código
     * da classe InterpretadorPPL esteja disponível
     * para o programa gerado.
     *
     * A arquitetura final deve separar o interpretador
     * em um arquivo reutilizável.
     */

    std::ofstream cpp(arquivo_temporario);

    if (!cpp.is_open()) {
        std::cerr
            << "erro: não foi possível criar o arquivo temporário:\n"
            << arquivo_temporario
            << "\n";

        return 1;
    }

    cpp << R"CPP(
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <cstdlib>
#include <stdexcept>

/*
 * Código do interpretador PPL
 * será disponibilizado aqui na versão
 * final do ppl-build.
 */

)CPP";

    /*
     * Aqui entrará o código do InterpretadorPPL.
     *
     * Por enquanto deixamos um erro explícito
     * para não gerar um executável falso.
     */

    cpp << R"CPP(
int main() {
    std::cerr
        << "erro: o interpretador PPL ainda não foi "
        << "incorporado ao executável.\n";

    return 1;
}
)CPP";

    cpp.close();

    /*
     * Escolhe o compilador.
     */

    std::string compilador;

    if (target == "linux") {
        compilador = "g++";
    }
    else if (target == "windows") {
        compilador = "x86_64-w64-mingw32-g++";
    }
    else {
        std::cerr
            << "erro: alvo desconhecido: "
            << target
            << "\n";

        fs::remove(arquivo_temporario);

        return 1;
    }

    /*
     * Monta o comando de compilação.
     */

    std::string comando =
        compilador +
        " -std=c++17 \"" +
        arquivo_temporario.string() +
        "\" -o \"" +
        saida +
        "\"";

    std::cout
        << "compilando...\n";

    int resultado = std::system(comando.c_str());

    /*
     * O .cpp temporário não é mais necessário.
     */

    std::error_code erro_remocao;

    fs::remove(
        arquivo_temporario,
        erro_remocao
    );

    if (erro_remocao) {
        std::cerr
            << "aviso: não foi possível remover o arquivo temporário:\n"
            << arquivo_temporario
            << "\n";
    }

    /*
     * Verifica o resultado da compilação.
     */

    if (resultado != 0) {
        std::cerr
            << "erro: falha ao compilar o programa PPL.\n";

        return 1;
    }

    std::cout
        << "build concluído!\n"
        << "executável: "
        << saida
        << "\n";

    return 0;
}