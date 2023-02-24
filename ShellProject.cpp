/**
 * Shell Project
 * 
 * Criação:         23 Fev 2023
 * Atualização:     23 Fev 2023
 * Compiladores:    [Linux SO]  g++ (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0
 * 
 * Descrição:       Implementação de um Shell.
 * 
 * @author Andevaldo Vitório
 * @version 1.0, 2023-02-23
 *                  
*/

#include <iostream>
#include <string>
#include <vector>
#include <limits.h>
#include <unistd.h>
#include <sstream>
#include <regex>


// Código de cores ANSI
static const std::string ANSI_COLOR_RED = "\x1b[31m";
static const std::string ANSI_COLOR_GREEN = "\x1b[32m";
static const std::string ANSI_COLOR_CYAN = "\x1b[36m";
static const std::string ANSI_COLOR_WHITE = "\x1b[37m";
static const std::string ANSI_COLOR_RESET = "\x1b[0m";


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
    
}
/**
 * Implementação do Shell
 * 
 * É utilizada para representar um shell simples.
*/
class Shell {

    private:

    /**
     * Obtém argumento do comando 'echo' a partir de um texto.
     * 
     * @param[in] text O texto com o comando 'echo' e seu parâmetro.
     * @return O argumento do comando 'echo'. 
    */
    std::string getEchoArg(std::string text) {
        std::string arg;

        size_t pos = text.find("echo");

        if ( pos != std::string::npos )
            text.erase(pos, 4);

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

        // Comando para mostrar um texto
        else if ( std::regex_match(text, std::regex("(\\s*)(echo)(\\s*)(.*)")) ) {
            Runner::display(getEchoArg(text));
        } 
        
        // Quando não é possível obter o comando do texto
        else Runner::display("Comando não encontrado: " + text, 'e');
        
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