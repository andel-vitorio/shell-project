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
        if ( std::regex_match(text, std::regex("(\\s*)(exit|quit)(\\s*)")) ) {
            isRunning = false;
        } else {
            Runner::display("Comando não encontrado: " + text, 'e');
        }
        
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