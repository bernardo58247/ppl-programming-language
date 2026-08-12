#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <cstdlib>
#include <stdexcept>

class InterpretadorPPL {
private:
    std::unordered_map<std::string, std::string> escopo_global;

    std::string aparar(const std::string& str) {
        size_t primeiro = str.find_first_not_of(" \t\r\n");

        if (primeiro == std::string::npos) {
            return "";
        }

        size_t ultimo = str.find_last_not_of(" \t\r\n");

        return str.substr(
            primeiro,
            ultimo - primeiro + 1
        );
    }

    std::vector<std::string> dividir(
        const std::string& str,
        char delimitador
    ) {
        std::vector<std::string> tokens;
        std::string token;
        bool em_aspas = false;

        for (char ch : str) {
            if (ch == '"') {
                em_aspas = !em_aspas;
            }

            if (ch == delimitador && !em_aspas) {
                tokens.push_back(token);
                token.clear();
            } else {
                token += ch;
            }
        }

        tokens.push_back(token);

        return tokens;
    }

    std::string buscar_variavel(
        const std::string& nome,
        const std::unordered_map<std::string, std::string>& escopo_local
    ) {
        std::string chave = aparar(nome);

        auto it_local = escopo_local.find(chave);

        if (it_local != escopo_local.end()) {
            return it_local->second;
        }

        auto it_global = escopo_global.find(chave);

        if (it_global != escopo_global.end()) {
            return it_global->second;
        }

        if (
            chave.length() >= 2 &&
            chave.front() == '"' &&
            chave.back() == '"'
        ) {
            return chave.substr(
                1,
                chave.length() - 2
            );
        }

        return chave;
    }

    std::string formatar_texto(
        std::string texto,
        const std::unordered_map<std::string, std::string>& escopo_local
    ) {
        for (const auto& [var, val] : escopo_local) {
            std::string alvo = "$" + var;
            size_t pos = 0;

            while (
                (pos = texto.find(alvo, pos))
                != std::string::npos
            ) {
                texto.replace(
                    pos,
                    alvo.length(),
                    val
                );

                pos += val.length();
            }
        }

        for (const auto& [var, val] : escopo_global) {
            std::string alvo = "$" + var;
            size_t pos = 0;

            while (
                (pos = texto.find(alvo, pos))
                != std::string::npos
            ) {
                texto.replace(
                    pos,
                    alvo.length(),
                    val
                );

                pos += val.length();
            }
        }

        return texto;
    }

    std::string resolver_caminho_ppll(
        const std::string& nome_lib
    ) {
        std::string nome_limpo = aparar(nome_lib);

        if (
            nome_limpo.length() >= 2 &&
            nome_limpo.front() == '"' &&
            nome_limpo.back() == '"'
        ) {
            nome_limpo = nome_limpo.substr(
                1,
                nome_limpo.length() - 2
            );
        }

        std::vector<std::string> candidatos_locais = {
            nome_limpo,
            nome_limpo + ".ppll",
            nome_limpo + ".ppl"
        };

        for (const auto& caminho : candidatos_locais) {
            std::ifstream arq(caminho);

            if (arq.good()) {
                return caminho;
            }
        }

        const char* env_home = std::getenv("HOME");

        if (env_home) {
            std::string home(env_home);

            std::vector<std::string> candidatos_globais = {
                home + "/.ppl/modules/" +
                    nome_limpo + "/" +
                    nome_limpo + ".ppll",

                home + "/.ppl/modules/" +
                    nome_limpo + "/" +
                    nome_limpo + ".ppl",

                home + "/.ppl/modules/" +
                    nome_limpo + "/index.ppll",

                home + "/.ppl/modules/" +
                    nome_limpo + "/index.ppl"
            };

            for (const auto& caminho : candidatos_globais) {
                std::ifstream arq(caminho);

                if (arq.good()) {
                    return caminho;
                }
            }
        }

        return "";
    }

public:
    void executar(
        const std::string& codigo,
        std::unordered_map<std::string, std::string> escopo_local = {}
    ) {
        std::vector<std::string> linhas =
            dividir(codigo, '\n');

        size_t num_linha = 0;

        while (num_linha < linhas.size()) {
            std::string linha_texto =
                aparar(linhas[num_linha]);

            size_t num_linha_real =
                num_linha + 1;

            if (
                linha_texto.empty() ||
                linha_texto.rfind("//", 0) == 0
            ) {
                num_linha++;
                continue;
            }

            try {

                /*
                 * importar
                 */

                if (
                    linha_texto.rfind(
                        "importar",
                        0
                    ) == 0
                ) {
                    std::regex padrao_importar(
                        R"(importar\s*\"(.*?)\"|importar\s+(.+))"
                    );

                    std::smatch match;

                    if (
                        std::regex_search(
                            linha_texto,
                            match,
                            padrao_importar
                        )
                    ) {
                        std::string nome_lib =
                            match[1].matched
                                ? match[1].str()
                                : match[2].str();

                        std::string caminho_resolvido =
                            resolver_caminho_ppll(
                                nome_lib
                            );

                        if (caminho_resolvido.empty()) {
                            throw std::runtime_error(
                                "biblioteca '" +
                                nome_lib +
                                "' não encontrada (.ppll)"
                            );
                        }

                        std::ifstream arquivo_lib(
                            caminho_resolvido
                        );

                        if (arquivo_lib.is_open()) {
                            std::string codigo_lib(
                                (
                                    std::istreambuf_iterator<char>(
                                        arquivo_lib
                                    )
                                ),
                                std::istreambuf_iterator<char>()
                            );

                            arquivo_lib.close();

                            executar(
                                codigo_lib,
                                escopo_local
                            );
                        } else {
                            throw std::runtime_error(
                                "falha ao abrir a biblioteca '" +
                                caminho_resolvido +
                                "'"
                            );
                        }
                    } else {
                        throw std::runtime