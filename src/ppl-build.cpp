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
        << "  ppl-build <arquivo.ppls> --windows\n"
        << "  ppl-build <arquivo.ppls> --windows -o <saida>\n"
        << "\n"
        << "opções:\n"
        << "  -o <arquivo>    define o nome do executável de saída\n"
        << "  --windows       compila para windows\n"
        << "  -h, --help      mostra esta ajuda\n";
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

    bool alvo_windows = false;

    /*
     * Processa os argumentos.
     */

    for (int i = 1; i < argc; ++i) {

        std::string argumento = argv[i];

        /*
         * ajuda
         */

        if (argumento == "-h" || argumento == "--help") {
            mostrar_uso();
            return 0;
        }

        /*
         * saída personalizada
         */

        if (argumento == "-o") {

            if (i + 1 >= argc) {
                std::cerr
                    << "erro: a opção -o precisa de um nome de arquivo.\n";

                return 1;
            }

            saida = argv[++i];

            continue;
        }

        /*
         * cross-compilation para windows
         */

        if (argumento == "--windows") {
            alvo_windows = true;
            continue;
        }

        /*
         * primeiro argumento sem opção = arquivo .ppls
         */

        if (arquivo_ppls.empty()) {
            arquivo_ppls = argumento;
            continue;
        }

        /*
         * argumento desconhecido
         */

        std::cerr
            << "erro: argumento desconhecido: "
            << argumento
            << "\n";

        return 1;
    }

    /*
     * Verifica se foi fornecido um arquivo.
     */

    if (arquivo_ppls.empty()) {
        std::cerr
            << "erro: nenhum arquivo .ppls foi informado.\n";

        return 1;
    }

    fs::path caminho_ppls(arquivo_ppls);

    /*
     * Verifica se o arquivo existe.
     */

    if (!fs::exists(caminho_ppls)) {
        std::cerr
            << "erro: arquivo '"
            << arquivo_ppls
            << "' não encontrado.\n";

        return 1;
    }

    /*
     * Verifica se é realmente um arquivo.
     */

    if (!fs::is_regular_file(caminho_ppls)) {
        std::cerr
            << "erro: '"
            << arquivo_ppls
            << "' não é um arquivo normal.\n";

        return 1;
    }

    /*
     * Verifica a extensão.
     */

    if (caminho_ppls.extension() != ".ppls") {
        std::cerr
            << "erro: o arquivo precisa possuir a extensão .ppls.\n";

        return 1;
    }

    /*
     * Detecta o sistema operacional.
     *
     * Se --windows foi usado, o alvo será Windows
     * independentemente do sistema atual.
     */

    std::string sistema;
    std::string compilador;

    if (alvo_windows) {

        sistema = "windows";

#if defined(_WIN32)

        /*
         * Estamos no próprio Windows.
         */

        compilador = "g++";

#elif defined(__linux__)

        /*
         * Linux/Termux fazendo cross-compilation.
         */

        compilador = "x86_64-w64-mingw32-g++";

#else

        std::cerr
            << "erro: não é possível fazer cross-compilation "
            << "para windows neste sistema.\n";

        return 1;

#endif

    } else {

        /*
         * Detecção automática do sistema.
         */

#if defined(_WIN32)

        sistema = "windows";
        compilador = "g++";

#elif defined(__linux__)

        sistema = "linux";
        compilador = "g++";

#else

        std::cerr
            << "erro: sistema operacional não suportado.\n";

        return 1;

#endif

    }

    /*
     * Define o nome da saída.
     */

    if (saida.empty()) {

        saida = caminho_ppls.stem().string();

        if (sistema == "windows") {
            saida += ".exe";
        }
    }

    /*
     * Lê o arquivo PPL.
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
     * Obtém o diretório temporário do sistema.
     *
     * Linux/Termux:
     *     normalmente /tmp
     *
     * Windows:
     *     normalmente %TEMP%
     */

    fs::path diretorio_temp;

    try {

        diretorio_temp = fs::temp_directory_path();

    } catch (const fs::filesystem_error& erro) {

        std::cerr
            << "erro: não foi possível localizar "
            << "o diretório temporário.\n"
            << erro.what()
            << "\n";

        return 1;
    }

    /*
     * Cria o nome do .cpp temporário.
     */

    fs::path arquivo_temporario =
        diretorio_temp / gerar_nome_temporario();

    std::cout
        << "sistema alvo: "
        << sistema
        << "\n";

    std::cout
        << "arquivo temporário: "
        << arquivo_temporario
        << "\n";

    /*
     * Cria o C++ temporário.
     */

    std::ofstream cpp(arquivo_temporario);

    if (!cpp.is_open()) {

        std::cerr
            << "erro: não foi possível criar o arquivo temporário:\n"
            << arquivo_temporario
            << "\n";

        return 1;
    }

    /*
     * Código inicial do programa gerado.
     */

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
 * PPL BUILD
 *
 * O InterpretadorPPL será incorporado aqui
 * quando o interpretador for separado do main()
 * do ppl.cpp.
 */

)CPP";

    /*
     * Código PPL embutido.
     *
     * raw string permite preservar praticamente
     * qualquer conteúdo do arquivo .ppls.
     */

    cpp << R"CPP(
int main() {

    const char* codigo_ppl = R"PPL_CODE(
)CPP";

    cpp << codigo_ppl;

    cpp << R"CPP(
)PPL_CODE";

    /*
     * Temporariamente apenas informa que o
     * interpretador ainda precisa ser incorporado.
     */

    std::cerr
        << "erro: o interpretador PPL ainda "
        << "não foi incorporado ao ppl-build.\n";

    return 1;
}
)CPP";

    cpp.close();

    /*
     * Monta o comando do compilador.
     */

    std::string comando =
        compilador +
        " -std=c++17 "
        "-Wall "
        "-Wextra "
        "\"" +
        arquivo_temporario.string() +
        "\" "
        "-o "
        "\"" +
        saida +
        "\"";

    std::cout
        << "compilando...\n";

    /*
     * Executa o compilador.
     */

    int resultado =
        std::system(comando.c_str());

    /*
     * Remove o .cpp temporário.
     */

    std::error_code erro_remocao;

    fs::remove(
        arquivo_temporario,
        erro_remocao
    );

    if (erro_remocao) {

        std::cerr
            << "aviso: não foi possível remover "
            << "o arquivo temporário:\n"
            << arquivo_temporario
            << "\n";
    }

    /*
     * Verifica o resultado da compilação.
     */

    if (resultado != 0) {

        std::cerr
            << "erro: falha na compilação.\n";

        return 1;
    }

    /*
     * Build concluído.
     */

    std::cout
        << "build concluído!\n"
        << "executável: "
        << saida
        << "\n";

    return 0;
}