//gcc -g -Og -Wall compilador.c -o compilador
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
const int TAMANHO_MAXIMO = 100;
const int TAMANHO_ID = 16;

int quantidade = 1;

// definicao de tipo
typedef enum {
    ERRO,
    IDENTIFICADOR,
    NUMERO,
    EOS,
    COMENTARIO,
    AND,
    BEGIN,
    BOOLEAN,
    ELIF,
    END,
    FALSE,
    FOR,
    IF,
    INTEGER,
    NOT,
    OF,
    OR,
    PROGRAM,
    READ,
    SET,
    TO,
    TRUE,
    WRITE,
    VIRGULA,
    PONTO_E_VIRGULA,
    PONTO,
    MENOR,
    MENOR_IGUAL,
    IGUAL,
    DIFERENTE,
    MAIOR,
    MAIOR_IGUAL,
    MAIS,
    MENOS,
    ASTERISCO,
    BARRA,
    ABRE_PARENTESE,
    FECHA_PARENTESE,
    DOIS_PONTOS
} TAtomo;

typedef struct{
    TAtomo atomo;
    int linha;
    float atributo_numero;
    char atributo_ID[16];
}TInfoAtomo;

char *msgAtomo[] = {
    "erro",
    "identificador",
    "numero",
    "eos",
    "comentario",
    "and",
    "begin",
    "boolean",
    "elif",
    "end",
    "false",
    "for",
    "if",
    "integer",
    "not",
    "of",
    "or",
    "program",
    "read",
    "set",
    "to",
    "true",
    "write",
    "virgula",
    "ponto e virgula",
    "ponto",
    "menor",
    "menor igual",
    "igual",
    "diferente",
    "maior",
    "maior igual",
    "mais",
    "menos",
    "asterisco",
    "barra",
    "abre parenteses",
    "fecha parenteses",
    "dois pontos"
};

// Declaração das funções
void programa();
void bloco();
void declaracao_de_variaveis();
void tipo();
char ** lista_variavel();
void comando_composto();
void comando();
void comando_atribuicao();
void comando_condicional();
void comando_repeticao();
void comando_entrada();
void comando_saida();
void expressao();
void expressao_logica();
void expressao_relacional();
void op_relacional();
void expressao_simples();
void termo();
void fator();
void consome(TAtomo atomo);
int busca_tabela_simbolos(char * identificador);
TInfoAtomo obter_atomo();

int rotulo_atual = 0;

int proximo_rotulo() {
    rotulo_atual += 1;
    return rotulo_atual;
}

TInfoAtomo InfoAtomo;
TInfoAtomo lookahead;
int contaLinha = 1;
// secao reservada para palavras reservadas em msgAtomo
const int reservado_comeco = 5;
const int reservado_tamanho = 18;

// tipo de comentario: 0 para sem comentario, 1 para #, 2 para {--}
typedef enum {
    SEM_COMENTARIO = 0,
    COMENTARIO_LINHA = 1,
    COMENTARIO_BLOCO = 2
} TipoComentario;
// variavel global para o analisador lexico
// variavel bufer devera ser inicializada a partir de um arquivo texto
char *buffer;

TipoComentario comentario = SEM_COMENTARIO;

typedef struct Simbolo { //implementacao de linked list
    char * identificador;
    int endereco;
    struct Simbolo * proximo;
} Simbolo;

Simbolo* primeiro_simbolo = NULL; //raiz da linked list
Simbolo* ultimo_simbolo = NULL; //final da linked list, usado durante criacao de novos simbolos na tabela
                                //para ter facil acesso durante a mudanca do ponteiro do proximo simbolo
int indice_simbolo = 0;

void criar_simbolo(char * identificador) {
    if (indice_simbolo == 0 || busca_tabela_simbolos(identificador) == -1) { //lista sem elementos dentro ou elemento nao existe na lista
        Simbolo * novo = (Simbolo*)malloc(sizeof(Simbolo));
        novo->identificador = strdup(identificador);
        novo->endereco = indice_simbolo;
        novo->proximo = NULL;
        if (indice_simbolo == 0) { //filtra para caso de lista vazia
            primeiro_simbolo = novo;
        }
        else {
            ultimo_simbolo->proximo = novo;
        }
        ultimo_simbolo = novo;
       // printf("RESULTADO 0: %d\n", indice_simbolo);
        indice_simbolo += 1; //acrescenta o endereco final atual
    }
    else { //se a lista nao estiver vazia e o simbolo for encontrado
       // printf("RESULTADO: %d\n", busca_tabela_simbolos(identificador));
        printf("Erro semantico: O simbolo ja existe na tabela.\n");
        exit(1);
    }
}

void imprimir_tabela() {
    Simbolo * busca = primeiro_simbolo;
    printf("TABELA DE SIMBOLOS\n");
    while (busca != NULL) {
        printf("%s   |   Endereco: %d\n", busca->identificador, busca->endereco);
        busca = busca->proximo;
    }
}

int busca_tabela_simbolos(char * identificador) { //retorna endereco do simbolo, -1 caso nao exista
    Simbolo * busca = primeiro_simbolo;
    while (busca != NULL) {
        if (strcmp(busca->identificador, identificador) == 0) {
            return busca->endereco;
        }
        busca = busca->proximo;
    }
    return -1;
}

void conferir_existencia(char * identificador) {
    if (busca_tabela_simbolos(identificador) == -1) {
        printf("Erro semantico: O símbolo já existe na tabela.\n");
        exit(1);
    }
}

// declaracao da funcao
TInfoAtomo reconhece_id();
bool eh_binario(char c);

void ler_arquivo(const char *nome_do_arquivo) 
{
    FILE *file = fopen(nome_do_arquivo, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    // Determina o tamanho do arquivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Aloca memória para o buffer e lê o conteúdo do arquivo
    buffer = malloc(file_size + 1);
    if (!buffer) {
        perror("Erro ao alocar memória");
        fclose(file);
        exit(1);
    }

    size_t bytesRead = fread(buffer, 1, file_size, file);
    if (bytesRead > (size_t)file_size) { // Lidar com o erro, por exemplo, exibir uma mensagem de erro
        printf("Erro ao ler o arquivo");
        free(buffer);
        fclose(file);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <nome_do_arquivo>\n", argv[0]);
        return 1;
    }

    ler_arquivo(argv[1]);

    // Inicializa o lookahead
    lookahead = obter_atomo();

    // Inicia a análise sintática
    programa();

    if (lookahead.atomo == EOS) {
        printf("A palavra pertence a linguagem gerada pela gramatica.\n");
    } else {
        printf("Erro sintatico na palavra.\n");
    }

    imprimir_tabela();
    printf("Fim da analise lexica e sintatica\n");
    return 0;
}

int binarioParaDecimal(const char *binario) {
    int decimal = 0;
    int tamanho = strlen(binario);
    
    for (int i = 0; i < tamanho; i++) {
        if (binario[tamanho - i - 1] == '1') {
            decimal += pow(2, i);
        }
    }
    return decimal;
}

// Função para obter o próximo token (simulação)
TInfoAtomo obter_atomo_sintatico() {
    TInfoAtomo token;
    if (*buffer == '\0') {
        token.atomo = EOS;
    } else if (strncmp(buffer, "program", 7) == 0) {
        token.atomo = PROGRAM;
        buffer += 7;
    } else if (strncmp(buffer, "true", 4) == 0) {
        token.atomo = TRUE;
        buffer += 4;
    } else if (strncmp(buffer, "false", 5) == 0) {
        token.atomo = FALSE;
        buffer += 5;
    } else if (strncmp(buffer, "begin", 5) == 0) {
        token.atomo = BEGIN;
        buffer += 5;
    } else if (*buffer == ';') {
        token.atomo = PONTO_E_VIRGULA;
        buffer++;
    } else if (*buffer == '.') {
        token.atomo = PONTO;
        buffer++;
    } else if (strncmp(buffer, "end", 3) == 0) {
        token.atomo = END;
        buffer += 3;
    } else if (strncmp(buffer, "integer", 7) == 0) {
        token.atomo = INTEGER;
        buffer += 7;
    } else if (strncmp(buffer, "boolean", 7) == 0) {
        token.atomo = BOOLEAN;
        buffer += 7;
    } else if (strncmp(buffer, ",", 1) == 0) {
        token.atomo = VIRGULA;
        buffer += 1;
    } else if (strncmp(buffer, "set", 3) == 0) {
        token.atomo = SET;
        buffer += 3;
    } else if (strncmp(buffer, "if", 2) == 0) {
        token.atomo = IF;
        buffer += 2;
    } else if (strncmp(buffer, "for", 3) == 0) {
        token.atomo = FOR;
        buffer += 3;
    } else if (strncmp(buffer, "read", 4) == 0) {
        token.atomo = READ;
        buffer += 4;
    } else if (strncmp(buffer, "write", 5) == 0) {
        token.atomo = WRITE;
        buffer += 5;
    } else if (strncmp(buffer, "elif", 4) == 0) {
        token.atomo = ELIF;
        buffer += 4;
    } else if (strncmp(buffer, "of", 2) == 0) {
        token.atomo = OF;
        buffer += 2;
    } else if (strncmp(buffer, "and", 3) == 0) {
        token.atomo = AND;
        buffer += 3;
    } else if (strncmp(buffer, "to", 2) == 0) {
        token.atomo = TO;
        buffer += 2;
    } else if (strncmp(buffer, ":", 1) == 0) {
        token.atomo = DOIS_PONTOS;
        buffer += 1;
    } else if (strncmp(buffer, "+", 1) == 0) {
        token.atomo = MAIS;
        buffer += 1;
    } else if (strncmp(buffer, "-", 1) == 0) {
        token.atomo = MENOS;
        buffer += 1;
    } else if (strncmp(buffer, "*", 1) == 0) {
        token.atomo = ASTERISCO;
        buffer += 1;
    } else if (strncmp(buffer, "/", 1) == 0) {
        token.atomo = BARRA;
        buffer += 1;
    } else if (strncmp(buffer, "<", 1) == 0) {
        if (strncmp(buffer, "<=", 2) == 0) {
            token.atomo = MENOR_IGUAL;
            buffer += 2;
        }
        else {
            token.atomo = MENOR;
            buffer += 1;
        }
    } else if (strncmp(buffer, ">", 1) == 0) {
        if (strncmp(buffer, ">=", 2) == 0) {
            token.atomo = MAIOR_IGUAL;
            buffer += 2;
        }
        else {
            token.atomo = MAIOR;
            buffer += 1;
        }
    } else if (strncmp(buffer, "=", 1) == 0) {
        token.atomo = IGUAL;
        buffer += 1;
    } else if (strncmp(buffer, "/=", 2) == 0) {
        token.atomo = DIFERENTE;
        buffer += 2;
    } else if (strncmp(buffer, "(", 1) == 0) {
        token.atomo = ABRE_PARENTESE;
        buffer += 1;
    } else if (strncmp(buffer, ")", 1) == 0) {
        token.atomo = FECHA_PARENTESE;
        buffer += 1;
    } else if (strncmp(buffer, "0b", 2) == 0) {
        token.atomo = NUMERO;
        buffer += 2;
        char *numero = malloc(strlen(buffer) + 1); //aloca memória dinamicamente para armazenar o número binário
        if (!numero) {
            perror("Erro ao alocar memória");
            exit(1);
        }
        int i = 0;
        while (eh_binario(*buffer)) { //se ainda for binario guarda em numero
            numero[i++] = *buffer++;
        }
        numero[i] = '\0'; //finalizador
        token.atributo_numero = (float) binarioParaDecimal(numero); //converte o número binário para decimal e armazena em atributo_numero
        free(numero);
    } else if (islower(*buffer) && isalpha(*buffer)) {
        token.atomo = IDENTIFICADOR;
        int i = 0;
        while ((isalpha(*buffer) || isdigit(*buffer) || *buffer == '_')) {
            if ( i == 15){
                printf("Erro lexico: tamanho do identificador maior que 15.\n");
                token.atomo = ERRO;
                break;
            }
            token.atributo_ID[i++] = *buffer++;
        }
        token.atributo_ID[i] = '\0';
    } else {
        token.atomo = ERRO;
    }
    return token;
}

TInfoAtomo obter_atomo(){
    TInfoAtomo info_atomo;

    // consome espaços em branco quebra de linhas tabulação e retorno de carro
    // adicionado gerenciamento de comentarios,
    // continua no loop até finalizar cada tipo ou acabar a string
g3:
    while(*buffer == ' ' || *buffer == '\n' || *buffer == '\t' ||*buffer == '\r' || (comentario != SEM_COMENTARIO && *buffer != 0)){
        if( *buffer =='\n'){
            if( comentario == COMENTARIO_LINHA)
                comentario = SEM_COMENTARIO;
            contaLinha++;
        }
        else if ( comentario == COMENTARIO_BLOCO && *buffer == '-'){
            buffer++;
            if ( *buffer == '}')
                comentario = SEM_COMENTARIO;
        }
        buffer++;
    }
    // reconhece identificador
    if( *buffer == '{' || *buffer == '#'){ // ser for letra mininuscula, inicio de comentario ou numero
        info_atomo = reconhece_id();
    }else if(*buffer == 0){
        info_atomo.atomo = EOS;
    }
    else{
        info_atomo = obter_atomo_sintatico();
    }
    if ( info_atomo.atomo == COMENTARIO) { goto g3; }
    info_atomo.linha = contaLinha;
    return info_atomo;

}

// Função para consumir tokens
void consome(TAtomo atomo) {
    if (comentario != SEM_COMENTARIO) {
        lookahead = obter_atomo();
    } else if (lookahead.atomo == atomo) {
   //     if( lookahead.atomo == IDENTIFICADOR)
      //      printf("%03d# %s | %s\n",lookahead.linha,msgAtomo[lookahead.atomo], lookahead.atributo_ID);
    //    else if( lookahead.atomo == NUMERO)
    //        printf("%03d# %s | %f\n",lookahead.linha,msgAtomo[lookahead.atomo], lookahead.atributo_numero);
  //      else
  //          printf("%03d# %s\n",lookahead.linha,msgAtomo[lookahead.atomo]);
        InfoAtomo = lookahead;
        lookahead = obter_atomo();
    } else {
        printf("Erro sintatico: esperado [%s] encontrado [%s]\n", msgAtomo[atomo], msgAtomo[lookahead.atomo]);
        exit(1);
    }
}

bool eh_binario(char c){
    if( c == '0' || c == '1')
        return true;
    return false;
}

// Função para verificar se uma string está no array reservado
// retorna indice para evitar uma busca em TAtomo a partir da string
int contem(char **array, int comeco, int tamanho, const char *str) {
    for (int i = comeco; i < comeco + tamanho; i++) {
        if (strcmp(array[i], str) == 0) {
            return i;
        }
    }
    return -1;
}

// funcao para processo repetitivo de mudar informacoes do atomo, atualiza o atomo no endereco
void atualizar_info(TInfoAtomo *atomo, char *comeco, int tamanho, TAtomo tipo, bool comentario, bool binario){
    if (!comentario){
        if( binario){
            // calcula tamanho da substring
            size_t t = tamanho;
            // aloca memoria para substring auxiliar
            char *temp = (char *)malloc((tamanho + 1) * sizeof(char));
            if (temp == NULL) {
                fprintf(stderr, "Erro ao alocar memoria\n");
                exit(1);
            }
            // copia a substring para o buffer temporário
            strncpy(temp, comeco, t);
            temp[t] = '\0'; // adiciona o terminador nulo
            // Converte a substring para número
            atomo->atributo_numero = strtol(temp, NULL, 2); // converte de binario para inteiro
            // libera a memoria alocada
            free(temp);
        }
        else{
            strncpy(atomo->atributo_ID, comeco, tamanho);
            atomo->atributo_ID[tamanho] = 0; // finaliza a string 
            int aux = contem(msgAtomo, reservado_comeco, reservado_tamanho, atomo->atributo_ID);
            if( aux != -1) // se esta no conjunto de palavras reservadas
            {
                tipo = (TAtomo)(aux); // Ajusta o índice para corresponder ao enum TAtomo
            }      
        }
    }
    atomo->atomo = tipo;
}

// IDENTIFICADOR -> LETRA_MINUSCULA (LETRA_MINUSCULA | DIGITO )*
TInfoAtomo reconhece_id(){
    TInfoAtomo info_atomo;
    info_atomo.atomo = ERRO;
    char *iniID = buffer;
    // contador tamanho do identificador, comeca com 1 pois ja considera o primeiro digito
    int contador = 1;
    bool binario = eh_binario(* buffer);

    if ( *buffer == '#'){ // feito antes de consumir pois e so um caractere
        comentario = COMENTARIO_LINHA;
        atualizar_info(&info_atomo, iniID, 1, COMENTARIO, true, binario);
        buffer++;
        return info_atomo;
    }

//TODO: REPORTAR COMENTARIOS PARA SINTATICO
    // ja temos uma letra minuscula / ja temos um '{'
    buffer++;

q0:
    if ( eh_binario(*buffer)){ // reconhece numeros
        buffer++;
        goto q0;
    }
q1:
    if( islower(*buffer) || isdigit(*buffer) || *buffer == '_'){
        buffer++;
        contador += 1;
        // tamanho do identificador limitado a 15
        // se for maior eh erro lexico
        if (contador > 15) {
            return info_atomo;
        }
        goto q1;
    }
    if ( *buffer == '-'){
        comentario = COMENTARIO_BLOCO;
        atualizar_info(&info_atomo, iniID, 2, COMENTARIO, true, binario);
        buffer++;
        return info_atomo;
    }
    if( isupper(*buffer))
        return info_atomo;

    TAtomo tipo = IDENTIFICADOR;
    if( binario)
        tipo = NUMERO;
    char temp[16];
    strncpy(temp, iniID, buffer-iniID);
    temp[buffer - iniID] = '\0';
    atualizar_info(&info_atomo, iniID, buffer-iniID, tipo, false, binario);
    return info_atomo;
}

// Implementação das funções de análise sintática
void programa() {
    printf("\tINPP\n");
    consome(PROGRAM);
    consome(IDENTIFICADOR);
    consome(PONTO_E_VIRGULA);
    bloco();
    consome(PONTO);
    printf("\tPARA\n");
}

void bloco() {
    declaracao_de_variaveis();
    comando_composto();
}

void declaracao_de_variaveis() {
    int k = 0;
    // Consome o tipo da variável (INTEGER ou BOOLEAN). Essa checagem e necessaria para casos com virgula, onde nao se repete o tipo de variavel
    if (lookahead.atomo != INTEGER && lookahead.atomo != BOOLEAN) //Sai do loop se nao houver mais variaveis ou identificadores para lista
        return;
    if (lookahead.atomo == INTEGER || lookahead.atomo == BOOLEAN)
        tipo();

    char** lista = lista_variavel();
    
    for (int i = 0; i < quantidade; i++) {
        if (lista[i][0] == '\0') {  // Verifica se a string está vazia
            break;
        } else {
            criar_simbolo(lista[i]);
            k++;
        }
    }

    // Se o proximo token for uma virgula ou ponto e virgula, consome e continua a lista
    consome(PONTO_E_VIRGULA);
    printf("\tAMEM %d\n", k);
    declaracao_de_variaveis();
}

void tipo() {
    consome(lookahead.atomo);
}

char** lista_variavel() {
    quantidade = 1;
    int i = 0;
    // Aloca memória para o array de ponteiros
    char** lista = (char**)malloc(TAMANHO_MAXIMO * sizeof(char*));
    
    // Aloca memória para cada string (atributo_ID)
    for (int j = 0; j < TAMANHO_MAXIMO; j++) {
        lista[j] = (char*)malloc(TAMANHO_ID * sizeof(char));
    }

    // Consome o primeiro identificador
    consome(IDENTIFICADOR);
    strcpy(lista[i], InfoAtomo.atributo_ID);

    // Verifica se há uma vírgula seguida por outro identificador
    while (lookahead.atomo == VIRGULA) {
        i++;
        consome(VIRGULA);  // Consome a vírgula
        consome(IDENTIFICADOR);  // Consome o identificador
        strcpy(lista[i], InfoAtomo.atributo_ID);
    }

    quantidade += i;
   // printf("QUANTIDADE: %d\n\n", quantidade);

    for (int i = 0; i < quantidade; i++) {
      //  printf("AQUI: %s", lista[i]);
    }
    return lista;
}   

void comando_composto() {
    consome(BEGIN);
    comando();
    while (lookahead.atomo == PONTO_E_VIRGULA) {
        consome(PONTO_E_VIRGULA);
        comando();
    }
    consome(END);
}

void comando() {
    if (lookahead.atomo == SET)
        comando_atribuicao();
    if (lookahead.atomo == IF)
        comando_condicional();
    if (lookahead.atomo == FOR)
        comando_repeticao();
    if (lookahead.atomo == READ)
        comando_entrada();
    if (lookahead.atomo == WRITE)
        comando_saida();
    if (lookahead.atomo == BEGIN)
        comando_composto();
}

void comando_atribuicao() {
    consome(SET);
    consome(IDENTIFICADOR);
    conferir_existencia(InfoAtomo.atributo_ID);
    TInfoAtomo temp = InfoAtomo;
    int endereco = busca_tabela_simbolos(temp.atributo_ID);
    consome(TO);
    expressao();
    printf("\tARMZ %d\n", endereco);
}

void comando_condicional() {
    int L1 = proximo_rotulo();
    int L2 = proximo_rotulo();
    consome(IF);
    expressao();
    consome(DOIS_PONTOS);
    printf("\tDVSF L%d\n",L1);
    comando();
    printf("\tDSVS L%d\n",L2);
    printf("L%d:\tNADA\n",L1);
    if (lookahead.atomo == ELIF){
        consome(ELIF);
        comando();
    }
    printf("L%d:\tNADA\n",L2);
    // Não tem loop para elif pois ele não tem condição na gramática
}

void comando_repeticao() {
    int L1 = proximo_rotulo();
    int L2 = proximo_rotulo();
    consome(FOR);
    consome(IDENTIFICADOR);
    conferir_existencia(InfoAtomo.atributo_ID);
    TInfoAtomo temp = InfoAtomo; //guarda variavel contadora
    int endereco = busca_tabela_simbolos(temp.atributo_ID);
    consome(OF);
    expressao(); //printa valor do OF (expressao() - numero ou variavel - ja cuida desse print sozinho)
    printf("\tARMZ %d\n", endereco); //armazena valor dado em OF na variavel contadora
    
    
    printf("L%d: NADA\n", L1); //rotulo comeco do for (volta pra loopar)
    
    
    printf("\tCRVL %d\n", endereco); //carrega valor da variavel contadora
    
    consome(TO);
    expressao(); //printa valor do TO (expressao() - numero ou variavel - ja cuida desse print sozinho)
    consome(DOIS_PONTOS);

    printf("\tCMEG\n"); //confere se é menor igual
    printf("\tDSVF L%d\n", L2);

    //codigo dentro do for
    comando();
    //depois do codigo dentro do for terminar
    
    printf("\tCRVL %d\n", endereco); //carrega valor da variavel contadora
    printf("\tCRCT 1\n"); //carrega numero 1
    printf("\tSOMA\n"); //soma 1 com valor da variavel
    printf("\tARMZ %d\n", endereco); //guarda valor na variavel contadora (acrescenta em 1)
    printf("\tDSVS L%d\n", L1); //sempre desvia (volta para o comeco) no final do for
    printf("L%d: NADA\n", L2); //rotulo no final (fora) do for
}

void comando_entrada() {
    consome(READ);
    printf("\tLEIT\n");
    consome(ABRE_PARENTESE);
    char ** lista = lista_variavel();
    for (int i = 0; i < 100; i++) {
        if (strcmp(lista[i], "") == 0) {
            break;
        }
        printf("\tARMZ %d\n", busca_tabela_simbolos(lista[i]));
    }
    consome(FECHA_PARENTESE);
}

void comando_saida() {
    consome(WRITE);
    consome(ABRE_PARENTESE);
    expressao();
    printf("\tIMPR\n");
    while (lookahead.atomo == VIRGULA){
        consome(VIRGULA);
        expressao();
        printf("\tIMPR\n");
    }
    consome(FECHA_PARENTESE);
}

void expressao() {
    // Consome a primeira expressão lógica
    expressao_logica();

    // Verifica se há operadores lógicos 'or' seguidos por outras expressões lógicas
    while (lookahead.atomo == OR) {
        consome(OR); // Consome o operador 'or'
        printf("\tDISJ\n");
        expressao_logica(); // Consome a próxima expressão lógica
    }
}

void expressao_logica() {
    // Consome a primeira expressão relacional
    expressao_relacional();

    // Verifica se há operadores lógicos 'and' seguidos por outras expressões relacionais
    while (lookahead.atomo == AND) {
        consome(AND); // Consome o operador 'and'
        printf("\tCONJ\n");
        expressao_relacional(); // Consome a próxima expressão relacional
    }
}

void expressao_relacional() {
    expressao_simples();
    if (lookahead.atomo == MENOR) {
        printf("\tCMME\n");
        op_relacional();
        expressao_simples();
    }
    if (lookahead.atomo == MENOR_IGUAL) {
        printf("\tCMEG\n");
        op_relacional();
        expressao_simples();
    }
    if (lookahead.atomo == IGUAL) {
        printf("\tCMIG\n");
        op_relacional();
        expressao_simples();
    }
    if (lookahead.atomo == DIFERENTE){
        printf("\tCMDG\n");
        op_relacional();
        expressao_simples();
    }
    if (lookahead.atomo == MAIOR) {
        printf("\tCMMA\n");
        op_relacional();
        expressao_simples();
    }
    if (lookahead.atomo == MAIOR_IGUAL) {
        printf("\tCMAG\n");
        op_relacional();
        expressao_simples();
    }
}

void op_relacional() {
    consome(lookahead.atomo);
}

void expressao_simples() {
    termo();
    if (lookahead.atomo == MAIS)
    {
        consome(lookahead.atomo);
        printf("\tSOMA\n");
        expressao_simples();
    }
    else if (lookahead.atomo == MENOS)
    {
        consome(lookahead.atomo);
        printf("\tSUBT\n");
        expressao_simples();
    }
}

void termo() {
    fator();
    if (lookahead.atomo == ASTERISCO)
    {
        consome(lookahead.atomo);
        termo();
        printf("\tMULT\n");
    }
    else if (lookahead.atomo == BARRA)
    {
        consome(lookahead.atomo);
        termo();
        printf("\tDIVI\n");
    }
}

void fator() {
    if (lookahead.atomo == NOT)
    {
        consome(lookahead.atomo);
        fator();
    }
    if (lookahead.atomo == ABRE_PARENTESE)
    {
        consome(ABRE_PARENTESE);
        expressao();
        consome(FECHA_PARENTESE);
    }
    else
    {
        if (lookahead.atomo == IDENTIFICADOR) {
            int endereco = busca_tabela_simbolos(lookahead.atributo_ID);
            printf("\tCRVL %d\n", endereco);
        }
        else if (lookahead.atomo == NUMERO) {
            printf("\tCRCT %f\n", lookahead.atributo_numero);
        }
        consome(lookahead.atomo);
    }
}