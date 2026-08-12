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
        << "  ppl-build <arquivo.ppls> --linux\n"
        << "  ppl-build <arquivo.ppls> --windows -o <saida>\n"
        << "  ppl-build <arquivo.ppls> --linux -o <saida>\n"
        << "\n"
        << "opções:\n"
        << "  -o <arquivo>    define o nome do executável de saída\n"
        << "  --windows       compila para windows\n"
        << "  --linux         compila para linux\n"
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
    bool alvo_linux = false;

    /*
     * Processa os argumentos.
     */

    for (int i = 1; i < argc; ++i) {

        std::string argumento = argv[i];

        /*
         * Ajuda.
         */

        if (argumento == "-h" || argumento == "--help") {
            mostrar_uso();
            return 0;
        }

        /*
         * Nome da saída.
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
         * Forçar Windows.
         */

        if (argumento == "--windows") {

            if (alvo_linux) {
                std::cerr
                    << "erro: --windows e --linux não podem "
                    << "ser usados ao mesmo tempo.\n";

                return 1;
            }

            alvo_windows = true;

            continue;
        }

        /*
         * Forçar Linux.
         */

        if (argumento == "--linux") {

            if (alvo_windows) {
                std::cerr
                    << "erro: --windows e --linux não podem "
                    << "ser usados ao mesmo tempo.\n";

                return 1;
            }

            alvo_linux = true;

            continue;
        }

        /*
         * Primeiro argumento sem opção = arquivo .ppls.
         */

        if (arquivo_ppls.empty()) {
            arquivo_ppls = argumento;
            continue;
        }

        /*
         * Argumento desconhecido.
         */

        std::cerr
            << "erro: argumento desconhecido: "
            << argumento
            << "\n";

        return 1;
    }

    /*
     * Verifica se foi informado um arquivo.
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
     * Verifica se é um arquivo normal.
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
            << "erro: o arquivo precisa possuir "
            << "a extensão .ppls.\n";

        return 1;
    }

    /*
     * Detecta o sistema operacional atual.
     */

    std::string sistema_atual;

#if defined(_WIN32)

    sistema_atual = "windows";

#elif defined(__linux__)

    sistema_atual = "linux";

#else

    sistema_atual = "desconhecido";

#endif

    /*
     * Define o alvo.
     *
     * Se nenhum alvo foi especificado,
     * usa o sistema atual.
     */

    std::string sistema_alvo;

    if (alvo_windows) {
        sistema_alvo = "windows";
    }
    else if (alvo_linux) {
        sistema_alvo = "linux";
    }
    else {
        sistema_alvo = sistema_atual;
    }

    /*
     * Verifica se o sistema atual é suportado.
     */

    if (sistema_atual != "linux" &&
        sistema_atual != "windows") {

        std::cerr
            << "erro: sistema operacional atual "
            << "não suportado.\n";

        return 1;
    }

    /*
     * Define o compilador.
     */

    std::string compilador;

    /*
     * Linux → Linux
     */

    if (sistema_alvo == "linux" &&
        sistema_atual == "linux") {

        compilador = "g++";
    }

    /*
     * Windows → Windows
     */

    else if (sistema_alvo == "windows" &&
             sistema_atual == "windows") {

        compilador = "g++";
    }

    /*
     * Linux → Windows
     */

    else if (sistema_alvo == "windows" &&
             sistema_atual == "linux") {

        compilador = "x86_64-w64-mingw32-g++";
    }

    /*
     * Windows → Linux
     *
     * Não existe um compilador universal que possamos
     * assumir que esteja instalado no Windows.
     *
     * Tentamos usar x86_64-linux-gnu-g++.
     */

    else if (sistema_alvo == "linux" &&
             sistema_atual == "windows") {

        compilador = "x86_64-linux-gnu-g++";
    }

    else {

        std::cerr
            << "erro: combinação de sistema e alvo "
            << "não suportada.\n";

        return 1;
    }

    /*
     * Define o nome padrão do executável.
     */

    if (saida.empty()) {

        saida = caminho_ppls.stem().string();

        if (sistema_alvo == "windows") {
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
     * Obtém o diretório temporário do sistema.
     *
     * Linux / Termux:
     *     normalmente /tmp
     *
     * Windows:
     *     normalmente %TEMP%
     */

    fs::path diretorio_temp;

    try {

        diretorio_temp =
            fs::temp_directory_path();

    }
    catch (const fs::filesystem_error& erro) {

        std::cerr
            << "erro: não foi possível localizar "
            << "o diretório temporário.\n"
            << erro.what()
            << "\n";

        return 1;
    }

    /*
     * Cria o caminho do C++ temporário.
     */

    fs::path arquivo_temporario =
        diretorio_temp / gerar_nome_temporario();

    std::cout
        << "sistema atual: "
        << sistema_atual
        << "\n";

    std::cout
        << "sistema alvo: "
        << sistema_alvo
        << "\n";

    std::cout
        << "compilador: "
        << compilador
        << "\n";

    std::cout
        << "arquivo temporário: "
        << arquivo_temporario
        << "\n";

    /*
     * Cria o arquivo C++ temporário.
     */

    std::ofstream cpp(arquivo_temporario);

    if (!cpp.is_open()) {

        std::cerr
            << "erro: não foi possível criar "
            << "o arquivo temporário:\n"
            << arquivo_temporario
            << "\n";

        return 1;
    }

    /*
     * Cabeçalhos do programa gerado.
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
 * na versão final do ppl-build.
 */

)CPP";

    /*
     * Código PPL.
     */

    cpp << R"CPP(
int main() {

    const char* codigo_ppl = R"PPL_CODE(
)CPP";

    cpp << codigo_ppl;

    cpp << R"CPP(
)PPL_CODE";

    /*
     * Temporariamente o interpretador ainda
     * não está incorporado.
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
     * Remove o C++ temporário.
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
     * Verifica a compilação.
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