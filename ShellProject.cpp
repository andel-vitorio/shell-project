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
#include <limits>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <regex>
#include <iomanip>
#include <map>
#include <fcntl.h>
#include <sys/stat.h>

#define OPEN_FAILURE -1
#define READ_FAILURE -2
#define MALLOC_FAILURE -3
#define WRITE_FAILURE -4
#define FILE_FAILURE -5
#define SAME_FILE -6

// Código de cores ANSI
static const std::string ANSI_COLOR_RED = "\x1b[31m";
static const std::string ANSI_COLOR_GREEN = "\x1b[32m";
static const std::string ANSI_COLOR_CYAN = "\x1b[36m";
static const std::string ANSI_COLOR_WHITE = "\x1b[37m";
static const std::string ANSI_COLOR_RESET = "\x1b[0m";
static const std::string CLEAR_CODE = "\033[2J\033[1;1H";


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
    { "clear", "Limpar a tela do shell" },
    { "cd", "Altera o diretório atual" },
    { "pwd", "Exibe o diretório atual" },
    { "ls", "Exibe os itens presente no diretório atual" },
    { "cat", "Exibe o conteúdo de um arquivo no shell" },
    { "touch", "Gera um arquivo arquivo em branco" },
    { "cp", "Copia o conteúdo de um arquivo em outro arquivo" },
    { "mkdir", "Gera diretórios" },
    { "rmdir", "Exclui um diretório" },
    { "rmfile", "Exclui um arquivo" },
    { "mv", "Move ou renomeia um arquivo ou diretório" },
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
        "clear",
        {{ "clear", "Limpa a tela do shell" }}
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
    },
    {
        "cat",
        {{ "cat <nome_do_arquivo>", "O comando cat permite a visualização do conteúdo de um arquivo" }}
    },
    {
        "touch",
        {{ "touch <nome_do_arquivo>", "Gera um arquivo em branco com o nome especificado" }}
    },
    {
        "cp",
        {{ "cp <nome_do_arquivo_1> <nome_do_arquivo_2>", "Copia todo o conteúdo do Arquivo 1 no Arquivo 2" }}
    },
    {
        "mkdir",
        {{ "mkdir <nome_do_diretório> ", "Gera um diretório" }}
    },
    {
        "rmdir",
        {{ "rmdir <nome_do_diretório> ", "Remove o diretório." }}
    },
    {
        "rmfile",
        {{ "rmfile /caminho/do/arquivo.ext", "Remove o arquivo no caminho especificado." }}
    },
    {
        "rmfile",
        {{ "mv <caminho/de/origem> <caminho/de/detino>", "Move ou renomeia um arquivo ou diretorio." }}
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
 * Obtém as substrings de um string delimitadas por um determinado caracter.
 * 
 * @param[in] str String a ser processada
 * @param[in] character O caracter delimitador
 * @return Um lista de substrings.
*/
std::vector<std::string> getSubstrings(const std::string & str, const char & character) {
    std::vector<std::string> substrings;
    size_t start = 0;
    size_t end;

    while ( ( start = str.find(character, start) ) != std::string::npos ) {
        end = str.find(character, start + 1);
        
        if (end == std::string::npos)
            break;

        substrings.push_back(str.substr(start + 1, end - start - 1));
        start = end + 1;
    }


    return substrings;
}

std::vector<std::string> split(const std::string & str, const char & character) {
    std::vector<std::string> substrings;
    size_t start = 0;
    size_t end;

    while ( ( end = str.find(character, start)) != std::string::npos ) {
        if ( end != start ) substrings.push_back(str.substr(start, end - start));
        start = end + 1;
    }

    substrings.push_back(str.substr(start));
    
    return substrings;
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
     * Limpa a tela do shell.
    */
    void clear() {
        std::cout << CLEAR_CODE;      
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
     * Obtém o conteúdo de um arquivo
     * 
     * @param[in] file Caminho do arquivo
     * @param[in, out] content String que conterá o conteúdo do arquivo
     * @return status da operação
    */
    int getFileContent(const std::string & file, std::string & content) {
        int fd = open(file.c_str(), O_RDONLY);
        ssize_t nread, total = 0;
        char *buffer;

        if ( fd < 0 ) return OPEN_FAILURE;

        off_t fileSize = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        // Aloca um buffer do tamanho do arquivo
        buffer = (char *) malloc(fileSize);

        if ( buffer == nullptr ) return MALLOC_FAILURE;

        // Lê o arquivo em um loop
        while ( ( nread = read(fd, buffer + total, fileSize - total)) > 0 )
            total += nread;

        if ( nread < 0 ) return READ_FAILURE;

        if ( buffer[fileSize - 1] == '\n' )
            buffer[fileSize - 1] = '\0';

        content = buffer;

        free(buffer);
        close(fd);

        return EXIT_SUCCESS;
    }

     /**
     * Gera um arquivo em branco com um determinado nome
     * 
     * @param[in] filename Nome do arquivo a ser criado
     * @return status da operação
    */
    int createBlankFile(const std::string & filename) {
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int fd = open(filename.c_str(), O_WRONLY | O_CREAT, mode);

        if ( fd < 0 ) return OPEN_FAILURE;
        close(fd);

        return EXIT_SUCCESS;
    }

     /**
     * Copia o conteúdo de um arquivo em outro arquivo.
     * 
     * @param[in] source Arquivo de origem
     * @param[in] target Arquivo de destino
     * @return status da operação
    */
    int copyContentFile(const std::string & source, const std::string & target) {
        std::string sourceContent;
        
        int status =  getFileContent(source, sourceContent);

        if ( status < 0 ) return status;

        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int fd = open(target.c_str(), O_WRONLY | O_CREAT, mode);

        ssize_t s = write(fd, sourceContent.c_str(), sourceContent.size());
        close(fd);

        if ( s != sourceContent.size() )
            return WRITE_FAILURE;

        return EXIT_SUCCESS;
    }

    /**
     * Gera um diretório.
     * 
     * @param[in] path Caminho do diretório
     * @return status da operação
    */
    int createDirectory(std::string & path) {
        std::string p = "./";
        int status;

        if ( path[0] == '/' or ( path[0] == '.' and path[1] == '/' ) )
            p = "";

        for ( auto &str: split(path, '/') ) {
            p += str + "/";
            status = mkdir(p.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        }

        if ( status < 0 ) return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    /**
     * Exlui um arquivo.
     * 
     * @param[in] path Caminho do arquivo
     * @return status da operação
    */
    int removeFile(std::string & path) {

        if ( not (path[0] == '/' or ( path[0] == '.' and path[1] == '/') ) )
            path = "./" + path;

        if ( !unlink(path.c_str())) return EXIT_SUCCESS;
        else return EXIT_FAILURE;   
    }

    /**
     * Exlui um diretório.
     * 
     * @param[in] path Caminho do diretório
     * @return status da operação
    */
    int removeDirectory(std::string & path) {

        if ( not (path[0] == '/' or ( path[0] == '.' and path[1] == '/') ) )
            path = "./" + path;


        auto itens = getItensOfDirectory(path, true);

        // O diretório não está vazio
        if ( itens.size() > 2 ) {

            struct stat st;

            for ( auto &item: itens ) {

                if ( !strcmp(item->d_name, ".") or !strcmp(item->d_name, "..") )
                    continue;

                
                std::string p = path + "/" + item->d_name;

                if ( lstat(p.c_str(), &st) < 0 ) return READ_FAILURE;

                if ( S_ISDIR(st.st_mode) and removeDirectory(p) != EXIT_SUCCESS )
                    return EXIT_FAILURE;


                int status = removeFile(p);
            
                if ( status == EXIT_SUCCESS ) continue;
                else return status;
            }
        }


        if ( rmdir(path.c_str()) != 0 ) return EXIT_FAILURE;
        return EXIT_SUCCESS;
        
    }

    /**
     * Move ou renomeia arquivos e diretórios.
     * 
     * @param[in] source Diretório ou arquivo de origem
     * @param[in] target Diretório ou arquivo de destino
     * @return status da operação
    */
    int moveFiles( std::string & source, const std::string & target ) {

        // Verifica se a origem existe
        struct stat source_sb;
        
        if ( stat(source.c_str(), &source_sb) == -1 ) 
            return FILE_FAILURE;
    
        // Verifica se a origem e o destino são o mesmo arquivo
        struct stat target_sb;
        
        if ( stat(target.c_str(), &target_sb) != -1 && target_sb.st_ino == source_sb.st_ino ) 
            return SAME_FILE;

        // Verifica se o destino já existe
        if ( stat(target.c_str(), &target_sb) != -1 ) {

            if ( S_ISDIR(target_sb.st_mode) ) {

                // O destino é um diretório, adiciona o nome do arquivo à pasta
                const char *filename = strrchr(source.c_str(), '/');

                if ( !filename ) filename = source.c_str();
                else filename++;

                char targetPath[ source.size() + strlen(filename) + 2];

                sprintf(targetPath, "%s/%s", target.c_str(), filename);

                if ( rename(source.c_str(), targetPath) == -1 ) 
                    return EXIT_FAILURE;

            } else if ( rename(source.c_str(), target.c_str()) == -1 )
                return EXIT_FAILURE;
        } else {
            // O destino não existe, renomeia o arquivo
            if ( rename(source.c_str(), target.c_str()) == -1 )
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
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

    /**
     * Obtém o caminho no formato ./caminho/qualquer.
     * 
     * @param[in] path O caminho a ser convertido.
     * @return status da operação. 
    */
    int getPath(std::string & arg) {
        if ( arg == "" ) return -1;

        if ( std::regex_match(arg, std::regex("\"(.*)\"")) )
            arg = arg.substr(1, arg.size() - 2);
        else if ( arg.find(" ") != std::string::npos ) return -2;
        
        if ( not (arg[0] == '/' or ( arg[0] == '.' and arg[1] == '/') ) )
            arg = "./" + arg;

        std::string home = getenv("HOME");
        int pos = arg.find("./~");

        if ( pos != std::string::npos )
            arg.replace(pos, 3, home);

        return 0;
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

        Runner::clear();
        Runner::display (
            ANSI_COLOR_RESET + 
            "Bem vindo ao Shell Project!\n" +
            "Digite \"exit\" ou \"quit\" para sair."
        );
        
        Runner::display(getHelpText());
    }

    std::string getHelpText() {
        std::stringstream ss;

        ss << "Para obter mais informações sobre um comando específico, ";
        ss << "digite: help <nome_do_comando>.\n\n";

        int i = 1;

        for ( auto it = helpDictionary.begin(); it != helpDictionary.end(); ++it ) {
            ss << std::left << std::setw(4) << i << " ";
            ss << std::left << std::setw(16);
            ss << it->first << it->second << '\n';
            i++;
        }

        return ss.str();
    }


    /**
     * Mostra a linha de comando para o usuário.
    */
    void showCommandLine(void) {
        std::string current = Runner::getCurrentDirectory();
        std::string home = getenv("HOME");
        int pos = current.find(home);

        if ( pos != std::string::npos )
            current.replace(pos, home.size(), "~");

        Runner::display(
            ANSI_COLOR_CYAN + "\n\n" + Runner::getCurrentUser() + "@" + Runner::getHostname() + " " +
            ANSI_COLOR_GREEN + current + "  " +
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

        // Comando para limpar a tela do shell
        else if ( std::regex_match(text, std::regex("(\\s*)(clear)(\\s*)(.*)")) ) {
            Runner::clear();
        } 

        // Comando de alteração de diretório
        else if ( std::regex_match(text, std::regex("(\\s*)(cd)(\\s*)(.*)")) ) {
            std::string arg = getArgs("cd", text);
            int status = getPath(arg);

            if ( status == -1 ) {
                Runner::display("É necessário especificar o diretório de destino.", 'e');
                return;
            } else if ( status == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas um parâmetro.");
                return;
            }

            if ( Runner::changeDirectory(arg) < 0 ) 
                Runner::display("Diretório não encontrado: " + arg, 'e');
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

            if (errno == ENOENT) {
                Runner::display("Diretório não encontrado!", 'e');
                return;
            }
        
            Runner::display(ss.str());
        }

        // Comando para visualizar o conteúdo de um arquivo
        else if ( std::regex_match(text, std::regex("(\\s*)(cat)(\\s*)(.*)")) ) {
            std::string arg = getArgs("cat", text);
            int status = getPath(arg);

            if ( status == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( status == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas um parâmetro.");
                return;
            }

            std::string content;
            status = Runner::getFileContent(arg, content);
            
            if ( status  == OPEN_FAILURE ) {
                Runner::display("Arquivo não encontrado: " + arg + "\n", 'e');
                Runner::display("OBS 1: Verifique se o caminho para o arquivo está correto.\n");
                Runner::display("OBS 2: É necessário informar a extensão do arquivo.\n");
            }
            else if ( status == MALLOC_FAILURE ) Runner::display("Erro ao alocar recursos.", 'e');
            else if ( status == READ_FAILURE ) Runner::display("Erro ao realizar a leitura do arquivo.", 'e');
            else Runner::display(content);

        }

        // Comando para criar um arquivo em branco
        else if ( std::regex_match(text, std::regex("(\\s*)(touch)(\\s*)(.*)")) ) {
            std::string arg = getArgs("touch", text);
            int status = getPath(arg);

            if ( status == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( status == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas um parâmetro.");
                return;
            }

            status = Runner::createBlankFile(arg);
            
            if ( status  == OPEN_FAILURE ) Runner::display("O arquivo não pode ser criado!", 'e');
            else Runner::display("Arquivo gerado com sucesso!");
        }

        // Comando para criar um arquivo em branco
        else if ( std::regex_match(text, std::regex("(\\s*)(cp)(\\s*)(.*)")) ) {
            std::string args = getArgs("cp", text);
            std::vector<std::string> paths;

            if ( std::regex_match(args, std::regex("(\"(.*)\")(\\s)+((\"(.*)\"))")) )
                paths = getSubstrings(args, '"');
            else if ( std::regex_match(args, std::regex("([^\\s]+)(\\s)+([^\\s]+)")) )
                paths = split(args, ' ');
            else {
                Runner::display("Parâmetros inválidos.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas dois parâmetro.");
                return;
            }

            if ( paths.empty() ) {
                Runner::display("É necessário especificar os nomes dos arquivos.", 'e');
                return;
            }

            int s1 = getPath(paths[0]), s2 = getPath(paths[1]);

            if ( s1 == -1 or s2 == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( s1 == -2 or s2 == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas dois parâmetro.");
                return;
            }
            
            int status = Runner::copyContentFile(paths[0], paths[1]);
           
            if ( status  == OPEN_FAILURE ) Runner::display("O arquivo de origem não pode ser encontrado!", 'e');
            else if ( status  == READ_FAILURE ) Runner::display("O arquivo de origem não pode ser lido!", 'e');
            else if ( status == MALLOC_FAILURE ) Runner::display("Erro ao alocar recursos.", 'e');
            else if ( status  == WRITE_FAILURE) Runner::display("O arquivo de destino não pode escrito!", 'e');
            else Runner::display("Conteúdo copiado com sucesso!");
        }

        // Comando para criar um diretório
        else if ( std::regex_match(text, std::regex("(\\s*)(mkdir)(\\s*)(.*)")) ) {
            std::string arg = getArgs("mkdir", text);
            int status = getPath(arg);

            if ( status == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( status == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas um parâmetro.");
                return;
            }

            status = Runner::createDirectory(arg);
            
            if ( status == EXIT_FAILURE ) Runner::display("Ocorreu um problema ao criar o diretório!", 'e');
            else Runner::display("Diretório criado com sucesso!");
        }

        // Comando para remover um diretório
        else if ( std::regex_match(text, std::regex("(\\s*)(rmdir)(\\s*)(.*)")) ) {
            std::string arg = getArgs("rmdir", text);
            int status = getPath(arg);

            if ( status == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( status == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas um parâmetro.");
                return;
            }

            bool isEmpty = Runner::getItensOfDirectory(arg, true).size() <= 2;

            if ( isEmpty ) {
                if ( Runner::removeDirectory(arg) == EXIT_FAILURE ) {
                    Runner::display("Ocorreu um problema ao remover o diretório!", 'e');
                    return;
                }
            } else {
                Runner::display("Este diretório contém arquivos e/ou diretórios. Ao continuar, todos serão removidos.\n");
                Runner::display("Deseja continuar [s/n]? ");
                std::string res;

                std::cin >> res;

                // Limpa o buffer de entrada do objeto std::cin
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if ( trim(res) == "s" ) {
                    if ( Runner::removeDirectory(arg) == EXIT_FAILURE ) {
                        Runner::display("Ocorreu um problema ao remover o diretório!", 'e');
                        return;
                    }
                } else {
                    Runner::display("Diretório não removido!");
                }

            }

            Runner::display("Diretório removido com sucesso!");
        }

        // Comando para criar um arquivo em branco
        else if ( std::regex_match(text, std::regex("(\\s*)(rmfile)(\\s*)(.*)")) ) {
            std::string arg = getArgs("rmfile", text);
            int status = getPath(arg);

            if ( status == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( status == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas um parâmetro.");
                return;
            }

            status = Runner::removeFile(arg);
            
            if ( status  == EXIT_FAILURE ) Runner::display("O arquivo não pode ser removido!", 'e');
            else Runner::display("Arquivo removido com sucesso!");
        }

        // Move arquivos
        else if ( std::regex_match(text, std::regex("(\\s*)(mv)(\\s*)(.*)")) ) {
            std::string args = getArgs("mv", text);
            std::vector<std::string> paths;

            if ( std::regex_match(args, std::regex("(\"(.*)\")(\\s)+((\"(.*)\"))")) )
                paths = getSubstrings(args, '"');
            else if ( std::regex_match(args, std::regex("([^\\s]+)(\\s)+([^\\s]+)")) )
                paths = split(args, ' ');
            else {
                Runner::display("Parâmetros inválidos.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas dois parâmetro.");
                return;
            }

            if ( paths.empty() ) {
                Runner::display("É necessário especificar os nomes dos arquivos.", 'e');
                return;
            }

            int s1 = getPath(paths[0]), s2 = getPath(paths[1]);

            if ( s1 == -1 or s2 == -1 ) {
                Runner::display("É necessário especificar o caminho correto do arquivo.", 'e');
                return;
            } else if ( s1 == -2 or s2 == -2 ) {
                Runner::display("Diretório de destino inválido.\n", 'e');
                Runner::display("OBS 1: Caminhos com espaços em branco precisam utilizar aspas duplas no início e no fim.\n");
                Runner::display("OBS 2: Este comando aceita apenas dois parâmetro.");
                return;
            }

            int status = Runner::moveFiles(paths[0], paths[1]);
            
            if ( status  == OPEN_FAILURE ) Runner::display("O arquivo de origem não pode ser encontrado!", 'e');
            else if ( status  == READ_FAILURE ) Runner::display("O arquivo de origem não pode ser lido!", 'e');
            else if ( status == MALLOC_FAILURE ) Runner::display("Erro ao alocar recursos.", 'e');
            else if ( status  == WRITE_FAILURE) Runner::display("O arquivo não pode ser movido!", 'e');
            else if ( status  == EXIT_FAILURE) Runner::display("O arquivo não pode ser movido!", 'e');
            else Runner::display("Arquivo movido com sucesso!");
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