/**
 * Shell Project
 * 
 * Criação:         23 Fev 2023
 * Atualização:     24 Fev 2023
 * Compiladores:    [Linux SO]  g++ (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0
 * 
 * Descrição:       Implementação de um Shell.
 * 
 * @author Andevaldo Vitório
 * @version 1.1, 2023-02-24
 *                  
*/

#include <iostream>
#include <string>
#include <vector>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <regex>
#include <iomanip>
#include <map>


// Código de cores ANSI
static const std::string ANSI_COLOR_RED = "\x1b[31m";
static const std::string ANSI_COLOR_GREEN = "\x1b[32m";
static const std::string ANSI_COLOR_CYAN = "\x1b[36m";
static const std::string ANSI_COLOR_WHITE = "\x1b[37m";
static const std::string ANSI_COLOR_RESET = "\x1b[0m";


struct CommandArgsDescription {
    std::string name;
    std::string description;

    CommandArgsDescription(std::string name, std::string description) {
        this->name = name;
        this->description = description;
    }
};

/// @brief Dicionário contendo pequenas descrições dos comandos disponíveis.
static std::map<std::string, std::string> helpDictionary = {
    { "help", "Exibe as informações dos comandos ou de um comando específico" },
    { "echo", "Exibe uma mensagem na tela" },
    { "cd", "Altera o diretório atual" },
    { "pwd", "Exibe o diretório atual" },
    { "ls", "Exibe os itens presente no diretório atual" },
    { "quit", "Finaliza o shell" },
    { "exit", "Finaliza o shell" }
};

/// @brief Dicionário contendo descrições completas dos comandos disponíveis.
static std::map<std::string, std::vector<CommandArgsDescription>> cmdArgsDescription = {
    { 
        "quit", 
        {{ "quit", "Finaliza o processo do shell atual" }} 
    },
    { 
        "exit", 
        {{ "exit", "Finaliza o processo do shell atual" }} 
    },
    {
        "help",
        { 
            { "help", "Exibe as informações dos comandos disponíveis" },
            { "help <nome_do_comando>", "Exibe as informações de um comando específico" } 
        }
    },
    {
        "echo",
        {{ "echo <texto>", "Exibe uma texto na tela" }}
    },
    {
        "cd",
        {{ "cd <path>", "Muda o diretório atual para o caminho especificado. Caminhos com espaços em branco precisam começar e terminar com aspas duplas." }}
    },
    {
        "pwd",
        {{ "pwd", "Exibe o diretório atual" }}
    },
    {
        "ls",
        {
            {"ls", "Exibe os itens não ocultos presentes no diretório atual" },
            {"ls -a", "Exibe todos os itens presentes no diretório atual, inclusive os ocultos" },
            {"ls -l", "Exibe os itens não ocultos presentes no diretório atual em forma de lista" },
            {"ls -la", "Exibe todos os itens presentes no diretório atual, inclusive os ocultos, em forma de lista" },
        }
    }
};

/**
 * Remove os espaços em branco do início e fim
 * de uma string.
 * 
 * @param[in] str Uma string com espaços em branco no início e/ou fim.
 * @return Um string sem espaços em branco no início e no fim.
*/
std::string trim(const std::string & str) {
    const auto begin = str.find_first_not_of(" \t");

    if ( begin == std::string::npos )
        return "";

    const auto end = str.find_last_not_of(" \t");
    const auto len = end - begin + 1;
    
    return str.substr(begin, len);
}

/**
 * Escopos das funções responsáveis em executar
 * os comandos disponíveis.
*/
namespace Runner {

    /**
     * Obtém o nome do usuário atual.
     * 
     * @return O nome do usuário. 
    */
    std::string getCurrentUser() {
        return getlogin();
    }
    
    /**
     * Obtém o nome do dispositivo atual.
     * 
     * @return O nome do dispositivo. 
    */
    std::string getHostname() {
        char $hostname[HOST_NAME_MAX + 1];
        gethostname( $hostname, HOST_NAME_MAX + 1 ); 
        return $hostname;
    }

    /**
     * Apresenta um texto na tela.
     * 
     * @param[in] text Texto a ser mostrado. 
     * @param[in] mode Modo de exibição
    */
    void display(const std::string & text, const char & mode = 'n') {
        
        if ( mode == 'n') std::cout << ANSI_COLOR_RESET << text;
        else if ( mode == 'e' ) std::cout << ANSI_COLOR_RED << "ERROR: " << text;
        
        fflush(stdout);
    }

    /**
     * Obtém o diretório atual.
     * 
     * @return O diretório atual.
    */
    std::string getCurrentDirectory() {
        return get_current_dir_name();
    }

    /**
     * Altera o diretório atual para um camingo específico.
     * 
     * @param[in] path Caminho de destino
     * @return Um valor negativo, caso não consiga alterar o diretório atual.
     *         Caso contrário, a mudança foi realizada com sucesso.
    */
    int changeDirectory(const std::string & path) {
        return chdir(path.c_str());
    }


    // Função auxiliar para comparar os valores
    bool cmp(const dirent * a, const dirent * b) {
        return (std::string) a->d_name < b->d_name;
    }

    /**
     * Altera o diretório atual para um camingo específico.
     * 
     * @param[in] path Caminho de destino
     * @param[in] all Flag que indica para onter todos os itens, inclusive os ocultos.
     * @return A lista de itens presentes no diretório.
    */
    std::vector<dirent *> getItensOfDirectory(const std::string & path, const bool & all = false) {
        DIR *dir = opendir(path.c_str());
        dirent *d;

        if ( dir == nullptr )
             return {}; 

        std::vector<dirent *> dirs;

        while ( (d = readdir(dir)) != nullptr ) {
            
            // Se arquivos ocultos são encontrados
            if ( !all and d->d_name[0] == '.' )
                continue;
            
            dirs.push_back(d);
        }   

        std::sort(dirs.begin(), dirs.end(), cmp);

        return dirs;
    }

    /**
     * Obtém a descrição de um comando específico.
     * 
     * @return A descrição do comando.
    */
    std::string getCommandDescription(std::string name) {
        std::stringstream ss;

        auto args = cmdArgsDescription[name];

        if ( !args.empty() ) {
            ss << "COMANDO:\n" << name << "\n\n";
            ss << "DESCRIÇÃO:\n" << helpDictionary[name] << "\n\n";
            ss << "USO:\n";

            for ( auto & arg: args ) {
                ss << "$ " << std::left << std::setw(32);
                ss << arg.name << arg.description << '\n';
            }
        } else ss << "Comando não encontrado: " << name;

        return ss.str();
    }
    
}
/**
 * Implementação do Shell
 * 
 * É utilizada para representar um shell simples.
*/
class Shell {

    private:

    /**
     * Obtém o(s) argumento(s) de um comando a partir de um texto.
     * 
     * @param[in] command O nome do comando.
     * @param[in] text O texto com o comando e o(s) seu(s) parâmetro(s).
     * @return Os argumentos. 
    */
    std::string getArgs(const std::string & command, std::string & text) {
        std::string arg;

        size_t pos = text.find(command);

        if ( pos != std::string::npos )
            text.erase(pos, command.size());

        return trim(text);
    }
    
    public: 

    bool isRunning = false;     /// @brief Indica se o shell está executando

    /**
     * Contrutor
     * 
     * Inicaliza o Shell e apresenta a mensagem de boas vindas.
    */
    Shell() {
        isRunning = true;

        Runner::display (
            ANSI_COLOR_RESET + 
            "Bem vindo ao Shell Project!\n" +
            "Digite \"exit\" ou \"quit\" para sair."
        );
    }

    std::string getHelpText() {
        std::stringstream ss;

        ss << "Para obter mais informações sobre um comando específico, ";
        ss << "digite: help <nome_do_comando>.\n\n";

        for ( auto it = helpDictionary.begin(); it != helpDictionary.end(); ++it ) {
            ss << std::left << std::setw(16);
            ss << it->first << it->second << '\n';
        }

        return ss.str();
    }


    /**
     * Mostra a linha de comando para o usuário.
    */
    void showCommandLine(void) {
        Runner::display(
            ANSI_COLOR_CYAN + "\n\n" + Runner::getCurrentUser() + "@" + Runner::getHostname() + " " +
            ANSI_COLOR_GREEN + Runner::getCurrentDirectory() + "  " +
            ANSI_COLOR_WHITE + "$ "
        );
    }

    /**
     * Obtém o texto digitado pelo usuário na linha de comando.
     * 
     * @return O texto digitado.
    */
    std::string getTextFromCommandLine() {
        std::string text;
        std::getline(std::cin, text);
        return text;
    }

    /**
     * Tenta executar um comando a partir de um texto.
     * Caso o texto seja válido, o comando é executado.
     * Caso contrário, uma mensagem de erro é apresentada.
     * 
    */
    void runCommandFromText(std::string text) {

        // Comando de saída do shell
        if ( std::regex_match(text, std::regex("(\\s*)(exit|quit)(\\s*)")) )
            isRunning = false;

        // Apresenta a lista de comandos disponíveis
        else if ( std::regex_match(text, std::regex("(\\s*)(help)(\\s*)")) ) {
            Runner::display(getHelpText());
        }

        // Comando de ajuda de um comando específico
        else if ( std::regex_match(text, std::regex("(\\s*)(help)(\\s*)(.*)")) ) {
            std::string cmd = getArgs("help", text);
            Runner::display(Runner::getCommandDescription(cmd));
        }

        // Comando para mostrar um texto
        else if ( std::regex_match(text, std::regex("(\\s*)(echo)(\\s*)(.*)")) ) {
            Runner::display(getArgs("echo", text));
        } 

        // Comando de alteração de diretório
        else if ( std::regex_match(text, std::regex("(\\s*)(cd)(\\s*)(.*)")) ) {
            std::string path = getArgs("cd", text);

            if ( path == "" ) Runner::display("É necessário especificar o caminho.", 'e');
            else if ( std::regex_match(path, std::regex("\"(.*)\"")) ) {
                path = path.substr(1, path.size() - 2);
                if ( Runner::changeDirectory(path) < 0 ) 
                    Runner::display("Diretório não encontrado: " + path, 'e');
            }
            else if ( std::regex_match(path, std::regex("[^\\s]+")) ) {
                if ( Runner::changeDirectory(path) < 0 ) 
                    Runner::display("Diretório não encontrado: " + path, 'e');
            } else if ( std::regex_match(path, std::regex("((.*)(\\s*))+")) )
                Runner::display("Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim do caminho.", 'e');
        }

        // Comando para exibir o atual diretório
        else if ( std::regex_match(text, std::regex("(\\s*)(pwd)(\\s*)")) )
            Runner::display(Runner::getCurrentDirectory());

            // Comando para exibir o atual diretório
        else if ( std::regex_match(text, std::regex("(\\s*)(ls)(\\s*)(.*)")) ) {
            std::string args = getArgs("ls", text);
            bool a = false, l = false;
            std::stringstream ss;

            if ( args == "-a") a = true;
            else if ( args == "-l" ) l = true;
            else if ( args == "-la" ) a = true, l = true;
            else if ( !args.empty() ) {
                Runner::display("Parâmetros inválidos.", 'e');
                return;
            }

            for (dirent * d: Runner::getItensOfDirectory(Runner::getCurrentDirectory(), a))
                ss << d->d_name << (l ?'\n' :'\t');

            if ( ss.str().empty() ) {
                if (errno == ENOENT) Runner::display("Diretório não encontrado!", 'e');
                else Runner::display("Leitura do diretório não permitida!", 'e');
            }
            
            Runner::display(ss.str());
        }
        
        // Quando não é possível obter o comando do texto
        else Runner::display("Comando inválido: " + text, 'e');        
    }

};

int main (void) {

    std::string text;
    Shell shell;

    while ( shell.isRunning ) {
        shell.showCommandLine();
        text = shell.getTextFromCommandLine();
        shell.runCommandFromText(text);
    }

    return EXIT_SUCCESS;
}